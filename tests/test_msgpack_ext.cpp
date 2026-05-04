/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

/**
 * Tests for the MessagePack extension type system (msgpack_ext_codec).
 *
 * The MessagePack spec defines an ext format that carries a signed 8-bit
 * application-defined type code alongside an arbitrary byte payload.  Type
 * codes in the range -128..-1 are reserved; type -1 is the built-in
 * Timestamp.  Positive codes (1..127) are free for applications to use.
 *
 * This file tests:
 *   1. Multiple custom extension codecs (including variant composition).
 *   2. Fixed-size and variable-size ext wire forms.
 *   3. Error paths: wrong type id, truncated payload, unknown marker.
 */

#include <muesli/format/msgpack_format>
#include <muesli/codecs>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace mu = muesli;

// ---------------------------------------------------------------------------
// Stream helpers
// ---------------------------------------------------------------------------

template<typename Fmt, typename T>
std::string to_bytes(const Fmt& fmt, const T& value) {
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    bool ok = fmt.serialize(value, ss);
    assert(ok);
    return ss.str();
}

template<typename Fmt>
auto from_bytes(const Fmt& fmt, const std::string& data) {
    std::istringstream ss(data, std::ios::binary);
    return fmt.deserialize(ss);
}

static std::string bytes(std::initializer_list<unsigned int> values) {
    std::string result;
    for (auto v : values) {
        result.push_back(static_cast<char>(static_cast<std::uint8_t>(v)));
    }
    return result;
}

// ---------------------------------------------------------------------------
// A 12-byte custom extension payload used to exercise ext8(variable-length)
// and multi-extension composition via variant.
// ---------------------------------------------------------------------------

struct ext_blob12 {
    std::array<std::uint8_t, 12> bytes{};

    bool operator==(const ext_blob12&) const = default;
};

static std::vector<std::byte> blob12_encode(const ext_blob12& blob) {
    std::vector<std::byte> payload(12);
    for (std::size_t i = 0; i < 12; ++i) {
        payload[i] = std::byte{blob.bytes[i]};
    }
    return payload;
}

static std::optional<ext_blob12> blob12_decode(std::span<const std::byte> data) {
    if (data.size() != 12) return std::nullopt;
    ext_blob12 blob{};
    for (std::size_t i = 0; i < 12; ++i) {
        blob.bytes[i] = static_cast<std::uint8_t>(data[i]);
    }
    return blob;
}

static constexpr std::int8_t blob12_type_id = 11;

// ---------------------------------------------------------------------------
// Custom application extension: a 3-byte RGB colour (type 42)
// ---------------------------------------------------------------------------

struct rgb_color {
    std::uint8_t r{0};
    std::uint8_t g{0};
    std::uint8_t b{0};

    bool operator==(const rgb_color&) const = default;
};

static constexpr std::int8_t rgb_type_id = 42;

static std::vector<std::byte> rgb_encode(const rgb_color& c) {
    return {std::byte{c.r}, std::byte{c.g}, std::byte{c.b}};
}

