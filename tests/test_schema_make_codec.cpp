/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/schema/extract>
#include <muesli/schema/make_codec>
#include <muesli/format/binary_format>
#include <muesli/codecs>

#include <cassert>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace mu = muesli;
namespace s = muesli::schema;

// -- Test structs (original typed codecs) -----------------------------------

struct point {
    int x, y;
    static constexpr auto codec = mu::tuple_codec(
        mu::int_codec.member<&point::x>().named("x"),
        mu::int_codec.member<&point::y>().named("y")
    ).apply<point>();
};

struct person {
    int age;
    std::string name;
    static constexpr auto codec = mu::tuple_codec(
        mu::int_codec.member<&person::age>().named("age"),
        mu::string_codec.member<&person::name>().named("name")
    ).apply<person>();
};

struct profile {
    std::string username;
    std::optional<std::string> bio;
    static constexpr auto codec = mu::tuple_codec(
        mu::string_codec.member<&profile::username>().named("username"),
        mu::optional_codec(mu::string_codec).member<&profile::bio>().named("bio")
    ).apply<profile>();
};

// -- Helpers ----------------------------------------------------------------

/// Serialize a value to a string of bytes
template<typename Fmt, typename T>
std::string to_bytes(const Fmt& fmt, const T& value) {
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    bool ok = fmt.serialize(value, ss);
    assert(ok);
    return ss.str();
}

/// Deserialize a value from a string of bytes
template<typename Fmt>
auto from_bytes(const Fmt& fmt, const std::string& bytes) {
    std::istringstream ss(bytes, std::ios::binary);
    return fmt.deserialize(ss);
}

