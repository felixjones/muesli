/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/schema/extract>
#include <muesli/schema/json_schema>
#include <muesli/codecs>

#include <cassert>
#include <optional>
#include <string>
#include <vector>

// ===========================================================================
// Minimal self-contained JSON backend (same as test_json_format.cpp)
// ===========================================================================
namespace test {

struct json_value {
    enum class kind { null_k, bool_k, int_k, uint_k, float_k, string_k, array_k, object_k };

    json_value() = default;
    json_value(const json_value&) = default;
    json_value(json_value&&) noexcept = default;
    json_value& operator=(const json_value&) = default;
    json_value& operator=(json_value&&) noexcept = default;
    ~json_value() = default;

    kind type = kind::null_k;
    bool bool_val = false;
    std::int64_t int_val = 0;
    std::uint64_t uint_val = 0;
    double float_val = 0.0;
    std::string string_val;
    std::vector<json_value> array_val;
    std::vector<std::pair<std::string, json_value>> object_val;
};

struct test_backend {
    using value_type = json_value;

    static value_type make_null() { value_type v; v.type = json_value::kind::null_k; return v; }
    static value_type make_bool(bool b) { value_type v; v.type = json_value::kind::bool_k; v.bool_val = b; return v; }
    static value_type make_int(std::int64_t i) { value_type v; v.type = json_value::kind::int_k; v.int_val = i; return v; }
    static value_type make_uint(std::uint64_t u) { value_type v; v.type = json_value::kind::uint_k; v.uint_val = u; return v; }
    static value_type make_float(double d) { value_type v; v.type = json_value::kind::float_k; v.float_val = d; return v; }
    static value_type make_string(std::string s) { value_type v; v.type = json_value::kind::string_k; v.string_val = std::move(s); return v; }
    static value_type make_array() { value_type v; v.type = json_value::kind::array_k; return v; }
    static value_type make_object() { value_type v; v.type = json_value::kind::object_k; return v; }

    static void array_push_back(value_type& arr, const value_type& elem) { arr.array_val.push_back(elem); }
    static std::size_t array_size(const value_type& arr) { return arr.array_val.size(); }
    static const value_type& array_at(const value_type& arr, std::size_t idx) { return arr.array_val[idx]; }

    static void object_set(value_type& obj, const std::string& key, const value_type& val) {
        for (auto& [k, v] : obj.object_val) { if (k == key) { v = val; return; } }
        obj.object_val.emplace_back(key, val);
    }
    static const value_type& object_get(const value_type& obj, const std::string& key) {
        for (const auto& [k, v] : obj.object_val) { if (k == key) return v; }
        static const value_type null_val{}; return null_val;
    }
    static bool object_has_key(const value_type& obj, const std::string& key) {
        for (const auto& [k, v] : obj.object_val) { if (k == key) return true; }
        return false;
    }
    static std::vector<std::string> object_keys(const value_type& obj) {
        std::vector<std::string> keys;
        for (const auto& [k, v] : obj.object_val) keys.push_back(k);
        return keys;
    }

    static bool is_null(const value_type& v) { return v.type == json_value::kind::null_k; }
    static bool is_bool(const value_type& v) { return v.type == json_value::kind::bool_k; }
    static bool is_integer(const value_type& v) { return v.type == json_value::kind::int_k || v.type == json_value::kind::uint_k; }
    static bool is_floating(const value_type& v) { return v.type == json_value::kind::float_k; }
    static bool is_string(const value_type& v) { return v.type == json_value::kind::string_k; }
    static bool is_array(const value_type& v) { return v.type == json_value::kind::array_k; }
    static bool is_object(const value_type& v) { return v.type == json_value::kind::object_k; }

    static bool as_bool(const value_type& v) { return v.bool_val; }
    static std::int64_t as_int64(const value_type& v) {
        if (v.type == json_value::kind::uint_k) return static_cast<std::int64_t>(v.uint_val);
        return v.int_val;
    }
    static std::uint64_t as_uint64(const value_type& v) {
        if (v.type == json_value::kind::int_k) return static_cast<std::uint64_t>(v.int_val);
        return v.uint_val;
    }
    static double as_double(const value_type& v) {
        if (v.type == json_value::kind::int_k) return static_cast<double>(v.int_val);
        if (v.type == json_value::kind::uint_k) return static_cast<double>(v.uint_val);
        return v.float_val;
    }
    static std::string as_string(const value_type& v) { return v.string_val; }
};

} // namespace test

