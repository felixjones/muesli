/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/format/json_format>
#include <muesli/format/boost_json_backend>
#include <muesli/codecs>

#include <boost/json.hpp>

#include <array>
#include <cassert>
#include <cmath>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

using B = muesli::boost_json_backend;
namespace mu = muesli;

// -- Test structs --------------------------------------------------------

struct point {
    int32_t x, y;
    static constexpr auto codec = mu::tuple_codec(
        mu::int32_codec.member<&point::x>(),
        mu::int32_codec.member<&point::y>()
    ).apply<point>();
};

struct person {
    int32_t age;
    std::string name;
    static constexpr auto codec = mu::tuple_codec(
        mu::int32_codec.member<&person::age>(),
        mu::string_codec.member<&person::name>()
    ).apply<person>();
};

// -- Helpers -------------------------------------------------------------

static std::string to_json(const boost::json::value& v) { return boost::json::serialize(v); }
static boost::json::value from_json(const std::string& s) { return boost::json::parse(s); }

int main() {
    // -- 1: int string round-trip ----------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::int32_codec);
        auto j = fmt.serialize(int32_t{42});
        assert(to_json(j) == "42");
        auto d = fmt.deserialize(from_json("42"));
        assert(d && *d == 42);
    }
    // -- 2: negative int -------------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::int32_codec);
        assert(to_json(fmt.serialize(int32_t{-99})) == "-99");
        auto d = fmt.deserialize(from_json("-99"));
        assert(d && *d == -99);
    }
    // -- 3: unsigned int -------------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::uint32_codec);
        assert(to_json(fmt.serialize(uint32_t{4000000000u})) == "4000000000");
        auto d = fmt.deserialize(from_json("4000000000"));
        assert(d && *d == 4000000000u);
    }
    // -- 4: bool ---------------------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::bool_codec);
        assert(to_json(fmt.serialize(true)) == "true");
        assert(to_json(fmt.serialize(false)) == "false");
        assert(fmt.deserialize(from_json("true")).value() == true);
        assert(fmt.deserialize(from_json("false")).value() == false);
    }
    // -- 5: double -------------------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::double_codec);
        auto j = fmt.serialize(3.25);
        assert(to_json(j) == "3.25E0" || to_json(j) == "3.25" || to_json(j) == "3.25e0");
        auto d = fmt.deserialize(from_json("3.25"));
        assert(d && *d == 3.25);
    }
    // -- 6: string -------------------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::string_codec);
        std::string val = "hello, muesli!";
        auto j = fmt.serialize(val);
        assert(to_json(j) == "\"hello, muesli!\"");
        auto d = fmt.deserialize(from_json("\"hello, muesli!\""));
        assert(d && *d == "hello, muesli!");
    }
    // -- 7: empty string -------------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::string_codec);
        assert(to_json(fmt.serialize(std::string())) == "\"\"");
        auto d = fmt.deserialize(from_json("\"\""));
        assert(d && d->empty());
    }
    // -- 8: string with special chars ------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::string_codec);
        std::string val = "line1\nline2\ttab";
        auto j = fmt.serialize(val);
        auto d = fmt.deserialize(j);
        assert(d && *d == "line1\nline2\ttab");
    }
    // -- 9: string with unicode ------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::string_codec);
        std::string val = "\xC3\xA9\xC3\xA0\xC3\xBC"; // eaue in UTF-8
        auto j = fmt.serialize(val);
        auto d = fmt.deserialize(j);
        assert(d && *d == val);
    }
    // -- 10: string with emoji (4-byte UTF-8) ----------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::string_codec);
        std::string val = "\xF0\x9F\x8E\xB5 music"; // musical note emoji
        auto j = fmt.serialize(val);
        auto d = fmt.deserialize(j);
        assert(d && *d == val);
    }
    // -- 11: struct -> JSON array string ---------------------------------
    {
        auto fmt = mu::make_json_format<B>(point::codec);
        assert(to_json(fmt.serialize(point{10, 20})) == "[10,20]");
        auto d = fmt.deserialize(from_json("[10,20]"));
        assert(d && d->x == 10 && d->y == 20);
    }
    // -- 12: struct with string member -----------------------------------
    {
        auto fmt = mu::make_json_format<B>(person::codec);
        auto j = fmt.serialize(person{30, "Alice"});
        assert(to_json(j) == "[30,\"Alice\"]");
        auto d = fmt.deserialize(from_json("[30,\"Alice\"]"));
        assert(d && d->age == 30 && d->name == "Alice");
    }
    // -- 13: vector<int> -> JSON array (no size prefix) ------------------
    {
        auto fmt = mu::make_json_format<B>(mu::vector_of(mu::int32_codec));
        std::vector<int32_t> val = {1, 2, 3};
        assert(to_json(fmt.serialize(val)) == "[1,2,3]");
        auto d = fmt.deserialize(from_json("[1,2,3]"));
        assert(d && *d == val);
    }
    // -- 14: vector<string> ----------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::vector_of(mu::string_codec));
        std::vector<std::string> val = {"foo", "bar"};
        assert(to_json(fmt.serialize(val)) == "[\"foo\",\"bar\"]");
        auto d = fmt.deserialize(from_json("[\"foo\",\"bar\"]"));
        assert(d && *d == val);
    }
    // -- 15: map<string,int> -> JSON object ------------------------------
    {
        auto codec = mu::vector_of(mu::pair_codec(mu::string_codec, mu::int32_codec))
            .apply<std::map<std::string, int32_t>>();
        auto fmt = mu::make_json_format<B>(codec);
        std::map<std::string, int32_t> val = {{"a", 1}, {"b", 2}};
        auto j = fmt.serialize(val);
        assert(B::is_object(j));
        auto d = fmt.deserialize(from_json("{\"a\":1,\"b\":2}"));
        assert(d && *d == val);
    }
    // -- 16: map<string,string> ------------------------------------------
    {
        auto codec = mu::vector_of(mu::pair_codec(mu::string_codec, mu::string_codec))
            .apply<std::map<std::string, std::string>>();
        auto fmt = mu::make_json_format<B>(codec);
        std::map<std::string, std::string> val = {{"greeting", "hello"}, {"target", "world"}};
        auto j = fmt.serialize(val);
        assert(B::is_object(j));
        auto d = fmt.deserialize(j);
        assert(d && *d == val);
    }
    // -- 17: optional<int> with value ------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::optional_codec(mu::int32_codec));
        assert(to_json(fmt.serialize(std::optional<int32_t>{42})) == "42");
        auto d = fmt.deserialize(from_json("42"));
        assert(d && d->has_value() && **d == 42);
    }
    // -- 18: optional<int> nullopt -> null -------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::optional_codec(mu::int32_codec));
        assert(to_json(fmt.serialize(std::optional<int32_t>{})) == "null");
        auto d = fmt.deserialize(from_json("null"));
        assert(d && !d->has_value());
    }
    // -- 19: optional<string> --------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::optional_codec(mu::string_codec));
        assert(to_json(fmt.serialize(std::optional<std::string>{"hi"})) == "\"hi\"");
        assert(to_json(fmt.serialize(std::optional<std::string>{})) == "null");
        auto d = fmt.deserialize(from_json("\"hi\""));
        assert(d && **d == "hi");
        auto n = fmt.deserialize(from_json("null"));
        assert(n && !n->has_value());
    }
    // -- 20: variant<int,string> -----------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::variant_codec(mu::int32_codec, mu::string_codec));
        using V = std::variant<int32_t, std::string>;
        // Array form: [index, value]
        auto j1 = fmt.serialize(V{int32_t{7}});
        assert(B::is_array(j1));
        auto d1 = fmt.deserialize(j1);
        assert(d1 && std::get<0>(*d1) == 7);
        auto d2 = fmt.deserialize(from_json("[1,\"hi\"]"));
        assert(d2 && std::get<1>(*d2) == "hi");
    }
    // -- 21: varint (signed) -> JSON integer -----------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::signed_varint_codec);
        assert(to_json(fmt.serialize(std::intmax_t{-42})) == "-42");
        auto d = fmt.deserialize(from_json("-42"));
        assert(d && *d == -42);
    }
    // -- 22: varint (unsigned) -> JSON integer ---------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::unsigned_varint_codec);
        assert(to_json(fmt.serialize(std::uintmax_t{99999})) == "99999");
        auto d = fmt.deserialize(from_json("99999"));
        assert(d && *d == 99999);
    }
    // -- 23: vector<optional<string>> -> array with nulls ----------------
    {
        auto fmt = mu::make_json_format<B>(mu::vector_of(mu::optional_codec(mu::string_codec)));
        std::vector<std::optional<std::string>> val = {"a", std::nullopt, "c"};
        auto j = fmt.serialize(val);
        assert(to_json(j) == "[\"a\",null,\"c\"]");
        auto d = fmt.deserialize(j);
        assert(d && d->size() == 3);
        assert((*d)[0].has_value() && *(*d)[0] == "a");
        assert(!(*d)[1].has_value());
        assert((*d)[2].has_value() && *(*d)[2] == "c");
    }
    // -- 24: map<string,point> -> JSON object of arrays ------------------
    {
        auto codec = mu::vector_of(mu::pair_codec(mu::string_codec, point::codec))
            .apply<std::map<std::string, point>>();
        auto fmt = mu::make_json_format<B>(codec);
        std::map<std::string, point> val = {{"a", {1, 2}}, {"b", {3, 4}}};
        auto j = fmt.serialize(val);
        assert(B::is_object(j));
        auto d = fmt.deserialize(j);
        assert(d && d->at("a").x == 1 && d->at("b").y == 4);
    }
    // =====================================================================
    // String round-trip tests: JSON string -> C++ object -> JSON string
    // =====================================================================

    // -- 25: person struct string round-trip ------------------------------
    {
        auto fmt = mu::make_json_format<B>(person::codec);
        std::string input = "[25,\"Bob\"]";
        auto obj = fmt.deserialize(from_json(input));
        assert(obj && obj->age == 25 && obj->name == "Bob");
        std::string output = to_json(fmt.serialize(*obj));
        assert(output == "[25,\"Bob\"]");
    }
    // -- 26: point struct string round-trip -------------------------------
    {
        auto fmt = mu::make_json_format<B>(point::codec);
        std::string input = "[10,20]";
        auto obj = fmt.deserialize(from_json(input));
        assert(obj && obj->x == 10 && obj->y == 20);
        std::string output = to_json(fmt.serialize(*obj));
        assert(output == "[10,20]");
    }
    // -- 27: map<string,int> string round-trip ---------------------------
    {
        auto codec = mu::vector_of(mu::pair_codec(mu::string_codec, mu::int32_codec))
            .apply<std::map<std::string, int32_t>>();
        auto fmt = mu::make_json_format<B>(codec);
        // Boost.JSON preserves insertion order; std::map iterates sorted
        std::string input = "{\"alpha\":1,\"beta\":2,\"gamma\":3}";
        auto obj = fmt.deserialize(from_json(input));
        assert(obj && obj->size() == 3);
        assert(obj->at("alpha") == 1 && obj->at("beta") == 2 && obj->at("gamma") == 3);
        std::string output = to_json(fmt.serialize(*obj));
        assert(output == "{\"alpha\":1,\"beta\":2,\"gamma\":3}");
    }
    // -- 28: vector<point> string round-trip -----------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::vector_of(point::codec));
        std::string input = "[[1,2],[3,4],[5,6]]";
        auto obj = fmt.deserialize(from_json(input));
        assert(obj && obj->size() == 3);
        assert((*obj)[0].x == 1 && (*obj)[0].y == 2);
        assert((*obj)[1].x == 3 && (*obj)[1].y == 4);
        assert((*obj)[2].x == 5 && (*obj)[2].y == 6);
        std::string output = to_json(fmt.serialize(*obj));
        assert(output == "[[1,2],[3,4],[5,6]]");
    }
    // -- 29: optional string round-trip (value and null) -----------------
    {
        auto fmt = mu::make_json_format<B>(mu::optional_codec(mu::string_codec));
        auto obj1 = fmt.deserialize(from_json("\"present\""));
        assert(obj1 && obj1->has_value() && **obj1 == "present");
        assert(to_json(fmt.serialize(*obj1)) == "\"present\"");

        auto obj2 = fmt.deserialize(from_json("null"));
        assert(obj2 && !obj2->has_value());
        assert(to_json(fmt.serialize(*obj2)) == "null");
    }
    // -- 30: variant string round-trip -----------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::variant_codec(mu::int32_codec, mu::string_codec));
        auto obj1 = fmt.deserialize(from_json("[0,42]"));
        assert(obj1 && std::get<0>(*obj1) == 42);
        // Round-trip: re-serialize and parse back
        std::string rt1 = to_json(fmt.serialize(*obj1));
        auto back1 = fmt.deserialize(from_json(rt1));
        assert(back1 && std::get<0>(*back1) == 42);

        auto obj2 = fmt.deserialize(from_json("[1,\"hello\"]"));
        assert(obj2 && std::get<1>(*obj2) == "hello");
        std::string rt2 = to_json(fmt.serialize(*obj2));
        auto back2 = fmt.deserialize(from_json(rt2));
        assert(back2 && std::get<1>(*back2) == "hello");
    }
    // -- 31: map<string,point> string round-trip -------------------------
    {
        auto codec = mu::vector_of(mu::pair_codec(mu::string_codec, point::codec))
            .apply<std::map<std::string, point>>();
        auto fmt = mu::make_json_format<B>(codec);
        std::string input = "{\"origin\":[0,0],\"target\":[10,20]}";
        auto obj = fmt.deserialize(from_json(input));
        assert(obj && obj->size() == 2);
        assert(obj->at("origin").x == 0 && obj->at("origin").y == 0);
        assert(obj->at("target").x == 10 && obj->at("target").y == 20);
        std::string output = to_json(fmt.serialize(*obj));
        assert(output == "{\"origin\":[0,0],\"target\":[10,20]}");
    }
    // -- 32: vector<optional<int>> string round-trip ---------------------
    {
        auto fmt = mu::make_json_format<B>(mu::vector_of(mu::optional_codec(mu::int32_codec)));
        std::string input = "[1,null,3,null,5]";
        auto obj = fmt.deserialize(from_json(input));
        assert(obj && obj->size() == 5);
        assert((*obj)[0].has_value() && *(*obj)[0] == 1);
        assert(!(*obj)[1].has_value());
        assert((*obj)[2].has_value() && *(*obj)[2] == 3);
        assert(!(*obj)[3].has_value());
        assert((*obj)[4].has_value() && *(*obj)[4] == 5);
        std::string output = to_json(fmt.serialize(*obj));
        assert(output == "[1,null,3,null,5]");
    }
    // -- 33: vector<person> string round-trip ----------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::vector_of(person::codec));
        std::string input = "[[30,\"Alice\"],[25,\"Bob\"],[22,\"Charlie\"]]";
        auto obj = fmt.deserialize(from_json(input));
        assert(obj && obj->size() == 3);
        assert((*obj)[0].age == 30 && (*obj)[0].name == "Alice");
        assert((*obj)[1].age == 25 && (*obj)[1].name == "Bob");
        assert((*obj)[2].age == 22 && (*obj)[2].name == "Charlie");
        std::string output = to_json(fmt.serialize(*obj));
        assert(output == "[[30,\"Alice\"],[25,\"Bob\"],[22,\"Charlie\"]]");
    }
    // -- 34: map<string,string> string round-trip ------------------------
    {
        auto codec = mu::vector_of(mu::pair_codec(mu::string_codec, mu::string_codec))
            .apply<std::map<std::string, std::string>>();
        auto fmt = mu::make_json_format<B>(codec);
        std::string input = "{\"greeting\":\"hello\",\"target\":\"world\"}";
        auto obj = fmt.deserialize(from_json(input));
        assert(obj && obj->at("greeting") == "hello" && obj->at("target") == "world");
        std::string output = to_json(fmt.serialize(*obj));
        assert(output == "{\"greeting\":\"hello\",\"target\":\"world\"}");
    }
    // -- 35: empty containers string round-trip --------------------------
    {
        auto vec_fmt = mu::make_json_format<B>(mu::vector_of(mu::int32_codec));
        auto vec_obj = vec_fmt.deserialize(from_json("[]"));
        assert(vec_obj && vec_obj->empty());
        assert(to_json(vec_fmt.serialize(*vec_obj)) == "[]");

        auto map_codec = mu::vector_of(mu::pair_codec(mu::string_codec, mu::int32_codec))
            .apply<std::map<std::string, int32_t>>();
        auto map_fmt = mu::make_json_format<B>(map_codec);
        auto map_obj = map_fmt.deserialize(from_json("{}"));
        assert(map_obj && map_obj->empty());
        assert(to_json(map_fmt.serialize(*map_obj)) == "{}");
    }

    // =====================================================================
    // array_codec tests
    // =====================================================================

    // -- 36: array<int32_t, 3> -> JSON array (no size prefix) ------------
    {
        auto fmt = mu::make_json_format<B>(mu::array_of<3>(mu::int32_codec));
        std::array<int32_t, 3> val = {10, 20, 30};
        assert(to_json(fmt.serialize(val)) == "[10,20,30]");
        auto d = fmt.deserialize(from_json("[10,20,30]"));
        assert(d && (*d)[0] == 10 && (*d)[1] == 20 && (*d)[2] == 30);
    }
    // -- 37: array<string, 2> -> JSON array of strings -------------------
    {
        auto fmt = mu::make_json_format<B>(mu::array_of<2>(mu::string_codec));
        std::array<std::string, 2> val = {"hello", "world"};
        assert(to_json(fmt.serialize(val)) == "[\"hello\",\"world\"]");
        auto d = fmt.deserialize(from_json("[\"hello\",\"world\"]"));
        assert(d && (*d)[0] == "hello" && (*d)[1] == "world");
    }

    // =====================================================================
    // nullable_codec tests (shared_ptr via make_nullable)
    // =====================================================================

    // -- 38: shared_ptr<int32_t> with value -> JSON integer ----------------
    {
        auto fmt = mu::make_json_format<B>(mu::make_nullable<std::shared_ptr>(mu::int32_codec));
        auto ptr = std::make_shared<int32_t>(42);
        assert(to_json(fmt.serialize(ptr)) == "42");
        auto d = fmt.deserialize(from_json("42"));
        assert(d && *d && **d == 42);
    }
    // -- 39: shared_ptr<int32_t> null -> JSON null -------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::make_nullable<std::shared_ptr>(mu::int32_codec));
        std::shared_ptr<int32_t> ptr;
        assert(to_json(fmt.serialize(ptr)) == "null");
        auto d = fmt.deserialize(from_json("null"));
        assert(d && !*d);
    }
    // -- 40: shared_ptr<string> with value -> JSON string ------------------
    {
        auto fmt = mu::make_json_format<B>(mu::make_nullable<std::shared_ptr>(mu::string_codec));
        auto ptr = std::make_shared<std::string>("hello");
        assert(to_json(fmt.serialize(ptr)) == "\"hello\"");
        auto d = fmt.deserialize(from_json("\"hello\""));
        assert(d && *d && **d == "hello");
    }
    // -- 41: shared_ptr<string> null -> JSON null --------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::make_nullable<std::shared_ptr>(mu::string_codec));
        std::shared_ptr<std::string> ptr;
        assert(to_json(fmt.serialize(ptr)) == "null");
        auto d = fmt.deserialize(from_json("null"));
        assert(d && !*d);
    }

    // =====================================================================
    // constrained_codec + json_format integration
    // =====================================================================

    // -- 42: constrained int -> JSON integer (constraint is transparent) --
    {
        auto codec = mu::int32_codec.constrain([](int32_t v) { return v > 0; });
        auto fmt = mu::make_json_format<B>(codec);
        assert(to_json(fmt.serialize(int32_t{99})) == "99");
        auto d = fmt.deserialize(from_json("99"));
        assert(d && *d == 99);
    }
    // -- 43: chained constraints -> JSON integer -------------------------
    {
        auto codec = mu::int32_codec
            .constrain([](int32_t v) { return v > 0; })
            .constrain([](int32_t v) { return v < 1000; });
        auto fmt = mu::make_json_format<B>(codec);
        assert(to_json(fmt.serialize(int32_t{500})) == "500");
        auto d = fmt.deserialize(from_json("500"));
        assert(d && *d == 500);
    }

    // =====================================================================
    // transform_codec + json_format integration
    // =====================================================================

    // -- 44: transform int -> json_format natively handles int, bypassing
    //        the transform. Round-trip preserves the original value.
    {
        auto codec = mu::int32_codec.transform(
            [](int32_t v) { return v * 2; },
            [](int32_t v) { return v / 2; }
        );
        auto fmt = mu::make_json_format<B>(codec);
        assert(to_json(fmt.serialize(int32_t{21})) == "21");
        auto d = fmt.deserialize(from_json("21"));
        assert(d && *d == 21);
    }
    // -- 45: transform string -> also natively handled, round-trip works
    {
        auto codec = mu::string_codec.transform(
            [](const std::string& s) { return "pfx_" + s; },
            [](const std::string& s) { return s.substr(4); }
        );
        auto fmt = mu::make_json_format<B>(codec);
        assert(to_json(fmt.serialize(std::string("data"))) == "\"data\"");
        auto d = fmt.deserialize(from_json("\"data\""));
        assert(d && *d == "data");
    }

    // =====================================================================
    // Error / malformed JSON deserialization tests
    // =====================================================================

    // -- 46: int codec given a string -> nullopt -------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::int32_codec);
        auto d = fmt.deserialize(from_json("\"not a number\""));
        assert(!d.has_value());
    }
    // -- 47: string codec given an integer -> nullopt --------------------
    {
        auto fmt = mu::make_json_format<B>(mu::string_codec);
        auto d = fmt.deserialize(from_json("42"));
        assert(!d.has_value());
    }
    // -- 48: bool codec given a string -> nullopt ------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::bool_codec);
        auto d = fmt.deserialize(from_json("\"true\""));
        assert(!d.has_value());
    }
    // -- 49: variant with invalid index -> nullopt -----------------------
    {
        auto fmt = mu::make_json_format<B>(mu::variant_codec(mu::int32_codec, mu::bool_codec));
        auto d = fmt.deserialize(from_json("[99,1]"));
        assert(!d.has_value());
    }
    // -- 50: variant array missing value slot -> nullopt ------------------
    {
        auto fmt = mu::make_json_format<B>(mu::variant_codec(mu::int32_codec, mu::bool_codec));
        auto d = fmt.deserialize(from_json("[0]"));
        assert(!d.has_value());
    }
    // -- 51: variant array missing index slot -> nullopt ------------------
    {
        auto fmt = mu::make_json_format<B>(mu::variant_codec(mu::int32_codec, mu::bool_codec));
        auto d = fmt.deserialize(from_json("[1]"));
        assert(!d.has_value());
    }
    // -- 52: vector given non-array -> nullopt ---------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::vector_of(mu::int32_codec));
        auto d = fmt.deserialize(from_json("\"not an array\""));
        assert(!d.has_value());
    }
    // -- 53: struct given wrong type -> nullopt ---------------------------
    {
        auto fmt = mu::make_json_format<B>(point::codec);
        auto d = fmt.deserialize(from_json("\"oops\""));
        assert(!d.has_value());
    }

    return 0;
}
