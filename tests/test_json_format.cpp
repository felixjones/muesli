/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/format/json_format>
#include <muesli/codecs>

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

// ===========================================================================
// Minimal self-contained JSON value type (for testing only -- no dependency)
// ===========================================================================
namespace test {

struct json_value {
    enum class kind { null_k, bool_k, int_k, uint_k, float_k, string_k, array_k, object_k };

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

inline auto make_string_int_map_codec() {
    return mu::vector_of(mu::pair_codec(mu::string_codec, mu::int32_codec))
        .apply<std::map<std::string, int32_t>>();
}

inline auto make_string_point_map_codec() {
    return mu::vector_of(mu::pair_codec(mu::string_codec, point::codec))
        .apply<std::map<std::string, point>>();
}

int main() {
    // -- 1: int round-trip -----------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::int_codec);
        auto j = fmt.serialize(42);
        assert(B::is_integer(j) && B::as_int64(j) == 42);
        auto d = fmt.deserialize(j);
        assert(d.has_value() && *d == 42);
    }
    // -- 2: negative int -------------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::int_codec);
        auto j = fmt.serialize(-99);
        auto d = fmt.deserialize(j);
        assert(d && *d == -99);
    }
    // -- 3: unsigned int -------------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::uint32_codec);
        auto j = fmt.serialize(std::uint32_t{12345});
        assert(B::is_integer(j));
        auto d = fmt.deserialize(j);
        assert(d && *d == 12345u);
    }
    // -- 4: float round-trip ---------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::float_codec);
        auto j = fmt.serialize(3.14f);
        assert(B::is_floating(j));
        auto d = fmt.deserialize(j);
        assert(d && std::abs(*d - 3.14f) < 0.001f);
    }
    // -- 5: double round-trip --------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::double_codec);
        auto j = fmt.serialize(2.718281828);
        auto d = fmt.deserialize(j);
        assert(d && std::abs(*d - 2.718281828) < 1e-9);
    }
    // -- 6: bool round-trip ----------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::bool_codec);
        auto jt = fmt.serialize(true);
        auto jf = fmt.serialize(false);
        assert(B::is_bool(jt) && B::as_bool(jt) == true);
        assert(B::as_bool(jf) == false);
        assert(fmt.deserialize(jt).value() == true);
        assert(fmt.deserialize(jf).value() == false);
    }
    // -- 7: string round-trip --------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::string_codec);
        std::string value = "hello, muesli!";
        auto j = fmt.serialize(value);
        assert(B::is_string(j) && B::as_string(j) == "hello, muesli!");
        auto d = fmt.deserialize(j);
        assert(d && *d == "hello, muesli!");
    }
    // -- 8: empty string -------------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::string_codec);
        auto j = fmt.serialize(std::string(""));
        assert(B::is_string(j) && B::as_string(j).empty());
        auto d = fmt.deserialize(j);
        assert(d && d->empty());
    }
    // -- 9: tuple -> JSON array -------------------------------------------
    {
        auto codec = mu::tuple_codec(mu::int32_codec, mu::float_codec, mu::bool_codec);
        auto fmt = mu::make_json_format<B>(codec);
        auto val = std::make_tuple(std::int32_t{7}, 2.5f, true);
        auto j = fmt.serialize(val);
        assert(B::is_array(j) && B::array_size(j) == 3);
        assert(B::as_int64(B::array_at(j, 0)) == 7);
        assert(B::as_bool(B::array_at(j, 2)) == true);
        auto d = fmt.deserialize(j);
        assert(d && std::get<0>(*d) == 7 && std::get<2>(*d) == true);
    }
    // -- 10: struct via apply_codec -> JSON array -------------------------
    {
        auto fmt = mu::make_json_format<B>(point::codec);
        auto j = fmt.serialize(point{10, 20});
        assert(B::is_array(j) && B::array_size(j) == 2);
        assert(B::as_int64(B::array_at(j, 0)) == 10);
        assert(B::as_int64(B::array_at(j, 1)) == 20);
        auto d = fmt.deserialize(j);
        assert(d && d->x == 10 && d->y == 20);
    }
    // -- 11: vector -> JSON array (no size prefix) ------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::vector_of(mu::int32_codec));
        std::vector<int32_t> value = {10, 20, 30};
        auto j = fmt.serialize(value);
        // Native: just a flat JSON array [10, 20, 30]
        assert(B::is_array(j));
        assert(B::array_size(j) == 3);
        assert(B::as_int64(B::array_at(j, 0)) == 10);
        assert(B::as_int64(B::array_at(j, 1)) == 20);
        assert(B::as_int64(B::array_at(j, 2)) == 30);
        auto d = fmt.deserialize(j);
        assert(d && d->size() == 3);
        assert((*d)[0] == 10 && (*d)[1] == 20 && (*d)[2] == 30);
    }
    // -- 12: optional with value -> inner value (not variant wrapper) -----
    {
        auto fmt = mu::make_json_format<B>(mu::optional_codec(mu::int32_codec));
        std::optional<int32_t> value = 42;
        auto j = fmt.serialize(value);
        // Native: just the integer 42, not {"i":1, "v":42}
        assert(B::is_integer(j));
        assert(B::as_int64(j) == 42);
        auto d = fmt.deserialize(j);
        assert(d && d->has_value() && **d == 42);
    }
    // -- 13: optional nullopt -> JSON null --------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::optional_codec(mu::int32_codec));
        std::optional<int32_t> value = std::nullopt;
        auto j = fmt.serialize(value);
        assert(B::is_null(j));
        auto d = fmt.deserialize(j);
        assert(d && !d->has_value());
    }
    // -- 14: variant -----------------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::variant_codec(mu::int32_codec, mu::bool_codec));
        std::variant<int32_t, bool> v1 = int32_t{99};
        auto j1 = fmt.serialize(v1);
        assert(B::is_object(j1));
        assert(B::as_uint64(B::object_get(j1, "i")) == 0);
        assert(B::as_int64(B::object_get(j1, "v")) == 99);
        auto d1 = fmt.deserialize(j1);
        assert(d1 && std::get<0>(*d1) == 99);

        std::variant<int32_t, bool> v2 = true;
        auto j2 = fmt.serialize(v2);
        assert(B::as_uint64(B::object_get(j2, "i")) == 1);
        assert(B::as_bool(B::object_get(j2, "v")) == true);
        auto d2 = fmt.deserialize(j2);
        assert(d2 && std::get<1>(*d2) == true);
    }
    // -- 15: struct with string member -----------------------------------
    {
        auto fmt = mu::make_json_format<B>(person::codec);
        auto j = fmt.serialize(person{30, "Alice"});
        assert(B::is_array(j));
        assert(B::as_int64(B::array_at(j, 0)) == 30);
        assert(B::as_string(B::array_at(j, 1)) == "Alice");
        auto d = fmt.deserialize(j);
        assert(d && d->age == 30 && d->name == "Alice");
    }
    // -- 16: map<string,int> -> JSON object (no size prefix) -------------
    {
        auto fmt = mu::make_json_format<B>(make_string_int_map_codec());
        std::map<std::string, int32_t> value = {{"alpha", 1}, {"beta", 2}, {"gamma", 3}};
        auto j = fmt.serialize(value);
        // Native: {"alpha":1, "beta":2, "gamma":3}
        assert(B::is_object(j));
        assert(B::object_has_key(j, "alpha"));
        assert(B::object_has_key(j, "beta"));
        assert(B::object_has_key(j, "gamma"));
        assert(B::as_int64(B::object_get(j, "alpha")) == 1);
        assert(B::as_int64(B::object_get(j, "beta")) == 2);
        assert(B::as_int64(B::object_get(j, "gamma")) == 3);
        auto d = fmt.deserialize(j);
        assert(d && d->size() == 3);
        assert(d->at("alpha") == 1 && d->at("beta") == 2 && d->at("gamma") == 3);
    }
    // -- 17: empty map ---------------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(make_string_int_map_codec());
        std::map<std::string, int32_t> value = {};
        auto j = fmt.serialize(value);
        assert(B::is_object(j));
        auto d = fmt.deserialize(j);
        assert(d && d->empty());
    }
    // -- 18: map with struct values -> JSON object of arrays --------------
    {
        auto fmt = mu::make_json_format<B>(make_string_point_map_codec());
        std::map<std::string, point> value = {{"origin", {0, 0}}, {"target", {10, 20}}};
        auto j = fmt.serialize(value);
        assert(B::is_object(j));
        const auto& origin = B::object_get(j, "origin");
        assert(B::is_array(origin));
        assert(B::as_int64(B::array_at(origin, 0)) == 0);
        assert(B::as_int64(B::array_at(origin, 1)) == 0);
        const auto& target = B::object_get(j, "target");
        assert(B::as_int64(B::array_at(target, 0)) == 10);
        assert(B::as_int64(B::array_at(target, 1)) == 20);
        auto d = fmt.deserialize(j);
        assert(d && d->size() == 2);
        assert(d->at("origin").x == 0 && d->at("origin").y == 0);
        assert(d->at("target").x == 10 && d->at("target").y == 20);
    }
    // -- 19: optional<string> with value -> JSON string -------------------
    {
        auto fmt = mu::make_json_format<B>(mu::optional_codec(mu::string_codec));
        std::optional<std::string> value = "hello";
        auto j = fmt.serialize(value);
        assert(B::is_string(j) && B::as_string(j) == "hello");
        auto d = fmt.deserialize(j);
        assert(d && d->has_value() && **d == "hello");
    }
    // -- 20: optional<string> nullopt -> JSON null ------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::optional_codec(mu::string_codec));
        std::optional<std::string> value = std::nullopt;
        auto j = fmt.serialize(value);
        assert(B::is_null(j));
        auto d = fmt.deserialize(j);
        assert(d && !d->has_value());
    }
    // -- 21: vector<string> -> JSON array of strings ----------------------
    {
        auto fmt = mu::make_json_format<B>(mu::vector_of(mu::string_codec));
        std::vector<std::string> value = {"foo", "bar", "baz"};
        auto j = fmt.serialize(value);
        assert(B::is_array(j) && B::array_size(j) == 3);
        assert(B::as_string(B::array_at(j, 0)) == "foo");
        assert(B::as_string(B::array_at(j, 1)) == "bar");
        assert(B::as_string(B::array_at(j, 2)) == "baz");
        auto d = fmt.deserialize(j);
        assert(d && d->size() == 3 && (*d)[0] == "foo" && (*d)[1] == "bar");
    }
    // -- 22: vector<optional<int>> ---------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::vector_of(mu::optional_codec(mu::int32_codec)));
        std::vector<std::optional<int32_t>> value = {1, std::nullopt, 3};
        auto j = fmt.serialize(value);
        assert(B::is_array(j) && B::array_size(j) == 3);
        assert(B::is_integer(B::array_at(j, 0)) && B::as_int64(B::array_at(j, 0)) == 1);
        assert(B::is_null(B::array_at(j, 1)));
        assert(B::is_integer(B::array_at(j, 2)) && B::as_int64(B::array_at(j, 2)) == 3);
        auto d = fmt.deserialize(j);
        assert(d && d->size() == 3);
        assert((*d)[0].has_value() && *(*d)[0] == 1);
        assert(!(*d)[1].has_value());
        assert((*d)[2].has_value() && *(*d)[2] == 3);
    }
    // -- 23: empty vector ------------------------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::vector_of(mu::int32_codec));
        std::vector<int32_t> value = {};
        auto j = fmt.serialize(value);
        assert(B::is_array(j) && B::array_size(j) == 0);
        auto d = fmt.deserialize(j);
        assert(d && d->empty());
    }
    // -- 24: varint (signed) -> JSON integer ------------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::signed_varint_codec);
        auto j = fmt.serialize(std::intmax_t{-42});
        assert(B::is_integer(j) && B::as_int64(j) == -42);
        auto d = fmt.deserialize(j);
        assert(d && *d == -42);
    }
    // -- 25: varint (unsigned) -> JSON integer ----------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::unsigned_varint_codec);
        auto j = fmt.serialize(std::uintmax_t{12345});
        assert(B::is_integer(j) && B::as_uint64(j) == 12345);
        auto d = fmt.deserialize(j);
        assert(d && *d == 12345);
    }

    // =====================================================================
    // array_codec tests
    // =====================================================================

    // -- 26: array<int32_t, 3> -> JSON array (no size prefix) ------------
    {
        auto fmt = mu::make_json_format<B>(mu::array_of<3>(mu::int32_codec));
        std::array<int32_t, 3> value = {10, 20, 30};
        auto j = fmt.serialize(value);
        assert(B::is_array(j));
        assert(B::array_size(j) == 3);
        assert(B::as_int64(B::array_at(j, 0)) == 10);
        assert(B::as_int64(B::array_at(j, 1)) == 20);
        assert(B::as_int64(B::array_at(j, 2)) == 30);
        auto d = fmt.deserialize(j);
        assert(d && (*d)[0] == 10 && (*d)[1] == 20 && (*d)[2] == 30);
    }
    // -- 27: array<string, 2> -> JSON array of strings -------------------
    {
        auto fmt = mu::make_json_format<B>(mu::array_of<2>(mu::string_codec));
        std::array<std::string, 2> value = {"hello", "world"};
        auto j = fmt.serialize(value);
        assert(B::is_array(j) && B::array_size(j) == 2);
        assert(B::as_string(B::array_at(j, 0)) == "hello");
        assert(B::as_string(B::array_at(j, 1)) == "world");
        auto d = fmt.deserialize(j);
        assert(d && (*d)[0] == "hello" && (*d)[1] == "world");
    }
    // -- 28: array<int32_t, 1> -> single-element JSON array --------------
    {
        auto fmt = mu::make_json_format<B>(mu::array_of<1>(mu::int32_codec));
        std::array<int32_t, 1> value = {42};
        auto j = fmt.serialize(value);
        assert(B::is_array(j) && B::array_size(j) == 1);
        assert(B::as_int64(B::array_at(j, 0)) == 42);
        auto d = fmt.deserialize(j);
        assert(d && (*d)[0] == 42);
    }

    // =====================================================================
    // nullable_codec tests (shared_ptr via make_nullable)
    // =====================================================================

    // -- 29: shared_ptr<int32_t> with value -> JSON integer ---------------
    {
        auto fmt = mu::make_json_format<B>(mu::make_nullable<std::shared_ptr>(mu::int32_codec));
        std::shared_ptr<int32_t> ptr = std::make_shared<int32_t>(42);
        auto j = fmt.serialize(ptr);
        // With fixed transform_codec: transform produces optional, which json_format
        // natively handles as integer/null (not variant object)
        assert(B::is_integer(j) && B::as_int64(j) == 42);
        auto d = fmt.deserialize(j);
        assert(d && *d && **d == 42);
    }
    // -- 30: shared_ptr<int32_t> null -> JSON null -------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::make_nullable<std::shared_ptr>(mu::int32_codec));
        std::shared_ptr<int32_t> ptr;
        auto j = fmt.serialize(ptr);
        assert(B::is_null(j));
        auto d = fmt.deserialize(j);
        assert(d && !*d);
    }
    // -- 31: shared_ptr<string> with value -> JSON string ------------------
    {
        auto fmt = mu::make_json_format<B>(mu::make_nullable<std::shared_ptr>(mu::string_codec));
        auto ptr = std::make_shared<std::string>("hello");
        auto j = fmt.serialize(ptr);
        assert(B::is_string(j) && B::as_string(j) == "hello");
        auto d = fmt.deserialize(j);
        assert(d && *d && **d == "hello");
    }
    // -- 32: shared_ptr<string> null -> JSON null --------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::make_nullable<std::shared_ptr>(mu::string_codec));
        std::shared_ptr<std::string> ptr;
        auto j = fmt.serialize(ptr);
        assert(B::is_null(j));
        auto d = fmt.deserialize(j);
        assert(d && !*d);
    }

    // =====================================================================
    // constrained_codec + json_format integration
    // =====================================================================

    // -- 33: constrained int -> JSON integer (constraint is transparent) --
    {
        auto codec = mu::int32_codec.constrain([](int32_t v) { return v > 0; });
        auto fmt = mu::make_json_format<B>(codec);
        auto j = fmt.serialize(int32_t{99});
        assert(B::is_integer(j) && B::as_int64(j) == 99);
        auto d = fmt.deserialize(j);
        assert(d && *d == 99);
    }
    // -- 34: chained constraints -> JSON integer -------------------------
    {
        auto codec = mu::int32_codec
            .constrain([](int32_t v) { return v > 0; })
            .constrain([](int32_t v) { return v < 1000; });
        auto fmt = mu::make_json_format<B>(codec);
        auto j = fmt.serialize(int32_t{500});
        assert(B::is_integer(j) && B::as_int64(j) == 500);
        auto d = fmt.deserialize(j);
        assert(d && *d == 500);
    }
    // -- 35: constrained string -> JSON string ---------------------------
    {
        auto codec = mu::string_codec.constrain([](const std::string& s) { return !s.empty(); });
        auto fmt = mu::make_json_format<B>(codec);
        auto j = fmt.serialize(std::string("test"));
        assert(B::is_string(j) && B::as_string(j) == "test");
        auto d = fmt.deserialize(j);
        assert(d && *d == "test");
    }

    // =====================================================================
    // transform_codec + json_format integration
    // =====================================================================

    // -- 36: transform int -> json_format natively handles int, bypassing
    //        the transform. Round-trip preserves the original value.
    {
        auto codec = mu::int32_codec.transform(
            [](int32_t v) { return v * 2; },
            [](int32_t v) { return v / 2; }
        );
        auto fmt = mu::make_json_format<B>(codec);
        auto j = fmt.serialize(int32_t{21});
        // json_format natively handles int32_t, so the value is NOT doubled
        assert(B::is_integer(j) && B::as_int64(j) == 21);
        auto d = fmt.deserialize(j);
        assert(d && *d == 21);
    }
    // -- 37: transform string -> also natively handled, round-trip works
    {
        auto codec = mu::string_codec.transform(
            [](const std::string& s) { return "pfx_" + s; },
            [](const std::string& s) { return s.substr(4); }
        );
        auto fmt = mu::make_json_format<B>(codec);
        std::string val = "data";
        auto j = fmt.serialize(val);
        assert(B::is_string(j) && B::as_string(j) == "data");
        auto d = fmt.deserialize(j);
        assert(d && *d == "data");
    }

    // =====================================================================
    // Error / malformed JSON deserialization tests
    // =====================================================================

    // -- 38: int codec given a string -> nullopt -------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::int32_codec);
        auto node = B::make_string("not a number");
        auto d = fmt.deserialize(node);
        assert(!d.has_value());
    }
    // -- 39: int codec given a bool -> nullopt ---------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::int32_codec);
        auto node = B::make_bool(true);
        auto d = fmt.deserialize(node);
        assert(!d.has_value());
    }
    // -- 40: string codec given an integer -> nullopt --------------------
    {
        auto fmt = mu::make_json_format<B>(mu::string_codec);
        auto node = B::make_int(42);
        auto d = fmt.deserialize(node);
        assert(!d.has_value());
    }
    // -- 41: bool codec given a string -> nullopt ------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::bool_codec);
        auto node = B::make_string("true");
        auto d = fmt.deserialize(node);
        assert(!d.has_value());
    }
    // -- 42: float codec given a string -> nullopt -----------------------
    {
        auto fmt = mu::make_json_format<B>(mu::float_codec);
        auto node = B::make_string("3.14");
        auto d = fmt.deserialize(node);
        assert(!d.has_value());
    }
    // -- 43: tuple with too few elements -> nullopt ----------------------
    {
        auto codec = mu::tuple_codec(mu::int32_codec, mu::float_codec, mu::bool_codec);
        auto fmt = mu::make_json_format<B>(codec);
        // Only 2 elements instead of 3
        auto node = B::make_array();
        B::array_push_back(node, B::make_int(1));
        B::array_push_back(node, B::make_float(2.0));
        auto d = fmt.deserialize(node);
        assert(!d.has_value());
    }
    // -- 44: tuple given non-array -> nullopt ----------------------------
    {
        auto codec = mu::tuple_codec(mu::int32_codec, mu::float_codec);
        auto fmt = mu::make_json_format<B>(codec);
        auto d = fmt.deserialize(B::make_int(42));
        assert(!d.has_value());
    }
    // -- 45: variant with invalid index -> nullopt -----------------------
    {
        auto fmt = mu::make_json_format<B>(mu::variant_codec(mu::int32_codec, mu::bool_codec));
        auto node = B::make_object();
        B::object_set(node, "i", B::make_uint(99));
        B::object_set(node, "v", B::make_int(1));
        auto d = fmt.deserialize(node);
        assert(!d.has_value());
    }
    // -- 46: variant missing "i" key -> nullopt --------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::variant_codec(mu::int32_codec, mu::bool_codec));
        auto node = B::make_object();
        B::object_set(node, "v", B::make_int(1));
        auto d = fmt.deserialize(node);
        assert(!d.has_value());
    }
    // -- 47: variant missing "v" key -> nullopt --------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::variant_codec(mu::int32_codec, mu::bool_codec));
        auto node = B::make_object();
        B::object_set(node, "i", B::make_uint(0));
        auto d = fmt.deserialize(node);
        assert(!d.has_value());
    }
    // -- 48: variant given non-object -> nullopt -------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::variant_codec(mu::int32_codec, mu::bool_codec));
        auto d = fmt.deserialize(B::make_int(42));
        assert(!d.has_value());
    }
    // -- 49: vector given non-array -> nullopt ---------------------------
    {
        auto fmt = mu::make_json_format<B>(mu::vector_of(mu::int32_codec));
        auto d = fmt.deserialize(B::make_string("not an array"));
        assert(!d.has_value());
    }
    // -- 50: map given non-object -> nullopt -----------------------------
    {
        auto fmt = mu::make_json_format<B>(make_string_int_map_codec());
        auto d = fmt.deserialize(B::make_array());
        assert(!d.has_value());
    }
    // -- 51: optional given wrong inner type -> nullopt -------------------
    {
        auto fmt = mu::make_json_format<B>(mu::optional_codec(mu::int32_codec));
        auto node = B::make_string("not a number");
        auto d = fmt.deserialize(node);
        assert(!d.has_value());
    }
    // -- 52: struct given non-array -> nullopt ----------------------------
    {
        auto fmt = mu::make_json_format<B>(point::codec);
        auto d = fmt.deserialize(B::make_string("oops"));
        assert(!d.has_value());
    }

    std::cout << "All json_format tests passed!" << std::endl;
    return 0;
}
