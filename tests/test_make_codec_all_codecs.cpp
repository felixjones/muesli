/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/schema/extract>
#include <muesli/schema/make_codec>
#include <muesli/codecs>

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <type_traits>

namespace mu = muesli;

/**
 * @brief Test that make_codec preserves exact primitive type codecs
 *
 * For each identity codec, extract its schema and use make_codec to
 * reconstruct it. Verify that the reconstructed codec has the same
 * value_type as the original.
 */
int main() {
    // -- Fundamental codecs --

    {
        constexpr auto schema = mu::make_schema(mu::bool_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, bool>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::float_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, float>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::double_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, double>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::long_double_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, long double>);
    }

    // -- Character codecs --

    {
        constexpr auto schema = mu::make_schema(mu::char_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, char>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::signed_char_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, signed char>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::unsigned_char_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, unsigned char>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::wchar_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, wchar_t>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::char8_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, char8_t>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::char16_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, char16_t>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::char32_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, char32_t>);
    }

    // -- Fundamental integer codecs (platform-dependent sizes) --

    {
        constexpr auto schema = mu::make_schema(mu::short_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, short>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::unsigned_short_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, unsigned short>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::int_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, int>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::unsigned_int_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, unsigned int>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::long_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, long>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::unsigned_long_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, unsigned long>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::long_long_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, long long>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::unsigned_long_long_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, unsigned long long>);
    }

    // -- Fixed-width integer codecs (std::intN_t / std::uintN_t) --

    {
        constexpr auto schema = mu::make_schema(mu::int8_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, std::int8_t>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::uint8_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, std::uint8_t>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::int16_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, std::int16_t>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::uint16_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, std::uint16_t>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::int32_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, std::int32_t>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::uint32_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, std::uint32_t>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::int64_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, std::int64_t>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::uint64_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, std::uint64_t>);
    }

    // -- Maximum-width and pointer-sized codecs --

    {
        constexpr auto schema = mu::make_schema(mu::intmax_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, std::intmax_t>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::uintmax_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, std::uintmax_t>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::intptr_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, std::intptr_t>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::uintptr_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, std::uintptr_t>);
    }

    // -- Standard definition codecs --

    {
        constexpr auto schema = mu::make_schema(mu::size_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, std::size_t>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::ptrdiff_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, std::ptrdiff_t>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::byte_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, std::byte>);
    }

    // -- String and monostate codecs --

    {
        constexpr auto schema = mu::make_schema(mu::string_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, std::string>);
    }

    {
        constexpr auto schema = mu::make_schema(mu::monostate_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::same_as<typename decltype(codec)::value_type, std::monostate>);
    }

    return 0;
}



