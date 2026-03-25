/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/format/json_format>
#include <muesli/format/nlohmann_json_backend>
#include <muesli/codecs>

#include <cassert>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

using B = muesli::nlohmann_json_backend;
namespace mu = muesli;

static std::string to_json(const nlohmann::json& j) {
    return j.dump();
}

// -- Test structs --------------------------------------------------------

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

// Struct with optional named member
struct profile {
    std::string username;
    std::optional<std::string> bio;

    static constexpr auto codec = mu::tuple_codec(
        mu::string_codec.member<&profile::username>().named("username"),
        mu::optional_codec(mu::string_codec).member<&profile::bio>().named("bio")
    ).apply<profile>();
};

// Struct with vector named member
struct team {
    std::string name;
    std::vector<person> members;

    static constexpr auto codec = mu::tuple_codec(
        mu::string_codec.member<&team::name>().named("name"),
        mu::vector_of(person::codec).member<&team::members>().named("members")
    ).apply<team>();
};

int main() {
    // -- 1: all-named point as JSON object -----------------------------
    {
        auto fmt = mu::make_json_format<B>(point::codec);
        point p{10, 20};
        auto j = fmt.serialize(p);
        assert(j.is_object());
        assert(j["x"] == 10);
        assert(j["y"] == 20);
        auto obj = fmt.deserialize(j);
        assert(obj && obj->x == 10 && obj->y == 20);
    }
    // -- 2: all-named person as JSON object ----------------------------
    {
        auto fmt = mu::make_json_format<B>(person::codec);
        person p{30, "Alice"};
        auto j = fmt.serialize(p);
        assert(j.is_object());
        assert(j["age"] == 30);
        assert(j["name"] == "Alice");
        auto obj = fmt.deserialize(j);
        assert(obj && obj->age == 30 && obj->name == "Alice");
    }
    // -- 3: nested named objects ---------------------------------------
    {
        auto fmt = mu::make_json_format<B>(located_person::codec);
        located_person p{25, "Bob", {3, 4}};
        auto j = fmt.serialize(p);
        assert(j.is_object());
        assert(j["age"] == 25);
        assert(j["name"] == "Bob");
        assert(j["location"].is_object());
        assert(j["location"]["x"] == 3);
        assert(j["location"]["y"] == 4);
        auto obj = fmt.deserialize(j);
        assert(obj && obj->age == 25 && obj->name == "Bob");
        assert(obj->location.x == 3 && obj->location.y == 4);
    }
    // -- 4: deserialization from JSON string ----------------------------
    {
        auto fmt = mu::make_json_format<B>(point::codec);
        auto j = nlohmann::json::parse("{\"x\":10,\"y\":20}");
        auto obj = fmt.deserialize(j);
        assert(obj && obj->x == 10 && obj->y == 20);
    }
    // -- 5: deserialization with reordered keys ------------------------
    {
        auto fmt = mu::make_json_format<B>(located_person::codec);
        auto j = nlohmann::json::parse(
            "{\"location\":{\"y\":20,\"x\":10},\"name\":\"Alice\",\"age\":30}");
        auto obj = fmt.deserialize(j);
        assert(obj && obj->age == 30 && obj->name == "Alice");
        assert(obj->location.x == 10 && obj->location.y == 20);
    }
    // -- 6: named struct with optional member (present) ----------------
    {
        auto fmt = mu::make_json_format<B>(profile::codec);
        profile p{"alice", "Hello world"};
        auto j = fmt.serialize(p);
        assert(j.is_object());
        assert(j["username"] == "alice");
        assert(j["bio"] == "Hello world");
        auto restored = fmt.deserialize(j);
        assert(restored && restored->username == "alice");
        assert(restored->bio.has_value() && *restored->bio == "Hello world");
    }
    // -- 7: named struct with optional member (null) -------------------
    {
        auto fmt = mu::make_json_format<B>(profile::codec);
        profile p{"bob", std::nullopt};
        auto j = fmt.serialize(p);
        assert(j.is_object());
        assert(j["username"] == "bob");
        assert(j["bio"].is_null());
        auto restored = fmt.deserialize(j);
        assert(restored && restored->username == "bob");
        assert(!restored->bio.has_value());
    }
    // -- 8: named struct with vector member ----------------------------
    {
        auto fmt = mu::make_json_format<B>(team::codec);
        team t{"Engineers", {{30, "Alice"}, {25, "Bob"}}};
        auto j = fmt.serialize(t);
        assert(j.is_object());
        assert(j["name"] == "Engineers");
        assert(j["members"].is_array());
        assert(j["members"].size() == 2);
        assert(j["members"][0]["age"] == 30);
        assert(j["members"][0]["name"] == "Alice");
        assert(j["members"][1]["age"] == 25);
        assert(j["members"][1]["name"] == "Bob");
        auto restored = fmt.deserialize(j);
        assert(restored && restored->name == "Engineers");
        assert(restored->members.size() == 2);
        assert(restored->members[0].age == 30 && restored->members[0].name == "Alice");
        assert(restored->members[1].age == 25 && restored->members[1].name == "Bob");
    }

    return 0;
}
