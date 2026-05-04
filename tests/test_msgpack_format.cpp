/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/format/msgpack_format>
#include <muesli/codecs>

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace mu = muesli;

template<typename Fmt, typename T>
std::string to_bytes(const Fmt& fmt, const T& value) {
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    bool ok = fmt.serialize(value, ss);
    assert(ok);
    return ss.str();
}

template<typename Fmt>
auto from_bytes(const Fmt& fmt, const std::string& bytes) {
    std::istringstream ss(bytes, std::ios::binary);
    return fmt.deserialize(ss);
}

template<typename T>
std::vector<T> collect_range(mu::range_holder<T>&& holder) {
    std::vector<T> values;
    for (auto&& v : holder) {
        values.push_back(v);
    }
    return values;
}

static std::string bytes(std::initializer_list<unsigned int> values) {
    std::string result;
    for (auto value : values) {
        result.push_back(static_cast<char>(value));
    }
    return result;
}

struct named_point {
    int x;
    int y;

    static constexpr auto codec = mu::tuple_codec(
        mu::int_codec.member<&named_point::x>().named("x"),
        mu::int_codec.member<&named_point::y>().named("y")
    ).apply<named_point>();
};

struct profile {
    std::string name;
    std::optional<std::int32_t> score;

    static constexpr auto codec = mu::tuple_codec(
        mu::string_codec.member<&profile::name>().named("name"),
        mu::optional_codec(mu::int32_codec).member<&profile::score>().named("score")
    ).apply<profile>();
};

struct server_config {
    std::int32_t port;

    static constexpr auto codec = mu::tuple_codec(
        mu::int32_codec.or_else([] { return std::int32_t{8080}; }).member<&server_config::port>().named("port")
    ).apply<server_config>();
};