using B = test::test_backend;
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
    // =====================================================================
    // 1. Primitive schemas -> JSON Schema
    // =====================================================================

    // -- 1a: boolean -> {"type": "boolean"} ----------------------------
    {
        constexpr auto s = mu::make_schema(mu::bool_codec);
        auto j = mu::to_json_schema<B>(s);
        assert(B::is_object(j));
        assert(B::as_string(B::object_get(j, "type")) == "boolean");
    }
    // -- 1b: signed_integer -> {"type": "integer"} ---------------------
    {
        constexpr auto s = mu::make_schema(mu::int32_codec);
        auto j = mu::to_json_schema<B>(s);
        assert(B::as_string(B::object_get(j, "type")) == "integer");
        assert(!B::object_has_key(j, "minimum"));
    }
    // -- 1c: unsigned_integer -> {"type": "integer", "minimum": 0} -----
    {
        constexpr auto s = mu::make_schema(mu::uint32_codec);
        auto j = mu::to_json_schema<B>(s);
        assert(B::as_string(B::object_get(j, "type")) == "integer");
        assert(B::object_has_key(j, "minimum"));
        assert(B::as_int64(B::object_get(j, "minimum")) == 0);
    }
    // -- 1d: floating_point -> {"type": "number"} ----------------------
    {
        constexpr auto s = mu::make_schema(mu::double_codec);
        auto j = mu::to_json_schema<B>(s);
        assert(B::as_string(B::object_get(j, "type")) == "number");
    }
    // -- 1e: string -> {"type": "string"} ------------------------------
    {
        constexpr auto s = mu::make_schema(mu::string_codec);
        auto j = mu::to_json_schema<B>(s);
        assert(B::as_string(B::object_get(j, "type")) == "string");
    }
    // -- 1f: monostate -> {"type": "null"} -----------------------------
    {
        constexpr auto s = mu::make_schema(mu::monostate_codec);
        auto j = mu::to_json_schema<B>(s);
        assert(B::as_string(B::object_get(j, "type")) == "null");
    }

    // =====================================================================
    // 2. Optional schema -> JSON Schema oneOf
    // =====================================================================

    // -- 2a: optional<int> -> {"oneOf": [{"type":"integer"}, {"type":"null"}]}
    {
        constexpr auto s = mu::make_schema(mu::optional_codec(mu::int32_codec));
        auto j = mu::to_json_schema<B>(s);
        assert(B::object_has_key(j, "oneOf"));
        auto& choices = B::object_get(j, "oneOf");
        assert(B::is_array(choices) && B::array_size(choices) == 2);
        assert(B::as_string(B::object_get(B::array_at(choices, 0), "type")) == "integer");
        assert(B::as_string(B::object_get(B::array_at(choices, 1), "type")) == "null");
    }

    // =====================================================================
    // 3. Array schema -> JSON Schema
    // =====================================================================

    // -- 3a: vector<int> -> {"type":"array","items":{"type":"integer"}}
    {
        constexpr auto s = mu::make_schema(mu::vector_of(mu::int32_codec));
        auto j = mu::to_json_schema<B>(s);
        assert(B::as_string(B::object_get(j, "type")) == "array");
        assert(B::object_has_key(j, "items"));
        assert(B::as_string(B::object_get(B::object_get(j, "items"), "type")) == "integer");
    }
    // -- 3b: vector<string>
    {
        constexpr auto s = mu::make_schema(mu::vector_of(mu::string_codec));
        auto j = mu::to_json_schema<B>(s);
        assert(B::as_string(B::object_get(j, "type")) == "array");
        assert(B::as_string(B::object_get(B::object_get(j, "items"), "type")) == "string");
    }

    // =====================================================================
    // 4. Object schema -> JSON Schema
    // =====================================================================

    // -- 4a: point -> {"type":"object","properties":{"x":{...},"y":{...}},"required":["x","y"]}
    {
        constexpr auto s = mu::make_schema(point::codec);
        auto j = mu::to_json_schema<B>(s);
        assert(B::as_string(B::object_get(j, "type")) == "object");

        auto& props = B::object_get(j, "properties");
        assert(B::is_object(props));
        assert(B::object_has_key(props, "x"));
        assert(B::object_has_key(props, "y"));
        assert(B::as_string(B::object_get(B::object_get(props, "x"), "type")) == "integer");
        assert(B::as_string(B::object_get(B::object_get(props, "y"), "type")) == "integer");

        auto& req = B::object_get(j, "required");
        assert(B::is_array(req) && B::array_size(req) == 2);
        assert(B::as_string(B::array_at(req, 0)) == "x");
        assert(B::as_string(B::array_at(req, 1)) == "y");
    }
    // -- 4b: person -> object with "age" (integer) + "name" (string)
    {
        constexpr auto s = mu::make_schema(person::codec);
        auto j = mu::to_json_schema<B>(s);
        auto& props = B::object_get(j, "properties");
        assert(B::as_string(B::object_get(B::object_get(props, "age"), "type")) == "integer");
        assert(B::as_string(B::object_get(B::object_get(props, "name"), "type")) == "string");

        auto& req = B::object_get(j, "required");
        assert(B::array_size(req) == 2);
    }
    // -- 4c: profile -> optional field "bio" is NOT in "required",
    //         and its property schema is the inner type (string), not oneOf
    {
        constexpr auto s = mu::make_schema(profile::codec);
        auto j = mu::to_json_schema<B>(s);
        auto& props = B::object_get(j, "properties");
        assert(B::object_has_key(props, "username"));
        assert(B::object_has_key(props, "bio"));

        // bio property should be {"type":"string"}, NOT {"oneOf":[...]}
        auto& bio = B::object_get(props, "bio");
        assert(B::as_string(B::object_get(bio, "type")) == "string");

        // required should only contain "username"
        auto& req = B::object_get(j, "required");
        assert(B::array_size(req) == 1);
        assert(B::as_string(B::array_at(req, 0)) == "username");
    }
    // -- 4d: team -> nested: members is array of person objects
    {
        constexpr auto s = mu::make_schema(team::codec);
        auto j = mu::to_json_schema<B>(s);
        auto& props = B::object_get(j, "properties");

        // name: string
        assert(B::as_string(B::object_get(B::object_get(props, "name"), "type")) == "string");

        // members: array of objects
        auto& members = B::object_get(props, "members");
        assert(B::as_string(B::object_get(members, "type")) == "array");
        auto& items = B::object_get(members, "items");
        assert(B::as_string(B::object_get(items, "type")) == "object");

        // items should have properties "age" and "name"
        auto& innerProps = B::object_get(items, "properties");
        assert(B::object_has_key(innerProps, "age"));
        assert(B::object_has_key(innerProps, "name"));
    }

    // =====================================================================
    // 5. Tuple schema -> JSON Schema prefixItems
    // =====================================================================

    // -- 5a: tuple<int, float, bool>
    {
        constexpr auto codec = mu::tuple_codec(mu::int32_codec, mu::float_codec, mu::bool_codec);
        constexpr auto s = mu::make_schema(codec);
        auto j = mu::to_json_schema<B>(s);
        assert(B::as_string(B::object_get(j, "type")) == "array");
        assert(B::object_has_key(j, "prefixItems"));

        auto& prefix = B::object_get(j, "prefixItems");
        assert(B::array_size(prefix) == 3);
        assert(B::as_string(B::object_get(B::array_at(prefix, 0), "type")) == "integer");
        assert(B::as_string(B::object_get(B::array_at(prefix, 1), "type")) == "number");
        assert(B::as_string(B::object_get(B::array_at(prefix, 2), "type")) == "boolean");

        // items: false means no additional items
        assert(B::is_bool(B::object_get(j, "items")));
        assert(B::as_bool(B::object_get(j, "items")) == false);
    }

    // =====================================================================
    // 6. Variant schema -> JSON Schema oneOf
    // =====================================================================

    // -- 6a: variant<int, string>
    {
        constexpr auto codec = mu::variant_codec(mu::int32_codec, mu::string_codec);
        constexpr auto s = mu::make_schema(codec);
        auto j = mu::to_json_schema<B>(s);
        assert(B::object_has_key(j, "oneOf"));
        auto& choices = B::object_get(j, "oneOf");
        assert(B::array_size(choices) == 2);
        assert(B::as_string(B::object_get(B::array_at(choices, 0), "type")) == "integer");
        assert(B::as_string(B::object_get(B::array_at(choices, 1), "type")) == "string");
    }
    // -- 6b: variant<monostate, int> (nullable)
    {
        constexpr auto codec = mu::variant_codec(mu::monostate_codec, mu::int32_codec);
        constexpr auto s = mu::make_schema(codec);
        auto j = mu::to_json_schema<B>(s);
        auto& choices = B::object_get(j, "oneOf");
        assert(B::array_size(choices) == 2);
        assert(B::as_string(B::object_get(B::array_at(choices, 0), "type")) == "null");
        assert(B::as_string(B::object_get(B::array_at(choices, 1), "type")) == "integer");
    }
    // -- 6c: variant containing objects
    {
        constexpr auto codec = mu::variant_codec(point::codec, person::codec);
        constexpr auto s = mu::make_schema(codec);
        auto j = mu::to_json_schema<B>(s);
        auto& choices = B::object_get(j, "oneOf");
        assert(B::array_size(choices) == 2);
        assert(B::as_string(B::object_get(B::array_at(choices, 0), "type")) == "object");
        assert(B::as_string(B::object_get(B::array_at(choices, 1), "type")) == "object");
    }

    // =====================================================================
    // 7. Complex compositions
    // =====================================================================

    // -- 7a: vector of vectors -> nested array items
    {
        constexpr auto s = mu::make_schema(mu::vector_of(mu::vector_of(mu::int32_codec)));
        auto j = mu::to_json_schema<B>(s);
        assert(B::as_string(B::object_get(j, "type")) == "array");
        auto& items = B::object_get(j, "items");
        assert(B::as_string(B::object_get(items, "type")) == "array");
        assert(B::as_string(B::object_get(B::object_get(items, "items"), "type")) == "integer");
    }
    // -- 7b: optional<vector<string>> -> oneOf with array
    {
        constexpr auto s = mu::make_schema(mu::optional_codec(mu::vector_of(mu::string_codec)));
        auto j = mu::to_json_schema<B>(s);
        auto& choices = B::object_get(j, "oneOf");
        assert(B::array_size(choices) == 2);
        assert(B::as_string(B::object_get(B::array_at(choices, 0), "type")) == "array");
        assert(B::as_string(B::object_get(B::array_at(choices, 1), "type")) == "null");
    }

    return 0;
}

