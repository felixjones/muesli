/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/schema/extract>
#include <muesli/codecs>

#include <cassert>
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mu = muesli;
namespace s = muesli::schema;

// -- Test structs -----------------------------------------------------------

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

struct located_person {
    int age;
    std::string name;
    point location;
    static constexpr auto codec = mu::tuple_codec(
        mu::int_codec.member<&located_person::age>().named("age"),
        mu::string_codec.member<&located_person::name>().named("name"),
        point::codec.member<&located_person::location>().named("location")
    ).apply<located_person>();
};

struct profile {
    std::string username;
    std::optional<std::string> bio;
    static constexpr auto codec = mu::tuple_codec(
        mu::string_codec.member<&profile::username>().named("username"),
        mu::optional_codec(mu::string_codec).member<&profile::bio>().named("bio")
    ).apply<profile>();
};

struct team {
    std::string name;
    std::vector<person> members;
    static constexpr auto codec = mu::tuple_codec(
        mu::string_codec.member<&team::name>().named("name"),
        mu::vector_of(person::codec).member<&team::members>().named("members")
    ).apply<team>();
};

// -- Additional test structs for complex coverage ---------------------------

struct color {
    uint8_t r, g, b, a;
    static constexpr auto codec = mu::tuple_codec(
        mu::uint8_codec.member<&color::r>().named("r"),
        mu::uint8_codec.member<&color::g>().named("g"),
        mu::uint8_codec.member<&color::b>().named("b"),
        mu::uint8_codec.member<&color::a>().named("a")
    ).apply<color>();
};

struct config {
    int32_t port;
    std::string host;
    bool verbose;
    double timeout;
    static constexpr auto codec = mu::tuple_codec(
        mu::int32_codec.member<&config::port>().named("port"),
        mu::string_codec.member<&config::host>().named("host"),
        mu::bool_codec.member<&config::verbose>().named("verbose"),
        mu::double_codec.member<&config::timeout>().named("timeout")
    ).apply<config>();
};

struct optional_point {
    std::optional<int> x;
    std::optional<int> y;
    static constexpr auto codec = mu::tuple_codec(
        mu::optional_codec(mu::int_codec).member<&optional_point::x>().named("x"),
        mu::optional_codec(mu::int_codec).member<&optional_point::y>().named("y")
    ).apply<optional_point>();
};

struct nested_teams {
    std::string league;
    std::vector<team> teams;
    static constexpr auto codec = mu::tuple_codec(
        mu::string_codec.member<&nested_teams::league>().named("league"),
        mu::vector_of(team::codec).member<&nested_teams::teams>().named("teams")
    ).apply<nested_teams>();
};

struct with_array_field {
    std::string label;
    std::array<int32_t, 4> values;
    static constexpr auto codec = mu::tuple_codec(
        mu::string_codec.member<&with_array_field::label>().named("label"),
        mu::array_of<4>(mu::int32_codec).member<&with_array_field::values>().named("values")
    ).apply<with_array_field>();
};

struct with_or_else_field {
    int32_t port;
    std::string name;
    static constexpr auto codec = mu::tuple_codec(
        mu::int32_codec
            .constrain([](int32_t v) { return v > 0 && v < 65536; })
            .or_else([] { return int32_t{8080}; })
            .member<&with_or_else_field::port>()
            .named("port"),
        mu::string_codec.member<&with_or_else_field::name>().named("name")
    ).apply<with_or_else_field>();
};

struct single_field {
    int32_t value;
    static constexpr auto codec = mu::tuple_codec(
        mu::int32_codec.member<&single_field::value>().named("value")
    ).apply<single_field>();
};

struct many_fields {
    int32_t a;
    int32_t b;
    int32_t c;
    int32_t d;
    int32_t e;
    std::string f;
    bool g;
    double h;
    static constexpr auto codec = mu::tuple_codec(
        mu::int32_codec.member<&many_fields::a>().named("a"),
        mu::int32_codec.member<&many_fields::b>().named("b"),
        mu::int32_codec.member<&many_fields::c>().named("c"),
        mu::int32_codec.member<&many_fields::d>().named("d"),
        mu::int32_codec.member<&many_fields::e>().named("e"),
        mu::string_codec.member<&many_fields::f>().named("f"),
        mu::bool_codec.member<&many_fields::g>().named("g"),
        mu::double_codec.member<&many_fields::h>().named("h")
    ).apply<many_fields>();
};

