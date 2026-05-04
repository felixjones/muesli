/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/format/msgpack_timestamp_ext>

#include <cassert>
#include <chrono>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>

namespace mu = muesli;

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

int main() {
    using ns_time = std::chrono::sys_time<std::chrono::nanoseconds>;
    auto codec = mu::make_msgpack_timestamp_ext_codec<>();
    auto fmt = mu::make_msgpack_format<char>(codec);

    {
        const ns_time value{std::chrono::seconds{1'000'000'000}};
        auto encoded = to_bytes(fmt, value);
        assert(encoded == bytes({0xD6, 0xFF, 0x3B, 0x9A, 0xCA, 0x00}));
        assert(from_bytes(fmt, encoded) == std::optional{value});
    }
    {
        const ns_time value{std::chrono::seconds{5} + std::chrono::nanoseconds{100'000}};
        auto encoded = to_bytes(fmt, value);
        assert(encoded == bytes({0xD7, 0xFF, 0x00, 0x06, 0x1A, 0x80, 0x00, 0x00, 0x00, 0x05}));
        assert(from_bytes(fmt, encoded) == std::optional{value});
    }
    {
        const ns_time value{std::chrono::seconds{-1} + std::chrono::nanoseconds{500'000'000}};
        auto encoded = to_bytes(fmt, value);
        assert(encoded == bytes({0xC7, 0x0C, 0xFF,
                                 0x1D, 0xCD, 0x65, 0x00,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}));
        assert(from_bytes(fmt, encoded) == std::optional{value});
    }

    {
        using ms_time = std::chrono::sys_time<std::chrono::milliseconds>;
        auto ms_codec = mu::make_msgpack_timestamp_ext_codec<std::chrono::milliseconds>();
        auto ms_fmt = mu::make_msgpack_format<char>(ms_codec);

        const ms_time value{std::chrono::milliseconds{12'345}};
        auto decoded = from_bytes(ms_fmt, to_bytes(ms_fmt, value));
        assert(decoded == std::optional{value});
    }

    return 0;
}