int main() {
    // =====================================================================
    // 1. Exact scalar encodings
    // =====================================================================
    {
        auto fmt = mu::make_msgpack_format<char>(mu::monostate_codec);
        assert(to_bytes(fmt, std::monostate{}) == bytes({0xC0}));
        auto decoded = from_bytes(fmt, bytes({0xC0}));
        assert(decoded.has_value());
    }
    {
        auto fmt = mu::make_msgpack_format<char>(mu::bool_codec);
        assert(to_bytes(fmt, true) == bytes({0xC3}));
        assert(to_bytes(fmt, false) == bytes({0xC2}));
        assert(from_bytes(fmt, bytes({0xC3})) == std::optional<bool>{true});
        assert(from_bytes(fmt, bytes({0xC2})) == std::optional<bool>{false});
    }
    {
        auto fmt = mu::make_msgpack_format<char>(mu::int8_codec);
        assert(to_bytes(fmt, std::int8_t{-32}) == bytes({0xE0}));
        assert(to_bytes(fmt, std::int8_t{-33}) == bytes({0xD0, 0xDF}));
        assert(from_bytes(fmt, bytes({0xE0})) == std::optional<std::int8_t>{-32});
        assert(from_bytes(fmt, bytes({0xD0, 0xDF})) == std::optional<std::int8_t>{-33});
    }
    {
        auto fmt = mu::make_msgpack_format<char>(mu::uint16_codec);
        assert(to_bytes(fmt, std::uint16_t{127}) == bytes({0x7F}));
        assert(to_bytes(fmt, std::uint16_t{128}) == bytes({0xCC, 0x80}));
        assert(to_bytes(fmt, std::uint16_t{256}) == bytes({0xCD, 0x01, 0x00}));
        assert(from_bytes(fmt, bytes({0xCD, 0x01, 0x00})) == std::optional<std::uint16_t>{256});
    }
    {
        auto fmt = mu::make_msgpack_format<char>(mu::uint64_codec);
        assert(to_bytes(fmt, std::uint64_t{0x1'0000'0000ULL}) == bytes({0xCF, 0x00, 0x00, 0x00, 0x01,
                                                                        0x00, 0x00, 0x00, 0x00}));
        assert(from_bytes(fmt, bytes({0xCF, 0x00, 0x00, 0x00, 0x01,
                                      0x00, 0x00, 0x00, 0x00})) ==
               std::optional<std::uint64_t>{0x1'0000'0000ULL});
    }
    {
        auto fmt = mu::make_msgpack_format<char>(mu::float_codec);
        assert(to_bytes(fmt, 1.0f) == bytes({0xCA, 0x3F, 0x80, 0x00, 0x00}));
        auto decoded = from_bytes(fmt, bytes({0xCA, 0x3F, 0x80, 0x00, 0x00}));
        assert(decoded && *decoded == 1.0f);
    }
    {
        auto fmt = mu::make_msgpack_format<char>(mu::double_codec);
        assert(to_bytes(fmt, 1.0) == bytes({0xCB, 0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}));
        auto decoded = from_bytes(fmt, bytes({0xCB, 0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}));
        assert(decoded && *decoded == 1.0);
    }

    // =====================================================================
    // 2. Strings and arrays
    // =====================================================================
    {
        auto fmt = mu::make_msgpack_format<char>(mu::string_codec);
        assert(to_bytes(fmt, std::string{"hello"}) == bytes({0xA5, 'h', 'e', 'l', 'l', 'o'}));
        auto decoded = from_bytes(fmt, bytes({0xA5, 'h', 'e', 'l', 'l', 'o'}));
        assert(decoded && *decoded == "hello");
    }
    {
        auto fmt = mu::make_msgpack_format<char>(mu::string_codec);
        std::string value(32, 'x');
        auto encoded = to_bytes(fmt, value);
        assert(static_cast<unsigned char>(encoded[0]) == 0xD9);
        assert(static_cast<unsigned char>(encoded[1]) == 32);
        auto decoded = from_bytes(fmt, encoded);
        assert(decoded && *decoded == value);
    }
    {
        auto fmt = mu::make_msgpack_format<char>(mu::vector_of(mu::int32_codec));
        std::vector<std::int32_t> value = {1, 2, 3};
        assert(to_bytes(fmt, value) == bytes({0x93, 0x01, 0x02, 0x03}));
        auto decoded = from_bytes(fmt, bytes({0x93, 0x01, 0x02, 0x03}));
        assert(decoded && *decoded == value);
    }
    {
        auto fmt = mu::make_msgpack_format<char>(mu::vector_of(mu::uint8_codec));
        std::vector<std::uint8_t> value(16, 1);
        auto encoded = to_bytes(fmt, value);
        assert(static_cast<unsigned char>(encoded[0]) == 0xDC);
        assert(static_cast<unsigned char>(encoded[1]) == 0x00);
        assert(static_cast<unsigned char>(encoded[2]) == 0x10);
        auto decoded = from_bytes(fmt, encoded);
        assert(decoded && *decoded == value);
    }
    {
        auto fmt = mu::make_msgpack_format<char>(mu::array_of<3>(mu::int32_codec));
        std::array<std::int32_t, 3> value = {4, 5, 6};
        auto decoded = from_bytes(fmt, to_bytes(fmt, value));
        assert(decoded && *decoded == value);
        assert(!from_bytes(fmt, bytes({0x92, 0x01, 0x02})).has_value());
    }
    {
        auto codec = mu::range_codec(mu::uint8_codec);
        auto fmt = mu::make_msgpack_format<char>(codec);

        std::vector<std::uint8_t> payload = {0x01, 0x02, 0x7E, 0x7F};
        auto encoded = to_bytes(fmt, mu::range_holder<std::uint8_t>(payload));
        assert(encoded == bytes({0xC4, 0x04, 0x01, 0x02, 0x7E, 0x7F}));

        auto decoded = from_bytes(fmt, encoded);
        assert(decoded.has_value());
        assert(collect_range(std::move(*decoded)) == payload);

        // Backward-compatible decode of legacy array form.
        auto legacy = from_bytes(fmt, bytes({0x94, 0x01, 0x02, 0x7E, 0x7F}));
        assert(legacy.has_value());
        assert(collect_range(std::move(*legacy)) == payload);
    }
    {
        auto codec = mu::range_codec(mu::identity_codec<std::byte>{});
        auto fmt = mu::make_msgpack_format<char>(codec);

        std::vector<std::byte> payload = {std::byte{0xAA}, std::byte{0xBB}, std::byte{0xCC}};
        auto encoded = to_bytes(fmt, mu::range_holder<std::byte>(payload));
        assert(encoded == bytes({0xC4, 0x03, 0xAA, 0xBB, 0xCC}));

        auto decoded = from_bytes(fmt, encoded);
        assert(decoded.has_value());
        assert(collect_range(std::move(*decoded)) == payload);
    }
    {
        auto codec = mu::range_codec(mu::uint8_codec);
        auto fmt = mu::make_msgpack_format<char>(codec);

        std::vector<std::uint8_t> bin16_payload(300, 0x7A);
        auto bin16_encoded = to_bytes(fmt, mu::range_holder<std::uint8_t>(bin16_payload));
        assert(static_cast<unsigned char>(bin16_encoded[0]) == 0xC5);
        assert(static_cast<unsigned char>(bin16_encoded[1]) == 0x01);
        assert(static_cast<unsigned char>(bin16_encoded[2]) == 0x2C);

        std::vector<std::uint8_t> bin32_payload(70000, 0x3C);
        auto bin32_encoded = to_bytes(fmt, mu::range_holder<std::uint8_t>(bin32_payload));
        assert(static_cast<unsigned char>(bin32_encoded[0]) == 0xC6);
        assert(static_cast<unsigned char>(bin32_encoded[1]) == 0x00);
        assert(static_cast<unsigned char>(bin32_encoded[2]) == 0x01);
        assert(static_cast<unsigned char>(bin32_encoded[3]) == 0x11);
        assert(static_cast<unsigned char>(bin32_encoded[4]) == 0x70);
    }

    // =====================================================================
    // 3. Tuples, variants, optionals, maps, and structs
    // =====================================================================
    {
        auto codec = mu::tuple_codec(mu::int32_codec, mu::string_codec, mu::bool_codec);
        auto fmt = mu::make_msgpack_format<char>(codec);
        auto value = std::tuple{std::int32_t{42}, std::string{"answer"}, true};
        auto decoded = from_bytes(fmt, to_bytes(fmt, value));
        assert(decoded && std::get<0>(*decoded) == 42);
        assert(std::get<1>(*decoded) == "answer");
        assert(std::get<2>(*decoded));
    }
    {
        using value_t = std::variant<std::int32_t, std::string>;
        auto codec = mu::variant_codec(mu::int32_codec, mu::string_codec);
        auto fmt = mu::make_msgpack_format<char>(codec);
        value_t first = std::int32_t{77};
        value_t second = std::string{"seventy-seven"};
        auto decoded_first = from_bytes(fmt, to_bytes(fmt, first));
        auto decoded_second = from_bytes(fmt, to_bytes(fmt, second));
        assert(decoded_first && decoded_first->index() == 0 && std::get<0>(*decoded_first) == 77);
        assert(decoded_second && decoded_second->index() == 1 && std::get<1>(*decoded_second) == "seventy-seven");
        assert(!from_bytes(fmt, bytes({0x92, 0x02, 0x00})).has_value());
    }
    {
        auto fmt = mu::make_msgpack_format<char>(mu::optional_codec(mu::string_codec));
        std::optional<std::string> present = "hello";
        std::optional<std::string> absent;
        auto decoded_present = from_bytes(fmt, to_bytes(fmt, present));
        auto decoded_absent = from_bytes(fmt, to_bytes(fmt, absent));
        assert(decoded_present && decoded_present->has_value() && **decoded_present == "hello");
        assert(decoded_absent && !decoded_absent->has_value());
        assert(to_bytes(fmt, absent) == bytes({0xC0}));
    }
    {
        auto map_codec = mu::vector_of(mu::pair_codec(mu::string_codec, mu::int32_codec))
                             .apply<std::map<std::string, std::int32_t>>();
        auto fmt = mu::make_msgpack_format<char>(map_codec);
        std::map<std::string, std::int32_t> value = {{"one", 1}, {"two", 2}};
        auto encoded = to_bytes(fmt, value);
        assert(encoded == bytes({0x82, 0xA3, 'o', 'n', 'e', 0x01, 0xA3, 't', 'w', 'o', 0x02}));
        auto decoded = from_bytes(fmt, encoded);
        assert(decoded && *decoded == value);
        decoded = from_bytes(fmt, bytes({0x82, 0xA3, 'o', 'n', 'e', 0x01, 0xA3, 't', 'w', 'o', 0x02}));
        assert(decoded && *decoded == value);
    }
    {
        auto fmt = mu::make_msgpack_format<char>(named_point::codec);
        named_point point{10, 20};
        assert(to_bytes(fmt, point) == bytes({0x82, 0xA1, 'x', 0x0A, 0xA1, 'y', 0x14}));

        // Key order is irrelevant for named-member maps.
        auto decoded = from_bytes(fmt, bytes({0x82, 0xA1, 'y', 0x14, 0xA1, 'x', 0x0A}));
        assert(decoded && decoded->x == 10 && decoded->y == 20);
    }
    {
        auto fmt = mu::make_msgpack_format<char>(profile::codec);
        profile value{"Alice", std::int32_t{100}};
        auto decoded = from_bytes(fmt, to_bytes(fmt, value));
        assert(decoded && decoded->name == "Alice");
        assert(decoded->score && *decoded->score == 100);

        profile absent{"Bob", std::nullopt};
        auto decoded_absent = from_bytes(fmt, to_bytes(fmt, absent));
        assert(decoded_absent && decoded_absent->name == "Bob" && !decoded_absent->score);
    }
    {
        auto fmt = mu::make_msgpack_format<char>(server_config::codec);
        auto decoded = from_bytes(fmt, bytes({0x80}));
        assert(decoded && decoded->port == 8080);
    }

    // =====================================================================
    // 4. Malformed input
    // =====================================================================
    {
        auto fmt = mu::make_msgpack_format<char>(mu::int32_codec);
        assert(!from_bytes(fmt, std::string{}).has_value());
        assert(!from_bytes(fmt, bytes({0xC0})).has_value());
        assert(!from_bytes(fmt, bytes({0xCD, 0x01})).has_value());
    }
    {
        auto fmt = mu::make_msgpack_format<char>(mu::string_codec);
        assert(!from_bytes(fmt, bytes({0xA5, 'h', 'e'})).has_value());
    }
    {
        auto fmt = mu::make_msgpack_format<char>(named_point::codec);
        assert(!from_bytes(fmt, bytes({0x81, 0xA1, 'x', 0x0A})).has_value());
    }

    return 0;
}