static std::optional<rgb_color> rgb_decode(std::span<const std::byte> data) {
    if (data.size() != 3) return std::nullopt;
    return rgb_color{
        static_cast<std::uint8_t>(data[0]),
        static_cast<std::uint8_t>(data[1]),
        static_cast<std::uint8_t>(data[2]),
    };
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    // =====================================================================
    // 1. Multiple extension codecs in one message shape (variant)
    // =====================================================================
    {
        auto rgb_codec = mu::make_msgpack_ext_codec<rgb_color>(rgb_type_id, rgb_encode, rgb_decode);
        auto blob_codec = mu::make_msgpack_ext_codec<ext_blob12>(blob12_type_id, blob12_encode, blob12_decode);
        using ext_value = std::variant<rgb_color, ext_blob12>;
        auto variant_codec = mu::variant_codec(rgb_codec, blob_codec);
        auto variant_fmt = mu::make_msgpack_format<char>(variant_codec);

        ext_value a = rgb_color{255u, 0u, 0u};
        auto a_decoded = from_bytes(variant_fmt, to_bytes(variant_fmt, a));
        assert(a_decoded.has_value());
        assert(a_decoded->index() == 0);
        rgb_color expected_rgb{255u, 0u, 0u};
        assert(std::get<0>(*a_decoded) == expected_rgb);

        ext_blob12 b_payload{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}};
        ext_value b = b_payload;
        auto b_decoded = from_bytes(variant_fmt, to_bytes(variant_fmt, b));
        assert(b_decoded.has_value());
        assert(b_decoded->index() == 1);
        assert(std::get<1>(*b_decoded) == b_payload);
    }

    // =====================================================================
    // 2. Variable-size ext payload (ext8 with 12-byte body)
    // =====================================================================
    {
        auto blob_codec = mu::make_msgpack_ext_codec<ext_blob12>(blob12_type_id, blob12_encode, blob12_decode);
        auto blob_fmt = mu::make_msgpack_format<char>(blob_codec);
        ext_blob12 payload{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}};
        auto encoded = to_bytes(blob_fmt, payload);
        assert(static_cast<unsigned char>(encoded[0]) == 0xC7);
        assert(static_cast<unsigned char>(encoded[1]) == 0x0C);
        assert(static_cast<unsigned char>(encoded[2]) == static_cast<unsigned char>(blob12_type_id));
        auto decoded = from_bytes(blob_fmt, encoded);
        assert(decoded.has_value());
        assert(*decoded == payload);
    }

    // =====================================================================
    // 3. Custom application extension (rgb_color, type 42)
    //
    //    3 bytes -> fixext with no fixed-3 format -> ext8 with length 3
    //    wire: C7  03  2A  R  G  B
    // =====================================================================
    {
        auto rgb_codec = mu::make_msgpack_ext_codec<rgb_color>(rgb_type_id, rgb_encode, rgb_decode);
        static_assert(mu::Codec<decltype(rgb_codec)>);

        auto rgb_fmt = mu::make_msgpack_format<char>(rgb_codec);
        const rgb_color red{255u, 0u, 0u};

        auto encoded = to_bytes(rgb_fmt, red);
        // ext8: C7, length=3, type=42 (0x2A), R, G, B
        assert(encoded == bytes({0xC7, 0x03, 0x2A, 0xFF, 0x00, 0x00}));

        auto decoded = from_bytes(rgb_fmt, encoded);
        assert(decoded.has_value());
        assert(*decoded == red);
    }
    {
        // fixext 1 (1-byte payload -> 0xD4)
        auto tiny_codec = mu::make_msgpack_ext_codec<std::uint8_t>(
            std::int8_t{10},
            [](const std::uint8_t& v) -> std::vector<std::byte> { return {std::byte{v}}; },
            [](std::span<const std::byte> d) -> std::optional<std::uint8_t> {
                if (d.size() != 1) return std::nullopt;
                return static_cast<std::uint8_t>(d[0]);
            });
        auto tiny_fmt = mu::make_msgpack_format<char>(tiny_codec);
        std::uint8_t val = 0xAB;
        auto encoded = to_bytes(tiny_fmt, val);
        assert(encoded == bytes({0xD4, 0x0A, 0xAB}));
        assert(from_bytes(tiny_fmt, encoded) == std::optional{val});
    }
    {
        // fixext 2 (2-byte payload -> 0xD5)
        auto two_codec = mu::make_msgpack_ext_codec<std::uint16_t>(
            std::int8_t{20},
            [](const std::uint16_t& v) -> std::vector<std::byte> {
                return {std::byte{static_cast<std::uint8_t>(v >> 8u)},
                        std::byte{static_cast<std::uint8_t>(v & 0xFFu)}};
            },
            [](std::span<const std::byte> d) -> std::optional<std::uint16_t> {
                if (d.size() != 2) return std::nullopt;
                return static_cast<std::uint16_t>(
                    (static_cast<std::uint16_t>(static_cast<std::uint8_t>(d[0])) << 8u) |
                     static_cast<std::uint8_t>(d[1]));
            });
        auto two_fmt = mu::make_msgpack_format<char>(two_codec);
        std::uint16_t val = 0x1234;
        auto encoded = to_bytes(two_fmt, val);
        assert(encoded == bytes({0xD5, 0x14, 0x12, 0x34}));
        assert(from_bytes(two_fmt, encoded) == std::optional{val});
    }
    {
        // fixext 2 via range_holder-backed msgpack_ext_payload
        auto range_codec = mu::make_msgpack_ext_codec<std::uint16_t>(
            std::int8_t{21},
            [](const std::uint16_t& v) -> mu::msgpack_ext_payload {
                std::array<std::byte, 2> payload{
                    std::byte{static_cast<std::uint8_t>(v >> 8u)},
                    std::byte{static_cast<std::uint8_t>(v & 0xFFu)}
                };
                return mu::msgpack_ext_payload(std::move(payload));
            },
            [](std::span<const std::byte> d) -> std::optional<std::uint16_t> {
                if (d.size() != 2) return std::nullopt;
                return static_cast<std::uint16_t>(
                    (static_cast<std::uint16_t>(static_cast<std::uint8_t>(d[0])) << 8u) |
                     static_cast<std::uint8_t>(d[1]));
            });
        auto range_fmt = mu::make_msgpack_format<char>(range_codec);
        std::uint16_t val = 0xBEEF;
        auto encoded = to_bytes(range_fmt, val);
        assert(encoded == bytes({0xD5, 0x15, 0xBE, 0xEF}));
        assert(from_bytes(range_fmt, encoded) == std::optional{val});
    }
    {
        // fixext 4 (4-byte payload -> 0xD6, type 7)
        auto four_codec = mu::make_msgpack_ext_codec<std::uint32_t>(
            std::int8_t{7},
            [](const std::uint32_t& v) -> std::vector<std::byte> {
                return {std::byte{static_cast<std::uint8_t>((v >> 24u) & 0xFFu)},
                        std::byte{static_cast<std::uint8_t>((v >> 16u) & 0xFFu)},
                        std::byte{static_cast<std::uint8_t>((v >>  8u) & 0xFFu)},
                        std::byte{static_cast<std::uint8_t>( v         & 0xFFu)}};
            },
            [](std::span<const std::byte> d) -> std::optional<std::uint32_t> {
                if (d.size() != 4) return std::nullopt;
                std::uint32_t v = 0;
                for (std::size_t i = 0; i < 4; ++i) v = (v << 8u) | static_cast<std::uint8_t>(d[i]);
                return v;
            });
        auto four_fmt = mu::make_msgpack_format<char>(four_codec);
        std::uint32_t val = 0xDEADBEEFu;
        auto encoded = to_bytes(four_fmt, val);
        assert(encoded == bytes({0xD6, 0x07, 0xDE, 0xAD, 0xBE, 0xEF}));
        assert(from_bytes(four_fmt, encoded) == std::optional{val});
    }
    {
        // fixext 8 (8-byte payload -> 0xD7, type 8)
        auto eight_codec = mu::make_msgpack_ext_codec<std::uint64_t>(
            std::int8_t{8},
            [](const std::uint64_t& v) -> std::vector<std::byte> {
                std::vector<std::byte> data(8);
                auto val = v;
                for (int i = 7; i >= 0; --i) {
                    data[static_cast<std::size_t>(i)] = std::byte{static_cast<std::uint8_t>(val & 0xFFu)};
                    val >>= 8u;
                }
                return data;
            },
            [](std::span<const std::byte> d) -> std::optional<std::uint64_t> {
                if (d.size() != 8) return std::nullopt;
                std::uint64_t v = 0;
                for (std::size_t i = 0; i < 8; ++i) v = (v << 8u) | static_cast<std::uint8_t>(d[i]);
                return v;
            });
        auto eight_fmt = mu::make_msgpack_format<char>(eight_codec);
        std::uint64_t val = 0x0102030405060708ull;
        auto encoded = to_bytes(eight_fmt, val);
        assert(encoded == bytes({0xD7, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}));
        assert(from_bytes(eight_fmt, encoded) == std::optional{val});
    }
    {
        // fixext 16 (16-byte payload -> 0xD8)
        using uuid_t = std::array<std::uint8_t, 16>;
        auto uuid_codec = mu::make_msgpack_ext_codec<uuid_t>(
            std::int8_t{3},
            [](const uuid_t& v) -> std::vector<std::byte> {
                std::vector<std::byte> data(16);
                for (std::size_t i = 0; i < 16; ++i) data[i] = std::byte{v[i]};
                return data;
            },
            [](std::span<const std::byte> d) -> std::optional<uuid_t> {
                if (d.size() != 16) return std::nullopt;
                uuid_t arr{};
                for (std::size_t i = 0; i < 16; ++i) arr[i] = static_cast<std::uint8_t>(d[i]);
                return arr;
            });
        auto uuid_fmt = mu::make_msgpack_format<char>(uuid_codec);
        uuid_t uuid{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        auto encoded = to_bytes(uuid_fmt, uuid);
        assert(static_cast<unsigned char>(encoded[0]) == 0xD8);
        assert(static_cast<unsigned char>(encoded[1]) == 0x03);
        assert(encoded.size() == 18);
        assert(from_bytes(uuid_fmt, encoded) == std::optional{uuid});
    }

    // =====================================================================
    // 4. Error paths
    // =====================================================================
    {
        // Wrong type id in payload -> nullopt
        auto blob_codec = mu::make_msgpack_ext_codec<ext_blob12>(blob12_type_id, blob12_encode, blob12_decode);
        auto blob_fmt = mu::make_msgpack_format<char>(blob_codec);
        auto wrong_type_wire = bytes({0xC7, 0x0C, 0x00,
                                      0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                                      0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B});
        assert(!from_bytes(blob_fmt, wrong_type_wire).has_value());
    }
    {
        // Truncated ext payload -> nullopt
        auto blob_codec = mu::make_msgpack_ext_codec<ext_blob12>(blob12_type_id, blob12_encode, blob12_decode);
        auto blob_fmt = mu::make_msgpack_format<char>(blob_codec);
        auto truncated = bytes({0xC7, 0x0C, static_cast<unsigned char>(blob12_type_id), 0x00, 0x00});
        assert(!from_bytes(blob_fmt, truncated).has_value());
    }
    {
        // Non-ext marker for an ext codec -> nullopt
        auto blob_codec = mu::make_msgpack_ext_codec<ext_blob12>(blob12_type_id, blob12_encode, blob12_decode);
        auto blob_fmt = mu::make_msgpack_format<char>(blob_codec);
        auto non_ext = bytes({0xC0}); // nil
        assert(!from_bytes(blob_fmt, non_ext).has_value());
    }
    {
        // Decoder returns nullopt for bad payload size -> propagated as nullopt
        auto rgb_codec = mu::make_msgpack_ext_codec<rgb_color>(rgb_type_id, rgb_encode, rgb_decode);
        auto rgb_fmt = mu::make_msgpack_format<char>(rgb_codec);
        // ext8 with correct type but wrong payload length (5 bytes instead of 3)
        auto bad_size = bytes({0xC7, 0x05, 0x2A, 0xFF, 0x00, 0x00, 0x00, 0x00});
        assert(!from_bytes(rgb_fmt, bad_size).has_value());
    }

    return 0;
}




