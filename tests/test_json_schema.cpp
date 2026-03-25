/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/format/nlohmann_json_backend>
#include <muesli/schema/extract>
#include <muesli/schema/json_schema>
#include <muesli/codecs>

#include <cassert>
#include <optional>
#include <string>
#include <vector>

using B = muesli::nlohmann_json_backend;
namespace mu = muesli;

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

int main() {
    {
        constexpr auto s = mu::make_schema(mu::bool_codec);
        auto j = mu::to_json_schema<B>(s);
        assert(j["type"] == "boolean");
    }
    {
        constexpr auto s = mu::make_schema(mu::int32_codec);
        auto j = mu::to_json_schema<B>(s);
        assert(j["type"] == "integer");
        assert(!j.contains("minimum"));
    }
    {
        constexpr auto s = mu::make_schema(mu::uint32_codec);
        auto j = mu::to_json_schema<B>(s);
        assert(j["type"] == "integer");
        assert(j.contains("minimum"));
        assert(j["minimum"] == 0);
    }
    {
        constexpr auto s = mu::make_schema(mu::float_codec);
        auto j = mu::to_json_schema<B>(s);
        assert(j["type"] == "number");
    }
    {
        constexpr auto s = mu::make_schema(mu::double_codec);
        auto j = mu::to_json_schema<B>(s);
        assert(j["type"] == "number");
    }
    {
        constexpr auto s = mu::make_schema(mu::string_codec);
        auto j = mu::to_json_schema<B>(s);
        assert(j["type"] == "string");
    }
    {
        constexpr auto s = mu::make_schema(mu::monostate_codec);
        auto j = mu::to_json_schema<B>(s);
        assert(j["type"] == "null");
    }
    {
        constexpr auto s = mu::make_schema(mu::optional_codec(mu::int32_codec));
        auto j = mu::to_json_schema<B>(s);
        assert(j.contains("oneOf"));
        assert(j["oneOf"].is_array());
        assert(j["oneOf"].size() == 2);
        assert(j["oneOf"][0]["type"] == "integer");
        assert(j["oneOf"][1]["type"] == "null");
    }
    {
        constexpr auto s = mu::make_schema(mu::vector_of(mu::int32_codec));
        auto j = mu::to_json_schema<B>(s);
        assert(j["type"] == "array");
        assert(j.contains("items"));
        assert(j["items"]["type"] == "integer");
    }
    {
        constexpr auto s = mu::make_schema(mu::vector_of(mu::string_codec));
        auto j = mu::to_json_schema<B>(s);
        assert(j["type"] == "array");
        assert(j["items"]["type"] == "string");
    }
    {
        constexpr auto s = mu::make_schema(point::codec);
        auto j = mu::to_json_schema<B>(s);
        assert(j["type"] == "object");
        assert(j["properties"]["x"]["type"] == "integer");
        assert(j["properties"]["y"]["type"] == "integer");
        assert(j["required"].size() == 2);
    }
    {
        constexpr auto s = mu::make_schema(person::codec);
        auto j = mu::to_json_schema<B>(s);
        assert(j["properties"]["age"]["type"] == "integer");
        assert(j["properties"]["name"]["type"] == "string");
        assert(j["required"].size() == 2);
    }
    {
        constexpr auto s = mu::make_schema(profile::codec);
        auto j = mu::to_json_schema<B>(s);
        assert(j["properties"].contains("username"));
        assert(j["properties"].contains("bio"));
        assert(j["properties"]["bio"]["type"] == "string");
        assert(j["required"].size() == 1);
        assert(j["required"][0] == "username");
    }
    {
        constexpr auto s = mu::make_schema(team::codec);
        auto j = mu::to_json_schema<B>(s);
        assert(j["properties"]["name"]["type"] == "string");
        assert(j["properties"]["members"]["type"] == "array");
        assert(j["properties"]["members"]["items"]["type"] == "object");
        assert(j["properties"]["members"]["items"]["properties"].contains("age"));
        assert(j["properties"]["members"]["items"]["properties"].contains("name"));
    }
    {
        constexpr auto codec = mu::tuple_codec(mu::int32_codec, mu::float_codec, mu::bool_codec);
        constexpr auto s = mu::make_schema(codec);
        auto j = mu::to_json_schema<B>(s);
        assert(j["type"] == "array");
        assert(j.contains("prefixItems"));
        assert(j["prefixItems"].size() == 3);
        assert(j["prefixItems"][0]["type"] == "integer");
        assert(j["prefixItems"][1]["type"] == "number");
        assert(j["prefixItems"][2]["type"] == "boolean");
        assert(j["items"].is_boolean());
        assert(j["items"] == false);
    }
    {
        constexpr auto codec = mu::variant_codec(mu::int32_codec, mu::string_codec);
        constexpr auto s = mu::make_schema(codec);
        auto j = mu::to_json_schema<B>(s);
        assert(j.contains("oneOf"));
        assert(j["oneOf"].size() == 2);
        assert(j["oneOf"][0]["type"] == "integer");
        assert(j["oneOf"][1]["type"] == "string");
    }
    {
        constexpr auto codec = mu::variant_codec(mu::monostate_codec, mu::int32_codec);
        constexpr auto s = mu::make_schema(codec);
        auto j = mu::to_json_schema<B>(s);
        assert(j["oneOf"].size() == 2);
        assert(j["oneOf"][0]["type"] == "null");
        assert(j["oneOf"][1]["type"] == "integer");
    }
    {
        constexpr auto codec = mu::variant_codec(point::codec, person::codec);
        constexpr auto s = mu::make_schema(codec);
        auto j = mu::to_json_schema<B>(s);
        assert(j["oneOf"].size() == 2);
        assert(j["oneOf"][0]["type"] == "object");
        assert(j["oneOf"][1]["type"] == "object");
    }
    {
        constexpr auto s = mu::make_schema(mu::vector_of(mu::vector_of(mu::int32_codec)));
        auto j = mu::to_json_schema<B>(s);
        assert(j["type"] == "array");
        assert(j["items"]["type"] == "array");
        assert(j["items"]["items"]["type"] == "integer");
    }
    {
        constexpr auto s = mu::make_schema(mu::optional_codec(mu::vector_of(mu::string_codec)));
        auto j = mu::to_json_schema<B>(s);
        assert(j["oneOf"].size() == 2);
        assert(j["oneOf"][0]["type"] == "array");
        assert(j["oneOf"][1]["type"] == "null");
    }

    return 0;
}