int main() {
    // =====================================================================
    // 1. Primitive codec from schema -- round-trip
    // =====================================================================

    // -- 1a: signed_integer -> int64_codec
    {
        constexpr auto schema = mu::make_schema(mu::int32_codec);
        constexpr auto codec = mu::make_codec(schema);
        auto fmt = mu::make_binary_format<char>(codec);

        auto bytes = to_bytes(fmt, std::int64_t{42});
        auto d = from_bytes(fmt, bytes);
        assert(d && *d == 42);
    }
    // -- 1b: negative signed_integer
    {
        constexpr auto schema = mu::make_schema(mu::int32_codec);
        constexpr auto codec = mu::make_codec(schema);
        auto fmt = mu::make_binary_format<char>(codec);

        auto bytes = to_bytes(fmt, std::int64_t{-999});
        auto d = from_bytes(fmt, bytes);
        assert(d && *d == -999);
    }
    // -- 1c: unsigned_integer -> uint64_codec
    {
        constexpr auto schema = mu::make_schema(mu::uint32_codec);
        constexpr auto codec = mu::make_codec(schema);
        auto fmt = mu::make_binary_format<char>(codec);

        auto bytes = to_bytes(fmt, std::uint64_t{999});
        auto d = from_bytes(fmt, bytes);
        assert(d && *d == 999u);
    }
    // -- 1d: float_32 -> float_codec
    {
        constexpr auto schema = mu::make_schema(mu::float_codec);
        constexpr auto codec = mu::make_codec(schema);
        static_assert(std::is_same_v<typename std::decay_t<decltype(codec)>::value_type, float>);
        auto fmt = mu::make_binary_format<char>(codec);

        auto bytes = to_bytes(fmt, 3.14f);
        auto d = from_bytes(fmt, bytes);
        assert(d && (*d - 3.14f) < 0.001f);
    }
    // -- 1e: boolean -> bool_codec
    {
        constexpr auto schema = mu::make_schema(mu::bool_codec);
        constexpr auto codec = mu::make_codec(schema);
        auto fmt = mu::make_binary_format<char>(codec);

        auto bytes_t = to_bytes(fmt, true);
        auto d_t = from_bytes(fmt, bytes_t);
        assert(d_t && *d_t == true);

        auto bytes_f = to_bytes(fmt, false);
        auto d_f = from_bytes(fmt, bytes_f);
        assert(d_f && *d_f == false);
    }
    // -- 1f: string -> string_codec
    {
        constexpr auto schema = mu::make_schema(mu::string_codec);
        constexpr auto codec = mu::make_codec(schema);
        auto fmt = mu::make_binary_format<char>(codec);

        auto bytes = to_bytes(fmt, std::string{"hello"});
        auto d = from_bytes(fmt, bytes);
        assert(d && *d == "hello");
    }
    // -- 1g: monostate -> monostate_codec
    {
        constexpr auto schema = mu::make_schema(mu::monostate_codec);
        constexpr auto codec = mu::make_codec(schema);
        auto fmt = mu::make_binary_format<char>(codec);

        auto bytes = to_bytes(fmt, std::monostate{});
        auto d = from_bytes(fmt, bytes);
        assert(d.has_value());
    }
    // -- 1h: int8 schema -> int64 round-trip
    {
        constexpr auto schema = mu::make_schema(mu::int8_codec);
        constexpr auto codec = mu::make_codec(schema);
        auto fmt = mu::make_binary_format<char>(codec);

        auto bytes = to_bytes(fmt, std::int64_t{-128});
        auto d = from_bytes(fmt, bytes);
        assert(d && *d == -128);
    }
    // -- 1i: uint8 schema -> uint64 round-trip
    {
        constexpr auto schema = mu::make_schema(mu::uint8_codec);
        constexpr auto codec = mu::make_codec(schema);
        auto fmt = mu::make_binary_format<char>(codec);

        auto bytes = to_bytes(fmt, std::uint64_t{255});
        auto d = from_bytes(fmt, bytes);
        assert(d && *d == 255u);
    }
    // -- 1j: double schema -> double round-trip (same canonical type)
    {
        constexpr auto schema = mu::make_schema(mu::double_codec);
        constexpr auto codec = mu::make_codec(schema);
        auto fmt = mu::make_binary_format<char>(codec);

        auto bytes = to_bytes(fmt, 2.718281828);
        auto d = from_bytes(fmt, bytes);
        assert(d && *d == 2.718281828);
    }
    // -- 1k: empty string round-trip
    {
        constexpr auto schema = mu::make_schema(mu::string_codec);
        constexpr auto codec = mu::make_codec(schema);
        auto fmt = mu::make_binary_format<char>(codec);

        auto bytes = to_bytes(fmt, std::string{""});
        auto d = from_bytes(fmt, bytes);
        assert(d && d->empty());
    }

    // =====================================================================
    // 2. Object codec from schema (named tuple -> binary tuple)
    // =====================================================================

    // -- 2a: point schema -> named tuple codec
    {
        constexpr auto schema = mu::make_schema(point::codec);
        constexpr auto codec = mu::make_codec(schema);

        using val_t = typename std::decay_t<decltype(codec)>::value_type;
        auto fmt = mu::make_binary_format<char>(codec);

        val_t value{std::int64_t{10}, std::int64_t{20}};
        auto bytes = to_bytes(fmt, value);
        auto d = from_bytes(fmt, bytes);
        assert(d.has_value());
        assert(std::get<0>(*d) == 10);
        assert(std::get<1>(*d) == 20);
    }
    // -- 2b: person schema -> named tuple with int64 + string
    {
        constexpr auto schema = mu::make_schema(person::codec);
        constexpr auto codec = mu::make_codec(schema);

        using val_t = typename std::decay_t<decltype(codec)>::value_type;
        auto fmt = mu::make_binary_format<char>(codec);

        val_t value{std::int64_t{30}, std::string{"Alice"}};
        auto bytes = to_bytes(fmt, value);
        auto d = from_bytes(fmt, bytes);
        assert(d.has_value());
        assert(std::get<0>(*d) == 30);
        assert(std::get<1>(*d) == "Alice");
    }
    // -- 2c: point with negative values
    {
        constexpr auto schema = mu::make_schema(point::codec);
        constexpr auto codec = mu::make_codec(schema);

        using val_t = typename std::decay_t<decltype(codec)>::value_type;
        auto fmt = mu::make_binary_format<char>(codec);

        val_t value{std::int64_t{-100}, std::int64_t{-200}};
        auto bytes = to_bytes(fmt, value);
        auto d = from_bytes(fmt, bytes);
        assert(d.has_value());
        assert(std::get<0>(*d) == -100);
        assert(std::get<1>(*d) == -200);
    }
    // -- 2d: point with zero values
    {
        constexpr auto schema = mu::make_schema(point::codec);
        constexpr auto codec = mu::make_codec(schema);

        using val_t = typename std::decay_t<decltype(codec)>::value_type;
        auto fmt = mu::make_binary_format<char>(codec);

        val_t value{std::int64_t{0}, std::int64_t{0}};
        auto bytes = to_bytes(fmt, value);
        auto d = from_bytes(fmt, bytes);
        assert(d.has_value());
        assert(std::get<0>(*d) == 0);
        assert(std::get<1>(*d) == 0);
    }

    // =====================================================================
    // 3. Array codec from schema
    // =====================================================================

    // -- 3a: vector<int> schema -> vector_of(int64_codec)
    {
        constexpr auto schema = mu::make_schema(mu::vector_of(mu::int32_codec));
        constexpr auto codec = mu::make_codec(schema);
        auto fmt = mu::make_binary_format<char>(codec);

        std::vector<std::int64_t> value = {1, 2, 3};
        auto bytes = to_bytes(fmt, value);
        auto d = from_bytes(fmt, bytes);
        assert(d && d->size() == 3);
        assert((*d)[0] == 1 && (*d)[1] == 2 && (*d)[2] == 3);
    }
    // -- 3b: empty vector
    {
        constexpr auto schema = mu::make_schema(mu::vector_of(mu::int32_codec));
        constexpr auto codec = mu::make_codec(schema);
        auto fmt = mu::make_binary_format<char>(codec);

        std::vector<std::int64_t> value = {};
        auto bytes = to_bytes(fmt, value);
        auto d = from_bytes(fmt, bytes);
        assert(d && d->empty());
    }
    // -- 3c: vector<string>
    {
        constexpr auto schema = mu::make_schema(mu::vector_of(mu::string_codec));
        constexpr auto codec = mu::make_codec(schema);
        auto fmt = mu::make_binary_format<char>(codec);

        std::vector<std::string> value = {"alpha", "beta", "gamma"};
        auto bytes = to_bytes(fmt, value);
        auto d = from_bytes(fmt, bytes);
        assert(d && d->size() == 3);
        assert((*d)[0] == "alpha" && (*d)[1] == "beta" && (*d)[2] == "gamma");
    }
    // -- 3d: vector<float>
    {
        constexpr auto schema = mu::make_schema(mu::vector_of(mu::float_codec));
        constexpr auto codec = mu::make_codec(schema);
        auto fmt = mu::make_binary_format<char>(codec);

        std::vector<float> value = {1.1f, 2.2f, 3.3f};
        auto bytes = to_bytes(fmt, value);
        auto d = from_bytes(fmt, bytes);
        assert(d && d->size() == 3);
        assert((*d)[0] == 1.1f && (*d)[1] == 2.2f && (*d)[2] == 3.3f);
    }

    // =====================================================================
    // 4. Optional codec from schema
    // =====================================================================

    // -- 4a: optional<string> present
    {
        constexpr auto schema = mu::make_schema(mu::optional_codec(mu::string_codec));
        constexpr auto codec = mu::make_codec(schema);
        auto fmt = mu::make_binary_format<char>(codec);

        std::optional<std::string> present{"hello"};
        auto bytes = to_bytes(fmt, present);
        auto d = from_bytes(fmt, bytes);
        assert(d && d->has_value() && **d == "hello");
    }
    // -- 4b: optional<string> absent
    {
        constexpr auto schema = mu::make_schema(mu::optional_codec(mu::string_codec));
        constexpr auto codec = mu::make_codec(schema);
        auto fmt = mu::make_binary_format<char>(codec);

        std::optional<std::string> absent;
        auto bytes = to_bytes(fmt, absent);
        auto d = from_bytes(fmt, bytes);
        assert(d && !d->has_value());
    }
    // -- 4c: optional<int64> present
    {
        constexpr auto schema = mu::make_schema(mu::optional_codec(mu::int32_codec));
        constexpr auto codec = mu::make_codec(schema);
        auto fmt = mu::make_binary_format<char>(codec);

        std::optional<std::int64_t> present{42};
        auto bytes = to_bytes(fmt, present);
        auto d = from_bytes(fmt, bytes);
        assert(d && d->has_value() && **d == 42);
    }
    // -- 4d: optional<int64> absent
    {
        constexpr auto schema = mu::make_schema(mu::optional_codec(mu::int32_codec));
        constexpr auto codec = mu::make_codec(schema);
        auto fmt = mu::make_binary_format<char>(codec);

        std::optional<std::int64_t> absent;
        auto bytes = to_bytes(fmt, absent);
        auto d = from_bytes(fmt, bytes);
        assert(d && !d->has_value());
    }

    // =====================================================================
    // 5. Tuple codec from schema (positional)
    // =====================================================================

    // -- 5a: tuple<int64, float, bool>
    {
        constexpr auto origCodec = mu::tuple_codec(mu::int32_codec, mu::float_codec, mu::bool_codec);
        constexpr auto schema = mu::make_schema(origCodec);
        constexpr auto codec = mu::make_codec(schema);

        using val_t = typename std::decay_t<decltype(codec)>::value_type;
        auto fmt = mu::make_binary_format<char>(codec);

        val_t value{std::int64_t{7}, 2.5f, true};
        auto bytes = to_bytes(fmt, value);
        auto d = from_bytes(fmt, bytes);
        assert(d.has_value());
        assert(std::get<0>(*d) == 7);
        assert(std::get<1>(*d) == 2.5f);
        assert(std::get<2>(*d) == true);
    }
    // -- 5b: tuple<string, uint64> (string before int tests sequential stream consumption)
    {
        constexpr auto origCodec = mu::tuple_codec(mu::string_codec, mu::uint32_codec);
        constexpr auto schema = mu::make_schema(origCodec);
        constexpr auto codec = mu::make_codec(schema);

        using val_t = typename std::decay_t<decltype(codec)>::value_type;
        auto fmt = mu::make_binary_format<char>(codec);

        val_t value{std::string{"test"}, std::uint64_t{123}};
        auto bytes = to_bytes(fmt, value);
        auto d = from_bytes(fmt, bytes);
        assert(d.has_value());
        assert(std::get<0>(*d) == "test");
        assert(std::get<1>(*d) == 123u);
    }

    // =====================================================================
    // 6. Variant codec from schema
    // =====================================================================

    // -- 6a: variant<int64, string> - int alternative
    {
        constexpr auto origCodec = mu::variant_codec(mu::int32_codec, mu::string_codec);
        constexpr auto schema = mu::make_schema(origCodec);
        constexpr auto codec = mu::make_codec(schema);

        using val_t = typename std::decay_t<decltype(codec)>::value_type;
        auto fmt = mu::make_binary_format<char>(codec);

        val_t v1 = std::int64_t{42};
        auto bytes = to_bytes(fmt, v1);
        auto d = from_bytes(fmt, bytes);
        assert(d.has_value());
        assert(d->index() == 0);
        assert(std::get<0>(*d) == 42);
    }
    // -- 6b: variant<int64, string> - string alternative
    {
        constexpr auto origCodec = mu::variant_codec(mu::int32_codec, mu::string_codec);
        constexpr auto schema = mu::make_schema(origCodec);
        constexpr auto codec = mu::make_codec(schema);

        using val_t = typename std::decay_t<decltype(codec)>::value_type;
        auto fmt = mu::make_binary_format<char>(codec);

        val_t v2 = std::string{"hello"};
        auto bytes = to_bytes(fmt, v2);
        auto d = from_bytes(fmt, bytes);
        assert(d.has_value());
        assert(d->index() == 1);
        assert(std::get<1>(*d) == "hello");
    }
    // -- 6c: variant<bool, double, string>
    {
        constexpr auto origCodec = mu::variant_codec(mu::bool_codec, mu::double_codec, mu::string_codec);
        constexpr auto schema = mu::make_schema(origCodec);
        constexpr auto codec = mu::make_codec(schema);

        using val_t = typename std::decay_t<decltype(codec)>::value_type;
        auto fmt = mu::make_binary_format<char>(codec);

        val_t v0 = true;
        auto bytes0 = to_bytes(fmt, v0);
        auto d0 = from_bytes(fmt, bytes0);
        assert(d0 && d0->index() == 0 && std::get<0>(*d0) == true);

        val_t v1 = 9.99;
        auto bytes1 = to_bytes(fmt, v1);
        auto d1 = from_bytes(fmt, bytes1);
        assert(d1 && d1->index() == 1 && std::get<1>(*d1) == 9.99);

        val_t v2 = std::string{"world"};
        auto bytes2 = to_bytes(fmt, v2);
        auto d2 = from_bytes(fmt, bytes2);
        assert(d2 && d2->index() == 2 && std::get<2>(*d2) == "world");
    }

    // =====================================================================
    // 7. Complex: profile with optional field
    // =====================================================================

    // -- 7a: profile with bio present
    {
        constexpr auto schema = mu::make_schema(profile::codec);
        constexpr auto codec = mu::make_codec(schema);

        using val_t = typename std::decay_t<decltype(codec)>::value_type;
        auto fmt = mu::make_binary_format<char>(codec);

        val_t v1{std::string{"alice"}, std::optional<std::string>{"A bio"}};
        auto bytes = to_bytes(fmt, v1);
        auto d = from_bytes(fmt, bytes);
        assert(d.has_value());
        assert(std::get<0>(*d) == "alice");
        assert(std::get<1>(*d).has_value());
        assert(*std::get<1>(*d) == "A bio");
    }
    // -- 7b: profile with bio absent
    {
        constexpr auto schema = mu::make_schema(profile::codec);
        constexpr auto codec = mu::make_codec(schema);

        using val_t = typename std::decay_t<decltype(codec)>::value_type;
        auto fmt = mu::make_binary_format<char>(codec);

        val_t v2{std::string{"bob"}, std::optional<std::string>{}};
        auto bytes = to_bytes(fmt, v2);
        auto d = from_bytes(fmt, bytes);
        assert(d.has_value());
        assert(std::get<0>(*d) == "bob");
        assert(!std::get<1>(*d).has_value());
    }

    // =====================================================================
    // 8. Nested compositions
    // =====================================================================

    // -- 8a: vector<optional<int64>>
    {
        constexpr auto origCodec = mu::vector_of(mu::optional_codec(mu::int32_codec));
        constexpr auto schema = mu::make_schema(origCodec);
        constexpr auto codec = mu::make_codec(schema);
        auto fmt = mu::make_binary_format<char>(codec);

        std::vector<std::optional<std::int64_t>> value = {1, std::nullopt, 3};
        auto bytes = to_bytes(fmt, value);
        auto d = from_bytes(fmt, bytes);
        assert(d && d->size() == 3);
        assert((*d)[0].has_value() && *(*d)[0] == 1);
        assert(!(*d)[1].has_value());
        assert((*d)[2].has_value() && *(*d)[2] == 3);
    }
    // -- 8b: vector of objects (vector of point-like tuples)
    {
        constexpr auto schema = mu::make_schema(mu::vector_of(point::codec));
        constexpr auto codec = mu::make_codec(schema);

        using inner_t = typename std::decay_t<decltype(codec)>::value_type::value_type;
        auto fmt = mu::make_binary_format<char>(codec);

        std::vector<inner_t> value = {
            inner_t{std::int64_t{1}, std::int64_t{2}},
            inner_t{std::int64_t{3}, std::int64_t{4}}
        };
        auto bytes = to_bytes(fmt, value);
        auto d = from_bytes(fmt, bytes);
        assert(d && d->size() == 2);
        assert(std::get<0>((*d)[0]) == 1 && std::get<1>((*d)[0]) == 2);
        assert(std::get<0>((*d)[1]) == 3 && std::get<1>((*d)[1]) == 4);
    }

    return 0;
}