int main() {
    // =====================================================================
    // 1. Primitive schemas -- all identity codec types
    // =====================================================================

    // -- 1a: signed integers ---------------------------------------------
    {
        constexpr auto node = mu::make_schema(mu::int32_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::signed_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::int8_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::signed_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::int16_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::signed_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::int64_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::signed_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::int_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::signed_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::long_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::signed_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::long_long_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::signed_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::short_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::signed_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::ptrdiff_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::signed_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::intmax_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::signed_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::intptr_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::signed_integer);
    }

    // -- 1b: unsigned integers -------------------------------------------
    {
        constexpr auto node = mu::make_schema(mu::uint32_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::unsigned_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::uint8_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::unsigned_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::uint16_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::unsigned_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::uint64_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::unsigned_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::unsigned_int_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::unsigned_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::unsigned_long_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::unsigned_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::unsigned_long_long_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::unsigned_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::unsigned_short_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::unsigned_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::size_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::unsigned_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::uintmax_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::unsigned_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::uintptr_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::unsigned_integer);
    }

    // -- 1c: floating point ----------------------------------------------
    {
        constexpr auto node = mu::make_schema(mu::float_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::float_32);
    }
    {
        constexpr auto node = mu::make_schema(mu::double_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::float_64);
    }
    {
        constexpr auto node = mu::make_schema(mu::long_double_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::float_128);
    }

    // -- 1d: boolean -----------------------------------------------------
    {
        constexpr auto node = mu::make_schema(mu::bool_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::boolean);
    }

    // -- 1e: string types ------------------------------------------------
    {
        constexpr auto node = mu::make_schema(mu::string_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::string);
    }
    {
        constexpr auto node = mu::make_schema(mu::wstring_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::string);
    }
    {
        constexpr auto node = mu::make_schema(mu::u8string_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::string);
    }
    {
        constexpr auto node = mu::make_schema(mu::u16string_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::string);
    }
    {
        constexpr auto node = mu::make_schema(mu::u32string_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::string);
    }

    // -- 1f: monostate ---------------------------------------------------
    {
        constexpr auto node = mu::make_schema(mu::monostate_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::monostate);
    }

    // -- 1g: char types map to signed/unsigned integer --------------------
    {
        constexpr auto node = mu::make_schema(mu::char8_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::unsigned_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::char16_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::unsigned_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::char32_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::unsigned_integer);
    }
    {
        // char signedness is implementation-defined; just confirm it's a primitive
        constexpr auto node = mu::make_schema(mu::char_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
    }
    {
        constexpr auto node = mu::make_schema(mu::signed_char_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::signed_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::unsigned_char_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::unsigned_integer);
    }
    {
        constexpr auto node = mu::make_schema(mu::wchar_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(
            decltype(node)::kind == s::fundamental_kind::signed_integer ||
            decltype(node)::kind == s::fundamental_kind::unsigned_integer
        );
    }
    {
        constexpr auto node = mu::make_schema(mu::byte_codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::fundamental_kind::unsigned_integer);
    }

    // =====================================================================
    // 2. Optional schemas
    // =====================================================================

    // -- 2a: optional<int32_t> -------------------------------------------
    {
        constexpr auto node = mu::make_schema(mu::optional_codec(mu::int32_codec));
        static_assert(s::is_optional_v<decltype(node)>);
        static_assert(s::is_primitive_v<decltype(node.inner)>);
        static_assert(decltype(node.inner)::kind == s::primitive_kind::signed_integer);
    }
    // -- 2b: optional<string> --------------------------------------------
    {
        constexpr auto node = mu::make_schema(mu::optional_codec(mu::string_codec));
        static_assert(s::is_optional_v<decltype(node)>);
        static_assert(decltype(node.inner)::kind == s::primitive_kind::string);
    }
    // -- 2c: optional<double> --------------------------------------------
    {
        constexpr auto node = mu::make_schema(mu::optional_codec(mu::double_codec));
        static_assert(s::is_optional_v<decltype(node)>);
        static_assert(decltype(node.inner)::kind == s::primitive_kind::float_64);
    }
    // -- 2d: optional<bool> ----------------------------------------------
    {
        constexpr auto node = mu::make_schema(mu::optional_codec(mu::bool_codec));
        static_assert(s::is_optional_v<decltype(node)>);
        static_assert(decltype(node.inner)::kind == s::primitive_kind::boolean);
    }
    // -- 2e: optional<uint64_t> ------------------------------------------
    {
        constexpr auto node = mu::make_schema(mu::optional_codec(mu::uint64_codec));
        static_assert(s::is_optional_v<decltype(node)>);
        static_assert(decltype(node.inner)::kind == s::primitive_kind::unsigned_integer);
    }

    // =====================================================================
    // 3. Array (vector) schemas
    // =====================================================================

    // -- 3a: vector<int32_t> ---------------------------------------------
    {
        constexpr auto node = mu::make_schema(mu::vector_of(mu::int32_codec));
        static_assert(s::is_array_v<decltype(node)>);
        static_assert(s::is_primitive_v<decltype(node.element)>);
        static_assert(decltype(node.element)::kind == s::primitive_kind::signed_integer);
    }
    // -- 3b: vector<string> ----------------------------------------------
    {
        constexpr auto node = mu::make_schema(mu::vector_of(mu::string_codec));
        static_assert(s::is_array_v<decltype(node)>);
        static_assert(decltype(node.element)::kind == s::primitive_kind::string);
    }
    // -- 3c: vector<double> ----------------------------------------------
    {
        constexpr auto node = mu::make_schema(mu::vector_of(mu::double_codec));
        static_assert(s::is_array_v<decltype(node)>);
        static_assert(decltype(node.element)::kind == s::primitive_kind::float_64);
    }
    // -- 3d: vector<bool> ------------------------------------------------
    {
        constexpr auto node = mu::make_schema(mu::vector_of(mu::bool_codec));
        static_assert(s::is_array_v<decltype(node)>);
        static_assert(decltype(node.element)::kind == s::primitive_kind::boolean);
    }
    // -- 3e: fixed-size array_of<3>(int32_codec) -------------------------
    {
        constexpr auto node = mu::make_schema(mu::array_of<3>(mu::int32_codec));
        static_assert(s::is_array_v<decltype(node)>);
        static_assert(s::is_primitive_v<decltype(node.element)>);
        static_assert(decltype(node.element)::kind == s::primitive_kind::signed_integer);
    }
    // -- 3f: fixed-size array_of<5>(string_codec) ------------------------
    {
        constexpr auto node = mu::make_schema(mu::array_of<5>(mu::string_codec));
        static_assert(s::is_array_v<decltype(node)>);
        static_assert(decltype(node.element)::kind == s::primitive_kind::string);
    }
    // -- 3g: fixed-size array_of<1>(float_codec) -- degenerate -----------
    {
        constexpr auto node = mu::make_schema(mu::array_of<1>(mu::float_codec));
        static_assert(s::is_array_v<decltype(node)>);
        static_assert(decltype(node.element)::kind == s::primitive_kind::float_32);
    }
    // -- 3h: nested vector -- vector<vector<int32_t>> --------------------
    {
        constexpr auto node = mu::make_schema(mu::vector_of(mu::vector_of(mu::int32_codec)));
        static_assert(s::is_array_v<decltype(node)>);
        static_assert(s::is_array_v<decltype(node.element)>);
        static_assert(s::is_primitive_v<decltype(node.element.element)>);
        static_assert(decltype(node.element.element)::kind == s::primitive_kind::signed_integer);
    }
    // -- 3i: vector<optional<int32_t>> -----------------------------------
    {
        constexpr auto node = mu::make_schema(mu::vector_of(mu::optional_codec(mu::int32_codec)));
        static_assert(s::is_array_v<decltype(node)>);
        static_assert(s::is_optional_v<decltype(node.element)>);
        static_assert(decltype(node.element.inner)::kind == s::primitive_kind::signed_integer);
    }
    // -- 3j: vector of objects (vector<person>) --------------------------
    {
        constexpr auto node = mu::make_schema(mu::vector_of(person::codec));
        static_assert(s::is_array_v<decltype(node)>);
        static_assert(s::is_object_v<decltype(node.element)>);
        static_assert(s::field_count_v<std::remove_cvref_t<decltype(node.element)>> == 2);
        static_assert(std::get<0>(node.element.fields).name == "age");
        static_assert(std::get<1>(node.element.fields).name == "name");
    }

    // =====================================================================
    // 4. Object schemas (named members)
    // =====================================================================

    // -- 4a: point (2 int fields) ----------------------------------------
    {
        constexpr auto node = mu::make_schema(point::codec);
        static_assert(s::is_object_v<decltype(node)>);
        static_assert(s::field_count_v<decltype(node)> == 2);
        static_assert(std::get<0>(node.fields).name == "x");
        static_assert(std::get<0>(node.fields).required);
        static_assert(s::is_primitive_v<decltype(std::get<0>(node.fields).schema)>);
        static_assert(decltype(std::get<0>(node.fields).schema)::kind == s::primitive_kind::signed_integer);
        static_assert(std::get<1>(node.fields).name == "y");
        static_assert(std::get<1>(node.fields).required);
    }
    // -- 4b: person (int + string) ---------------------------------------
    {
        constexpr auto node = mu::make_schema(person::codec);
        static_assert(s::is_object_v<decltype(node)>);
        static_assert(s::field_count_v<decltype(node)> == 2);
        static_assert(std::get<0>(node.fields).name == "age");
        static_assert(decltype(std::get<0>(node.fields).schema)::kind == s::primitive_kind::signed_integer);
        static_assert(std::get<1>(node.fields).name == "name");
        static_assert(decltype(std::get<1>(node.fields).schema)::kind == s::primitive_kind::string);
    }
    // -- 4c: profile (optional member) -----------------------------------
    {
        constexpr auto node = mu::make_schema(profile::codec);
        static_assert(s::is_object_v<decltype(node)>);
        static_assert(s::field_count_v<decltype(node)> == 2);

        // username: required string
        static_assert(std::get<0>(node.fields).name == "username");
        static_assert(std::get<0>(node.fields).required);
        static_assert(s::is_primitive_v<decltype(std::get<0>(node.fields).schema)>);

        // bio: optional string (not required)
        static_assert(std::get<1>(node.fields).name == "bio");
        static_assert(!std::get<1>(node.fields).required);
        static_assert(s::is_optional_v<decltype(std::get<1>(node.fields).schema)>);
        static_assert(decltype(std::get<1>(node.fields).schema.inner)::kind == s::primitive_kind::string);
    }
    // -- 4d: nested object (located_person) ------------------------------
    {
        constexpr auto node = mu::make_schema(located_person::codec);
        static_assert(s::is_object_v<decltype(node)>);
        static_assert(s::field_count_v<decltype(node)> == 3);

        static_assert(std::get<0>(node.fields).name == "age");
        static_assert(std::get<1>(node.fields).name == "name");

        // location: nested object
        static_assert(std::get<2>(node.fields).name == "location");
        static_assert(std::get<2>(node.fields).required);
        static_assert(s::is_object_v<decltype(std::get<2>(node.fields).schema)>);
        static_assert(s::field_count_v<std::remove_cvref_t<decltype(std::get<2>(node.fields).schema)>> == 2);
        static_assert(std::get<0>(std::get<2>(node.fields).schema.fields).name == "x");
        static_assert(std::get<1>(std::get<2>(node.fields).schema.fields).name == "y");
    }
    // -- 4e: team (vector of nested objects) -----------------------------
    {
        constexpr auto node = mu::make_schema(team::codec);
        static_assert(s::is_object_v<decltype(node)>);
        static_assert(s::field_count_v<decltype(node)> == 2);

        // name: string
        static_assert(std::get<0>(node.fields).name == "name");
        static_assert(s::is_primitive_v<decltype(std::get<0>(node.fields).schema)>);

        // members: array of person objects
        static_assert(std::get<1>(node.fields).name == "members");
        static_assert(std::get<1>(node.fields).required);
        static_assert(s::is_array_v<decltype(std::get<1>(node.fields).schema)>);
        static_assert(s::is_object_v<decltype(std::get<1>(node.fields).schema.element)>);
        static_assert(s::field_count_v<std::remove_cvref_t<decltype(std::get<1>(node.fields).schema.element)>> == 2);
        static_assert(std::get<0>(std::get<1>(node.fields).schema.element.fields).name == "age");
        static_assert(std::get<1>(std::get<1>(node.fields).schema.element.fields).name == "name");
    }
    // -- 4f: color (4 uint8 fields) --------------------------------------
    {
        constexpr auto node = mu::make_schema(color::codec);
        static_assert(s::is_object_v<decltype(node)>);
        static_assert(s::field_count_v<decltype(node)> == 4);
        static_assert(std::get<0>(node.fields).name == "r");
        static_assert(std::get<1>(node.fields).name == "g");
        static_assert(std::get<2>(node.fields).name == "b");
        static_assert(std::get<3>(node.fields).name == "a");
        static_assert(decltype(std::get<0>(node.fields).schema)::kind == s::primitive_kind::unsigned_integer);
        static_assert(decltype(std::get<3>(node.fields).schema)::kind == s::primitive_kind::unsigned_integer);
    }
    // -- 4g: config (int32 + string + bool + double) ---------------------
    {
        constexpr auto node = mu::make_schema(config::codec);
        static_assert(s::is_object_v<decltype(node)>);
        static_assert(s::field_count_v<decltype(node)> == 4);
        static_assert(std::get<0>(node.fields).name == "port");
        static_assert(decltype(std::get<0>(node.fields).schema)::kind == s::primitive_kind::signed_integer);
        static_assert(std::get<1>(node.fields).name == "host");
        static_assert(decltype(std::get<1>(node.fields).schema)::kind == s::primitive_kind::string);
        static_assert(std::get<2>(node.fields).name == "verbose");
        static_assert(decltype(std::get<2>(node.fields).schema)::kind == s::primitive_kind::boolean);
        static_assert(std::get<3>(node.fields).name == "timeout");
        static_assert(decltype(std::get<3>(node.fields).schema)::kind == s::primitive_kind::float_64);
    }
    // -- 4h: all-optional fields -----------------------------------------
    {
        constexpr auto node = mu::make_schema(optional_point::codec);
        static_assert(s::is_object_v<decltype(node)>);
        static_assert(s::field_count_v<decltype(node)> == 2);
        static_assert(!std::get<0>(node.fields).required);
        static_assert(!std::get<1>(node.fields).required);
        static_assert(s::is_optional_v<decltype(std::get<0>(node.fields).schema)>);
        static_assert(s::is_optional_v<decltype(std::get<1>(node.fields).schema)>);
    }
    // -- 4i: single field ------------------------------------------------
    {
        constexpr auto node = mu::make_schema(single_field::codec);
        static_assert(s::is_object_v<decltype(node)>);
        static_assert(s::field_count_v<decltype(node)> == 1);
        static_assert(std::get<0>(node.fields).name == "value");
        static_assert(std::get<0>(node.fields).required);
    }
    // -- 4j: many fields (8) ---------------------------------------------
    {
        constexpr auto node = mu::make_schema(many_fields::codec);
        static_assert(s::is_object_v<decltype(node)>);
        static_assert(s::field_count_v<decltype(node)> == 8);
        static_assert(std::get<0>(node.fields).name == "a");
        static_assert(std::get<5>(node.fields).name == "f");
        static_assert(decltype(std::get<5>(node.fields).schema)::kind == s::primitive_kind::string);
        static_assert(std::get<6>(node.fields).name == "g");
        static_assert(decltype(std::get<6>(node.fields).schema)::kind == s::primitive_kind::boolean);
        static_assert(std::get<7>(node.fields).name == "h");
        static_assert(decltype(std::get<7>(node.fields).schema)::kind == s::primitive_kind::float_64);
    }
    // -- 4k: object with array field (fixed-size std::array) -------------
    {
        constexpr auto node = mu::make_schema(with_array_field::codec);
        static_assert(s::is_object_v<decltype(node)>);
        static_assert(s::field_count_v<decltype(node)> == 2);
        static_assert(std::get<0>(node.fields).name == "label");
        static_assert(decltype(std::get<0>(node.fields).schema)::kind == s::primitive_kind::string);
        static_assert(std::get<1>(node.fields).name == "values");
        static_assert(s::is_array_v<decltype(std::get<1>(node.fields).schema)>);
        static_assert(decltype(std::get<1>(node.fields).schema.element)::kind == s::primitive_kind::signed_integer);
    }
    // -- 4l: object with or_else field (transparent pass-through) --------
    {
        constexpr auto node = mu::make_schema(with_or_else_field::codec);
        static_assert(s::is_object_v<decltype(node)>);
        static_assert(s::field_count_v<decltype(node)> == 2);
        static_assert(std::get<0>(node.fields).name == "port");
        static_assert(std::get<0>(node.fields).required);
        // or_else + constrained passes through to the underlying int32 schema
        static_assert(s::is_primitive_v<decltype(std::get<0>(node.fields).schema)>);
        static_assert(decltype(std::get<0>(node.fields).schema)::kind == s::primitive_kind::signed_integer);
        static_assert(std::get<1>(node.fields).name == "name");
    }
    // -- 4m: deeply nested -- nested_teams (league + vector<team>) ------
    {
        constexpr auto node = mu::make_schema(nested_teams::codec);
        static_assert(s::is_object_v<decltype(node)>);
        static_assert(s::field_count_v<decltype(node)> == 2);
        static_assert(std::get<0>(node.fields).name == "league");
        static_assert(decltype(std::get<0>(node.fields).schema)::kind == s::primitive_kind::string);
        static_assert(std::get<1>(node.fields).name == "teams");
        static_assert(s::is_array_v<decltype(std::get<1>(node.fields).schema)>);
        // array element is team object
        static_assert(s::is_object_v<decltype(std::get<1>(node.fields).schema.element)>);
        static_assert(s::field_count_v<std::remove_cvref_t<decltype(std::get<1>(node.fields).schema.element)>> == 2);
        // team.name is string
        static_assert(std::get<0>(std::get<1>(node.fields).schema.element.fields).name == "name");
        // team.members is array of person objects
        static_assert(std::get<1>(std::get<1>(node.fields).schema.element.fields).name == "members");
        static_assert(s::is_array_v<decltype(std::get<1>(std::get<1>(node.fields).schema.element.fields).schema)>);
        // person has 2 fields
        static_assert(s::is_object_v<
            decltype(std::get<1>(std::get<1>(node.fields).schema.element.fields).schema.element)>);
    }

    // =====================================================================
    // 5. Positional tuple schemas
    // =====================================================================

    // -- 5a: tuple<int32, float, bool> -----------------------------------
    {
        constexpr auto codec = mu::tuple_codec(mu::int32_codec, mu::float_codec, mu::bool_codec);
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_tuple_v<decltype(node)>);
        static_assert(s::element_count_v<decltype(node)> == 3);
        static_assert(std::remove_cvref_t<decltype(std::get<0>(node.elements))>::kind == s::primitive_kind::signed_integer);
        static_assert(std::remove_cvref_t<decltype(std::get<1>(node.elements))>::kind == s::primitive_kind::float_32);
        static_assert(std::remove_cvref_t<decltype(std::get<2>(node.elements))>::kind == s::primitive_kind::boolean);
    }
    // -- 5b: tuple<string, string> (2-element) ---------------------------
    {
        constexpr auto codec = mu::tuple_codec(mu::string_codec, mu::string_codec);
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_tuple_v<decltype(node)>);
        static_assert(s::element_count_v<decltype(node)> == 2);
        static_assert(std::remove_cvref_t<decltype(std::get<0>(node.elements))>::kind == s::primitive_kind::string);
        static_assert(std::remove_cvref_t<decltype(std::get<1>(node.elements))>::kind == s::primitive_kind::string);
    }
    // -- 5c: single-element tuple ----------------------------------------
    {
        constexpr auto codec = mu::tuple_codec(mu::uint64_codec);
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_tuple_v<decltype(node)>);
        static_assert(s::element_count_v<decltype(node)> == 1);
    }
    // -- 5d: tuple of all primitive kinds --------------------------------
    {
        constexpr auto codec = mu::tuple_codec(
            mu::int32_codec, mu::uint32_codec, mu::float_codec,
            mu::bool_codec, mu::string_codec, mu::monostate_codec
        );
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_tuple_v<decltype(node)>);
        static_assert(s::element_count_v<decltype(node)> == 6);
        static_assert(std::remove_cvref_t<decltype(std::get<0>(node.elements))>::kind == s::primitive_kind::signed_integer);
        static_assert(std::remove_cvref_t<decltype(std::get<1>(node.elements))>::kind == s::primitive_kind::unsigned_integer);
        static_assert(std::remove_cvref_t<decltype(std::get<2>(node.elements))>::kind == s::primitive_kind::float_32);
        static_assert(std::remove_cvref_t<decltype(std::get<3>(node.elements))>::kind == s::primitive_kind::boolean);
        static_assert(std::remove_cvref_t<decltype(std::get<4>(node.elements))>::kind == s::primitive_kind::string);
        static_assert(std::remove_cvref_t<decltype(std::get<5>(node.elements))>::kind == s::primitive_kind::monostate);
    }
    // -- 5e: nested positional tuple -- tuple<tuple<int, float>, string> -
    {
        constexpr auto inner = mu::tuple_codec(mu::int32_codec, mu::float_codec);
        constexpr auto codec = mu::tuple_codec(inner, mu::string_codec);
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_tuple_v<decltype(node)>);
        static_assert(s::element_count_v<decltype(node)> == 2);
        // first element is itself a tuple
        static_assert(s::is_tuple_v<std::remove_cvref_t<decltype(std::get<0>(node.elements))>>);
        static_assert(s::element_count_v<std::remove_cvref_t<decltype(std::get<0>(node.elements))>> == 2);
        static_assert(std::remove_cvref_t<decltype(std::get<1>(node.elements))>::kind == s::primitive_kind::string);
    }

    // =====================================================================
    // 6. Variant schemas
    // =====================================================================

    // -- 6a: variant<int32, string> (2 alternatives) ---------------------
    {
        constexpr auto codec = mu::variant_codec(mu::int32_codec, mu::string_codec);
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_variant_v<decltype(node)>);
        static_assert(s::alternative_count_v<decltype(node)> == 2);
        static_assert(std::remove_cvref_t<decltype(std::get<0>(node.alternatives))>::kind == s::primitive_kind::signed_integer);
        static_assert(std::remove_cvref_t<decltype(std::get<1>(node.alternatives))>::kind == s::primitive_kind::string);
    }
    // -- 6b: variant<int32, bool, string> (3 alternatives) ---------------
    {
        constexpr auto codec = mu::variant_codec(mu::int32_codec, mu::bool_codec, mu::string_codec);
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_variant_v<decltype(node)>);
        static_assert(s::alternative_count_v<decltype(node)> == 3);
        static_assert(std::remove_cvref_t<decltype(std::get<0>(node.alternatives))>::kind == s::primitive_kind::signed_integer);
        static_assert(std::remove_cvref_t<decltype(std::get<1>(node.alternatives))>::kind == s::primitive_kind::boolean);
        static_assert(std::remove_cvref_t<decltype(std::get<2>(node.alternatives))>::kind == s::primitive_kind::string);
    }
    // -- 6c: variant<monostate, int32> (optional-like) -------------------
    {
        constexpr auto codec = mu::variant_codec(mu::monostate_codec, mu::int32_codec);
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_variant_v<decltype(node)>);
        static_assert(s::alternative_count_v<decltype(node)> == 2);
        static_assert(std::remove_cvref_t<decltype(std::get<0>(node.alternatives))>::kind == s::primitive_kind::monostate);
        static_assert(std::remove_cvref_t<decltype(std::get<1>(node.alternatives))>::kind == s::primitive_kind::signed_integer);
    }
    // -- 6d: variant containing objects ----------------------------------
    {
        constexpr auto codec = mu::variant_codec(point::codec, person::codec);
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_variant_v<decltype(node)>);
        static_assert(s::alternative_count_v<decltype(node)> == 2);
        static_assert(s::is_object_v<std::remove_cvref_t<decltype(std::get<0>(node.alternatives))>>);
        static_assert(s::field_count_v<std::remove_cvref_t<decltype(std::get<0>(node.alternatives))>> == 2);
        static_assert(s::is_object_v<std::remove_cvref_t<decltype(std::get<1>(node.alternatives))>>);
        static_assert(s::field_count_v<std::remove_cvref_t<decltype(std::get<1>(node.alternatives))>> == 2);
    }
    // -- 6e: variant containing different composite types ----------------
    {
        constexpr auto codec = mu::variant_codec(
            mu::int32_codec,
            mu::string_codec,
            mu::vector_of(mu::int32_codec)
        );
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_variant_v<decltype(node)>);
        static_assert(s::alternative_count_v<decltype(node)> == 3);
        static_assert(s::is_primitive_v<std::remove_cvref_t<decltype(std::get<0>(node.alternatives))>>);
        static_assert(s::is_primitive_v<std::remove_cvref_t<decltype(std::get<1>(node.alternatives))>>);
        static_assert(s::is_array_v<std::remove_cvref_t<decltype(std::get<2>(node.alternatives))>>);
    }
    // -- 6f: variant<monostate, string, int32, double, bool> (many) ------
    {
        constexpr auto codec = mu::variant_codec(
            mu::monostate_codec, mu::string_codec, mu::int32_codec,
            mu::double_codec, mu::bool_codec
        );
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_variant_v<decltype(node)>);
        static_assert(s::alternative_count_v<decltype(node)> == 5);
    }

    // =====================================================================
    // 7. Transparent codec pass-through
    // =====================================================================

    // -- 7a: constrained int ---------------------------------------------
    {
        constexpr auto codec = mu::int32_codec.constrain([](int32_t v) { return v > 0; });
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::signed_integer);
    }
    // -- 7b: constrained string ------------------------------------------
    {
        constexpr auto codec = mu::string_codec.constrain(
            [](const std::string& s) { return !s.empty(); });
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::string);
    }
    // -- 7c: constrained double ------------------------------------------
    {
        constexpr auto codec = mu::double_codec.constrain(
            [](double v) { return v >= 0.0 && v <= 1.0; });
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::float_64);
    }
    // -- 7d: or_else on int (constrained + fallback) ---------------------
    {
        constexpr auto codec = mu::int32_codec
            .constrain([](int32_t v) { return v > 0; })
            .or_else([] { return int32_t{1}; });
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::signed_integer);
    }
    // -- 7e: or_else on string (standalone) ------------------------------
    {
        constexpr auto codec = mu::string_codec
            .or_else([] { return std::string{"default"}; });
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_primitive_v<decltype(node)>);
        static_assert(decltype(node)::kind == s::primitive_kind::string);
    }
    // -- 7f: constrained vector ------------------------------------------
    {
        constexpr auto codec = mu::vector_of(mu::int32_codec).constrain(
            [](const std::vector<int32_t>& v) { return v.size() <= 100; });
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_array_v<decltype(node)>);
        static_assert(decltype(node.element)::kind == s::primitive_kind::signed_integer);
    }
    // -- 7g: constrained optional ----------------------------------------
    {
        constexpr auto codec = mu::optional_codec(mu::int32_codec).constrain(
            [](const std::optional<int32_t>& v) { return true; });
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_optional_v<decltype(node)>);
        static_assert(decltype(node.inner)::kind == s::primitive_kind::signed_integer);
    }

    // =====================================================================
    // 8. Complex composition (cross-cutting)
    // =====================================================================

    // -- 8a: optional of vector ------------------------------------------
    {
        constexpr auto node = mu::make_schema(mu::optional_codec(mu::vector_of(mu::int32_codec)));
        static_assert(s::is_optional_v<decltype(node)>);
        static_assert(s::is_array_v<decltype(node.inner)>);
        static_assert(decltype(node.inner.element)::kind == s::primitive_kind::signed_integer);
    }
    // -- 8b: optional of object ------------------------------------------
    {
        constexpr auto node = mu::make_schema(mu::optional_codec(point::codec));
        static_assert(s::is_optional_v<decltype(node)>);
        static_assert(s::is_object_v<decltype(node.inner)>);
        static_assert(s::field_count_v<std::remove_cvref_t<decltype(node.inner)>> == 2);
        static_assert(std::get<0>(node.inner.fields).name == "x");
    }
    // -- 8c: vector of vectors of strings --------------------------------
    {
        constexpr auto node = mu::make_schema(
            mu::vector_of(mu::vector_of(mu::string_codec)));
        static_assert(s::is_array_v<decltype(node)>);
        static_assert(s::is_array_v<decltype(node.element)>);
        static_assert(decltype(node.element.element)::kind == s::primitive_kind::string);
    }
    // -- 8d: vector of optional strings ----------------------------------
    {
        constexpr auto node = mu::make_schema(
            mu::vector_of(mu::optional_codec(mu::string_codec)));
        static_assert(s::is_array_v<decltype(node)>);
        static_assert(s::is_optional_v<decltype(node.element)>);
        static_assert(decltype(node.element.inner)::kind == s::primitive_kind::string);
    }
    // -- 8e: fixed array of objects (array_of<2>(point::codec)) ----------
    {
        constexpr auto node = mu::make_schema(mu::array_of<2>(point::codec));
        static_assert(s::is_array_v<decltype(node)>);
        static_assert(s::is_object_v<decltype(node.element)>);
        static_assert(s::field_count_v<std::remove_cvref_t<decltype(node.element)>> == 2);
    }
    // -- 8f: tuple containing a variant ----------------------------------
    {
        constexpr auto var = mu::variant_codec(mu::int32_codec, mu::string_codec);
        constexpr auto codec = mu::tuple_codec(mu::bool_codec, var);
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_tuple_v<decltype(node)>);
        static_assert(s::element_count_v<decltype(node)> == 2);
        static_assert(std::remove_cvref_t<decltype(std::get<0>(node.elements))>::kind == s::primitive_kind::boolean);
        static_assert(s::is_variant_v<std::remove_cvref_t<decltype(std::get<1>(node.elements))>>);
        static_assert(s::alternative_count_v<std::remove_cvref_t<decltype(std::get<1>(node.elements))>> == 2);
    }
    // -- 8g: tuple containing a tuple ------------------------------------
    {
        constexpr auto inner = mu::tuple_codec(mu::int32_codec, mu::float_codec);
        constexpr auto codec = mu::tuple_codec(mu::string_codec, inner, mu::bool_codec);
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_tuple_v<decltype(node)>);
        static_assert(s::element_count_v<decltype(node)> == 3);
        static_assert(std::remove_cvref_t<decltype(std::get<0>(node.elements))>::kind == s::primitive_kind::string);
        static_assert(s::is_tuple_v<std::remove_cvref_t<decltype(std::get<1>(node.elements))>>);
        static_assert(std::remove_cvref_t<decltype(std::get<2>(node.elements))>::kind == s::primitive_kind::boolean);
    }
    // -- 8h: variant containing optionals --------------------------------
    {
        constexpr auto codec = mu::variant_codec(
            mu::optional_codec(mu::int32_codec),
            mu::string_codec
        );
        constexpr auto node = mu::make_schema(codec);
        static_assert(s::is_variant_v<decltype(node)>);
        static_assert(s::alternative_count_v<decltype(node)> == 2);
        static_assert(s::is_optional_v<std::remove_cvref_t<decltype(std::get<0>(node.alternatives))>>);
        static_assert(s::is_primitive_v<std::remove_cvref_t<decltype(std::get<1>(node.alternatives))>>);
    }
    // -- 8i: optional containing optional (nested optional) --------------
#if !defined(_MSC_VER)
    {
        constexpr auto node = mu::make_schema(
            mu::optional_codec(mu::optional_codec(mu::int32_codec)));
        static_assert(s::is_optional_v<decltype(node)>);
        static_assert(s::is_optional_v<decltype(node.inner)>);
        static_assert(decltype(node.inner.inner)::kind == s::primitive_kind::signed_integer);
    }
#endif

    return 0;
}


