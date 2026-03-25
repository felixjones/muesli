#include <muesli/format/binary_format>
#include <muesli/codecs>

#include <sstream>
#include <cassert>
#include <string>
#include <optional>
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

template<typename Fmt, typename Stream>
auto from_stream(const Fmt& fmt, Stream& stream) {
    return fmt.deserialize(stream);
}

int main() {
    // =====================================================================
    // 1. Fundamental types
    // =====================================================================

    // -- 1a: int round-trip
    {
        auto fmt = mu::make_binary_format<char>(mu::int_codec);
        auto bytes = to_bytes(fmt, 77);
        auto d = from_bytes(fmt, bytes);
        assert(d && *d == 77);
    }
    // -- 1b: float round-trip
    {
        auto fmt = mu::make_binary_format<char>(mu::float_codec);
        auto bytes = to_bytes(fmt, 3.14f);
        auto d = from_bytes(fmt, bytes);
        assert(d && *d == 3.14f);
    }
    // -- 1c: double round-trip
    {
        auto fmt = mu::make_binary_format<char>(mu::double_codec);
        auto bytes = to_bytes(fmt, 2.718281828);
        auto d = from_bytes(fmt, bytes);
        assert(d && *d == 2.718281828);
    }
    // -- 1d: bool round-trip
    {
        auto fmt = mu::make_binary_format<char>(mu::bool_codec);
        auto bt = to_bytes(fmt, true);
        auto bf = to_bytes(fmt, false);
        assert(from_bytes(fmt, bt) && *from_bytes(fmt, bt) == true);
        assert(from_bytes(fmt, bf) && *from_bytes(fmt, bf) == false);
    }
    // -- 1e: int32 round-trip
    {
        auto fmt = mu::make_binary_format<char>(mu::int32_codec);
        auto bytes = to_bytes(fmt, std::int32_t{-42});
        auto d = from_bytes(fmt, bytes);
        assert(d && *d == -42);
    }
    // -- 1f: uint64 round-trip
    {
        auto fmt = mu::make_binary_format<char>(mu::uint64_codec);
        auto bytes = to_bytes(fmt, std::uint64_t{999999});
        auto d = from_bytes(fmt, bytes);
        assert(d && *d == 999999u);
    }

    // =====================================================================
    // 2. String codec (null-terminated via delimited_codec)
    // =====================================================================

    // -- 2a: basic string round-trip
    {
        auto fmt = mu::make_binary_format<char>(mu::string_codec);
        auto bytes = to_bytes(fmt, std::string{"hello"});
        auto d = from_bytes(fmt, bytes);
        assert(d && *d == "hello");
    }
    // -- 2b: empty string round-trip
    {
        auto fmt = mu::make_binary_format<char>(mu::string_codec);
        auto bytes = to_bytes(fmt, std::string{""});
        auto d = from_bytes(fmt, bytes);
        assert(d && d->empty());
    }
    // -- 2c: string with special characters
    {
        auto fmt = mu::make_binary_format<char>(mu::string_codec);
        auto bytes = to_bytes(fmt, std::string{"line1\nline2\ttab"});
        auto d = from_bytes(fmt, bytes);
        assert(d && *d == "line1\nline2\ttab");
    }

    // =====================================================================
    // 3. Tuple codecs (including string ordering)
    // =====================================================================

    // -- 3a: tuple<int, float> round-trip
    {
        auto codec = mu::tuple_codec(mu::int32_codec, mu::float_codec);
        auto fmt = mu::make_binary_format<char>(codec);
        auto val = std::tuple{std::int32_t{42}, 3.14f};
        auto bytes = to_bytes(fmt, val);
        auto d = from_bytes(fmt, bytes);
        assert(d && std::get<0>(*d) == 42 && std::get<1>(*d) == 3.14f);
    }
    // -- 3b: tuple<string, int> — string BEFORE int (tests sequential consumption)
    {
        auto codec = mu::tuple_codec(mu::string_codec, mu::int32_codec);
        auto fmt = mu::make_binary_format<char>(codec);
        auto val = std::tuple{std::string{"hello"}, std::int32_t{99}};
        auto bytes = to_bytes(fmt, val);
        auto d = from_bytes(fmt, bytes);
        assert(d);
        assert(std::get<0>(*d) == "hello");
        assert(std::get<1>(*d) == 99);
    }
    // -- 3c: tuple<int, string> — int before string
    {
        auto codec = mu::tuple_codec(mu::int32_codec, mu::string_codec);
        auto fmt = mu::make_binary_format<char>(codec);
        auto val = std::tuple{std::int32_t{-7}, std::string{"world"}};
        auto bytes = to_bytes(fmt, val);
        auto d = from_bytes(fmt, bytes);
        assert(d && std::get<0>(*d) == -7 && std::get<1>(*d) == "world");
    }
    // -- 3d: tuple<string, string, int> — two strings before int
    {
        auto codec = mu::tuple_codec(mu::string_codec, mu::string_codec, mu::int32_codec);
        auto fmt = mu::make_binary_format<char>(codec);
        auto val = std::tuple{std::string{"alpha"}, std::string{"beta"}, std::int32_t{42}};
        auto bytes = to_bytes(fmt, val);
        auto d = from_bytes(fmt, bytes);
        assert(d);
        assert(std::get<0>(*d) == "alpha");
        assert(std::get<1>(*d) == "beta");
        assert(std::get<2>(*d) == 42);
    }
    // -- 3e: tuple<string, int, string> — interleaved
    {
        auto codec = mu::tuple_codec(mu::string_codec, mu::int32_codec, mu::string_codec);
        auto fmt = mu::make_binary_format<char>(codec);
        auto val = std::tuple{std::string{"first"}, std::int32_t{0}, std::string{"last"}};
        auto bytes = to_bytes(fmt, val);
        auto d = from_bytes(fmt, bytes);
        assert(d);
        assert(std::get<0>(*d) == "first");
        assert(std::get<1>(*d) == 0);
        assert(std::get<2>(*d) == "last");
    }
    // -- 3f: tuple<string, uint64> sequential reads from one stream
    {
        auto codec = mu::tuple_codec(mu::string_codec, mu::uint64_codec);
        auto fmt = mu::make_binary_format<char>(codec);

        auto first = std::tuple{std::string{"alpha"}, std::uint64_t{11}};
        auto second = std::tuple{std::string{"beta"}, std::uint64_t{22}};

        std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
        assert(fmt.serialize(first, ss));
        assert(fmt.serialize(second, ss));

        ss.seekg(0);
        auto d0 = from_stream(fmt, ss);
        auto d1 = from_stream(fmt, ss);

        assert(d0 && std::get<0>(*d0) == "alpha" && std::get<1>(*d0) == 11u);
        assert(d1 && std::get<0>(*d1) == "beta" && std::get<1>(*d1) == 22u);
    }

    // =====================================================================
    // 4. Vector codecs
    // =====================================================================

    // -- 4a: vector<int> round-trip
    {
        auto fmt = mu::make_binary_format<char>(mu::vector_of(mu::int32_codec));
        std::vector<std::int32_t> val = {1, 2, 3, 4, 5};
        auto bytes = to_bytes(fmt, val);
        auto d = from_bytes(fmt, bytes);
        assert(d && *d == val);
    }
    // -- 4b: empty vector
    {
        auto fmt = mu::make_binary_format<char>(mu::vector_of(mu::int32_codec));
        std::vector<std::int32_t> val = {};
        auto bytes = to_bytes(fmt, val);
        auto d = from_bytes(fmt, bytes);
        assert(d && d->empty());
    }
    // -- 4c: vector<string>
    {
        auto fmt = mu::make_binary_format<char>(mu::vector_of(mu::string_codec));
        std::vector<std::string> val = {"alpha", "beta", "gamma"};
        auto bytes = to_bytes(fmt, val);
        auto d = from_bytes(fmt, bytes);
        assert(d && d->size() == 3);
        assert((*d)[0] == "alpha" && (*d)[1] == "beta" && (*d)[2] == "gamma");
    }
    // -- 4d: vector<double>
    {
        auto fmt = mu::make_binary_format<char>(mu::vector_of(mu::double_codec));
        std::vector<double> val = {1.1, 2.2, 3.3};
        auto bytes = to_bytes(fmt, val);
        auto d = from_bytes(fmt, bytes);
        assert(d && *d == val);
    }
    // -- 4e: tuple<vector<string>, uint64> keeps stream ordering intact
    {
        auto codec = mu::tuple_codec(mu::vector_of(mu::string_codec), mu::uint64_codec);
        auto fmt = mu::make_binary_format<char>(codec);

        auto val = std::tuple{std::vector<std::string>{"x", "yy", "zzz"}, std::uint64_t{987654321}};
        auto bytes = to_bytes(fmt, val);
        auto d = from_bytes(fmt, bytes);

        assert(d);
        assert(std::get<0>(*d).size() == 3);
        assert(std::get<0>(*d)[0] == "x");
        assert(std::get<0>(*d)[1] == "yy");
        assert(std::get<0>(*d)[2] == "zzz");
        assert(std::get<1>(*d) == 987654321u);
    }
    // -- 4f: truncated vector payload fails cleanly
    {
        auto fmt = mu::make_binary_format<char>(mu::vector_of(mu::int32_codec));
        std::vector<std::int32_t> val = {10, 20, 30};
        auto bytes = to_bytes(fmt, val);
        bytes.resize(bytes.size() - sizeof(std::int32_t));
        auto d = from_bytes(fmt, bytes);
        assert(!d);
    }

    // =====================================================================
    // 5. Variant codecs (including string alternatives)
    // =====================================================================

    // -- 5a: variant<int, string> — int alternative
    {
        auto codec = mu::variant_codec(mu::int32_codec, mu::string_codec);
        auto fmt = mu::make_binary_format<char>(codec);
        std::variant<std::int32_t, std::string> val = std::int32_t{42};
        auto bytes = to_bytes(fmt, val);
        auto d = from_bytes(fmt, bytes);
        assert(d && d->index() == 0 && std::get<0>(*d) == 42);
    }
    // -- 5b: variant<int, string> — string alternative
    {
        auto codec = mu::variant_codec(mu::int32_codec, mu::string_codec);
        auto fmt = mu::make_binary_format<char>(codec);
        std::variant<std::int32_t, std::string> val = std::string{"hello"};
        auto bytes = to_bytes(fmt, val);
        auto d = from_bytes(fmt, bytes);
        assert(d && d->index() == 1 && std::get<1>(*d) == "hello");
    }
    // -- 5c: variant<bool, double, string> — all alternatives
    {
        auto codec = mu::variant_codec(mu::bool_codec, mu::double_codec, mu::string_codec);
        auto fmt = mu::make_binary_format<char>(codec);

        using var_t = std::variant<bool, double, std::string>;

        var_t v0 = true;
        auto d0 = from_bytes(fmt, to_bytes(fmt, v0));
        assert(d0 && std::get<0>(*d0) == true);

        var_t v1 = 9.99;
        auto d1 = from_bytes(fmt, to_bytes(fmt, v1));
        assert(d1 && std::get<1>(*d1) == 9.99);

        var_t v2 = std::string{"world"};
        auto d2 = from_bytes(fmt, to_bytes(fmt, v2));
        assert(d2 && std::get<2>(*d2) == "world");
    }
    // -- 5d: tuple<variant<int, string>, uint64> with string alternative
    {
        auto codec = mu::tuple_codec(mu::variant_codec(mu::int32_codec, mu::string_codec), mu::uint64_codec);
        auto fmt = mu::make_binary_format<char>(codec);

        auto val = std::tuple{std::variant<std::int32_t, std::string>{std::string{"variant-string"}}, std::uint64_t{55}};
        auto bytes = to_bytes(fmt, val);
        auto d = from_bytes(fmt, bytes);

        assert(d);
        assert(std::get<0>(*d).index() == 1);
        assert(std::get<1>(std::get<0>(*d)) == "variant-string");
        assert(std::get<1>(*d) == 55u);
    }
    // -- 5e: sequential variant decode from one stream (string then int)
    {
        auto codec = mu::variant_codec(mu::int32_codec, mu::string_codec);
        auto fmt = mu::make_binary_format<char>(codec);

        std::variant<std::int32_t, std::string> first = std::string{"hello"};
        std::variant<std::int32_t, std::string> second = std::int32_t{77};

        std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
        assert(fmt.serialize(first, ss));
        assert(fmt.serialize(second, ss));

        ss.seekg(0);
        auto d0 = from_stream(fmt, ss);
        auto d1 = from_stream(fmt, ss);

        assert(d0 && d0->index() == 1 && std::get<1>(*d0) == "hello");
        assert(d1 && d1->index() == 0 && std::get<0>(*d1) == 77);
    }
    // -- 5f: truncated variant payload fails cleanly
    {
        auto codec = mu::variant_codec(mu::int32_codec, mu::string_codec);
        auto fmt = mu::make_binary_format<char>(codec);
        std::variant<std::int32_t, std::string> val = std::int32_t{456};
        auto bytes = to_bytes(fmt, val);
        bytes.resize(bytes.size() - 1);
        auto d = from_bytes(fmt, bytes);
        assert(!d);
    }

    // =====================================================================
    // 6. Optional codec
    // =====================================================================

    // -- 6a: optional<string> present
    {
        auto fmt = mu::make_binary_format<char>(mu::optional_codec(mu::string_codec));
        std::optional<std::string> val{"present"};
        auto bytes = to_bytes(fmt, val);
        auto d = from_bytes(fmt, bytes);
        assert(d && d->has_value() && **d == "present");
    }
    // -- 6b: optional<string> absent
    {
        auto fmt = mu::make_binary_format<char>(mu::optional_codec(mu::string_codec));
        std::optional<std::string> val;
        auto bytes = to_bytes(fmt, val);
        auto d = from_bytes(fmt, bytes);
        assert(d && !d->has_value());
    }

    // =====================================================================
    // 7. Nested compositions
    // =====================================================================

    // -- 7a: vector<optional<string>>
    {
        auto fmt = mu::make_binary_format<char>(mu::vector_of(mu::optional_codec(mu::string_codec)));
        std::vector<std::optional<std::string>> val = {"a", std::nullopt, "c"};
        auto bytes = to_bytes(fmt, val);
        auto d = from_bytes(fmt, bytes);
        assert(d && d->size() == 3);
        assert((*d)[0].has_value() && *(*d)[0] == "a");
        assert(!(*d)[1].has_value());
        assert((*d)[2].has_value() && *(*d)[2] == "c");
    }

    // =====================================================================
    // 8. Struct codec (apply_codec) with strings
    // =====================================================================

    // -- 8a: point-like struct with int fields
    {
        struct point {
            int x, y;
        };
        auto codec = mu::tuple_codec(
            mu::int_codec.member<&point::x>().named("x"),
            mu::int_codec.member<&point::y>().named("y")
        ).apply<point>();
        auto fmt = mu::make_binary_format<char>(codec);
        point val{10, 20};
        auto bytes = to_bytes(fmt, val);
        auto d = from_bytes(fmt, bytes);
        assert(d && d->x == 10 && d->y == 20);
    }
    // -- 8b: person-like struct with string field
    {
        struct person {
            int age;
            std::string name;
        };
        auto codec = mu::tuple_codec(
            mu::int_codec.member<&person::age>().named("age"),
            mu::string_codec.member<&person::name>().named("name")
        ).apply<person>();
        auto fmt = mu::make_binary_format<char>(codec);
        person val{30, "Alice"};
        auto bytes = to_bytes(fmt, val);
        auto d = from_bytes(fmt, bytes);
        assert(d && d->age == 30 && d->name == "Alice");
    }

    return 0;
}
