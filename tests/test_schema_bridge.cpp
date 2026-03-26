/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/schema/extract>
#include <muesli/schema/make_codec>
#include <muesli/schema/adapter_codec>
#include <muesli/format/binary_format>
#include <muesli/codecs>

#include <cassert>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace mu = muesli;

// -- Test structs -----------------------------------------------------------

struct point {
    int x, y;

    bool operator==(const point& o) const { return x == o.x && y == o.y; }

    static constexpr auto codec = mu::tuple_codec(
        mu::int_codec.member<&point::x>().named("x"),
        mu::int_codec.member<&point::y>().named("y")
    ).apply<point>();
};

struct person {
    int age;
    std::string name;

    bool operator==(const person& o) const { return age == o.age && name == o.name; }

    static constexpr auto codec = mu::tuple_codec(
        mu::int_codec.member<&person::age>().named("age"),
        mu::string_codec.member<&person::name>().named("name")
    ).apply<person>();
};

struct profile {
    std::string username;
    std::optional<std::string> bio;

    bool operator==(const profile& o) const { return username == o.username && bio == o.bio; }

    static constexpr auto codec = mu::tuple_codec(
        mu::string_codec.member<&profile::username>().named("username"),
        mu::optional_codec(mu::string_codec).member<&profile::bio>().named("bio")
    ).apply<profile>();
};

struct address {
    std::string city;
    std::string country;

    bool operator==(const address& o) const { return city == o.city && country == o.country; }

    static constexpr auto codec = mu::tuple_codec(
        mu::string_codec.member<&address::city>().named("city"),
        mu::string_codec.member<&address::country>().named("country")
    ).apply<address>();
};

struct located_person {
    int age;
    std::string name;
    point location;

    bool operator==(const located_person& o) const {
        return age == o.age && name == o.name && location == o.location;
    }

    static constexpr auto codec = mu::tuple_codec(
        mu::int_codec.member<&located_person::age>().named("age"),
        mu::string_codec.member<&located_person::name>().named("name"),
        point::codec.member<&located_person::location>().named("location")
    ).apply<located_person>();
};

// -- Helpers ----------------------------------------------------------------

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

/// Round-trip through the schema adapter: encode typed → agnostic,
/// decode agnostic → typed, assert equality.
template<mu::Codec OrigCodec, typename T>
void assert_bridge_round_trip(const OrigCodec& orig_codec, const T& value) {
    auto schema = mu::make_schema(orig_codec);
    auto schema_codec = mu::make_codec(schema);
    auto adapter = mu::make_schema_adapter(orig_codec, schema_codec);

    // typed → agnostic via binary bridge
    auto agnostic = adapter.encode(value);
    assert(agnostic.has_value());

    // agnostic → typed via binary bridge
    auto result = adapter.decode(*agnostic);
    assert(result.has_value());
    assert(*result == value);
}

int main() {
    // =====================================================================
    // 1. Point: two int fields
    // =====================================================================
    {
        assert_bridge_round_trip(point::codec, point{42, -100});
    }

    // =====================================================================
    // 2. Point with zeros
    // =====================================================================
    {
        assert_bridge_round_trip(point::codec, point{0, 0});
    }

    // =====================================================================
    // 3. Point with negative values
    // =====================================================================
    {
        assert_bridge_round_trip(point::codec, point{-50, -75});
    }

    // =====================================================================
    // 4. Person (int + string)
    // =====================================================================
    {
        assert_bridge_round_trip(person::codec, person{30, "Alice"});
    }

    // =====================================================================
    // 5. Person with empty string
    // =====================================================================
    {
        assert_bridge_round_trip(person::codec, person{0, ""});
    }

    // =====================================================================
    // 6. Profile: optional present
    // =====================================================================
    {
        assert_bridge_round_trip(profile::codec, profile{"alice", "A bio"});
    }

    // =====================================================================
    // 7. Profile: optional absent
    // =====================================================================
    {
        assert_bridge_round_trip(profile::codec, profile{"bob", std::nullopt});
    }

    // =====================================================================
    // 8. Address (two string fields)
    // =====================================================================
    {
        assert_bridge_round_trip(address::codec, address{"London", "UK"});
    }

    // =====================================================================
    // 9. Nested struct: located_person (embeds point)
    //    NOTE: For nested structs, std::apply cannot recursively convert
    //    inner tuples to inner structs without C++ reflection.  The binary
    //    format handles nesting correctly (each field is deserialized
    //    individually through the codec chain).  Here we verify the raw
    //    schema codec still faithfully round-trips the bytes.
    // =====================================================================
    {
        auto fmt_orig = mu::make_binary_format<char>(located_person::codec);
        located_person lp{25, "Bob", {3, 4}};
        auto bytes = to_bytes(fmt_orig, lp);

        constexpr auto schema = mu::make_schema(located_person::codec);
        constexpr auto schema_codec = mu::make_codec(schema);
        auto fmt_schema = mu::make_binary_format<char>(schema_codec);
        auto schema_result = from_bytes(fmt_schema, bytes);
        assert(schema_result.has_value());
        // Schema value is tuple<int, string, tuple<int, int>>
        assert(std::get<0>(*schema_result) == 25);
        assert(std::get<1>(*schema_result) == "Bob");
        assert(std::get<0>(std::get<2>(*schema_result)) == 3);
        assert(std::get<1>(std::get<2>(*schema_result)) == 4);
    }

    // =====================================================================
    // 10. Vector of persons: byte-level verification
    // =====================================================================
    {
        constexpr auto vec_person_codec = mu::vector_of(person::codec);
        auto fmt_orig = mu::make_binary_format<char>(vec_person_codec);
        std::vector<person> val = {{25, "Alice"}, {30, "Bob"}};
        auto bytes = to_bytes(fmt_orig, val);

        constexpr auto schema = mu::make_schema(vec_person_codec);
        constexpr auto schema_codec = mu::make_codec(schema);
        auto fmt_schema = mu::make_binary_format<char>(schema_codec);
        auto schema_result = from_bytes(fmt_schema, bytes);
        assert(schema_result.has_value());
        assert(schema_result->size() == 2);
        assert(std::get<0>((*schema_result)[0]) == 25);
        assert(std::get<1>((*schema_result)[0]) == "Alice");
        assert(std::get<0>((*schema_result)[1]) == 30);
        assert(std::get<1>((*schema_result)[1]) == "Bob");
    }

    // =====================================================================
    // 11. Contact-like: complex tuple byte-level (string, int32, opt,
    //     nested address, vector<string>)
    // =====================================================================
    {
        constexpr auto contact_codec = mu::tuple_codec(
            mu::string_codec,
            mu::int32_codec,
            mu::optional_codec(mu::string_codec),
            address::codec,
            mu::vector_of(mu::string_codec)
        );

        auto fmt_orig = mu::make_binary_format<char>(contact_codec);
        auto val = std::make_tuple(
            std::string{"Michael"}, std::int32_t{30},
            std::optional<std::string>{"m@example.com"},
            address{"London", "UK"},
            std::vector<std::string>{"dev", "film"}
        );
        auto bytes = to_bytes(fmt_orig, val);

        constexpr auto schema = mu::make_schema(contact_codec);
        constexpr auto schema_codec = mu::make_codec(schema);
        auto fmt_schema = mu::make_binary_format<char>(schema_codec);
        auto schema_result = from_bytes(fmt_schema, bytes);
        assert(schema_result.has_value());
        assert(std::get<0>(*schema_result) == "Michael");
        assert(std::get<1>(*schema_result) == 30);
        assert(std::get<2>(*schema_result).has_value());
        assert(*std::get<2>(*schema_result) == "m@example.com");
        assert(std::get<0>(std::get<3>(*schema_result)) == "London");
        assert(std::get<1>(std::get<3>(*schema_result)) == "UK");
        auto& tags = std::get<4>(*schema_result);
        assert(tags.size() == 2);
        assert(tags[0] == "dev" && tags[1] == "film");
    }

    // =====================================================================
    // 12. Contact-like: absent optional (byte-level)
    // =====================================================================
    {
        constexpr auto contact_codec = mu::tuple_codec(
            mu::string_codec,
            mu::int32_codec,
            mu::optional_codec(mu::string_codec),
            address::codec,
            mu::vector_of(mu::string_codec)
        );

        auto fmt_orig = mu::make_binary_format<char>(contact_codec);
        auto val = std::make_tuple(
            std::string{"Pichael"}, std::int32_t{31},
            std::optional<std::string>{},
            address{"Bradford", "UK"},
            std::vector<std::string>{"con man"}
        );
        auto bytes = to_bytes(fmt_orig, val);

        constexpr auto schema = mu::make_schema(contact_codec);
        constexpr auto schema_codec = mu::make_codec(schema);
        auto fmt_schema = mu::make_binary_format<char>(schema_codec);
        auto schema_result = from_bytes(fmt_schema, bytes);
        assert(schema_result.has_value());
        assert(std::get<0>(*schema_result) == "Pichael");
        assert(std::get<1>(*schema_result) == 31);
        assert(!std::get<2>(*schema_result).has_value());
    }

    // =====================================================================
    // 13. Vector of points (struct elements) -- adapter convert
    // =====================================================================
    {
        constexpr auto codec = mu::vector_of(point::codec);
        std::vector<point> val = {{1, 2}, {3, 4}, {5, 6}};
        assert_bridge_round_trip(codec, val);
    }

    // =====================================================================
    // 14. Vector of persons (struct with string) -- adapter convert
    // =====================================================================
    {
        constexpr auto codec = mu::vector_of(person::codec);
        std::vector<person> val = {{25, "Alice"}, {30, "Bob"}};
        assert_bridge_round_trip(codec, val);
    }

    // =====================================================================
    // 15. Empty vector of points
    // =====================================================================
    {
        constexpr auto codec = mu::vector_of(point::codec);
        std::vector<point> val;
        assert_bridge_round_trip(codec, val);
    }

    // =====================================================================
    // 16. Optional of point: present
    // =====================================================================
    {
        constexpr auto codec = mu::optional_codec(point::codec);
        std::optional<point> val{point{5, 10}};
        assert_bridge_round_trip(codec, val);
    }

    // =====================================================================
    // 17. Optional of point: absent
    // =====================================================================
    {
        constexpr auto codec = mu::optional_codec(point::codec);
        std::optional<point> val{};
        assert_bridge_round_trip(codec, val);
    }

    // =====================================================================
    // 18. Variant with struct alternative: struct path
    // =====================================================================
    {
        constexpr auto codec = mu::variant_codec(mu::string_codec, point::codec);
        using var_t = std::variant<std::string, point>;
        var_t val = point{7, 8};
        assert_bridge_round_trip(codec, val);
    }

    // =====================================================================
    // 19. Variant with struct alternative: primitive path
    // =====================================================================
    {
        constexpr auto codec = mu::variant_codec(mu::string_codec, point::codec);
        using var_t = std::variant<std::string, point>;
        var_t val = std::string{"hello"};
        assert_bridge_round_trip(codec, val);
    }

    // =====================================================================
    // 20. Tuple containing a struct (byte-level)
    // =====================================================================
    {
        constexpr auto codec = mu::tuple_codec(mu::string_codec, point::codec);
        auto val = std::make_tuple(std::string{"origin"}, point{0, 0});
        auto fmt_orig = mu::make_binary_format<char>(codec);
        auto bytes = to_bytes(fmt_orig, val);

        constexpr auto schema = mu::make_schema(codec);
        constexpr auto schema_codec = mu::make_codec(schema);
        auto fmt_schema = mu::make_binary_format<char>(schema_codec);
        auto schema_result = from_bytes(fmt_schema, bytes);
        assert(schema_result.has_value());
        assert(std::get<0>(*schema_result) == "origin");
        assert(std::get<0>(std::get<1>(*schema_result)) == 0);
        assert(std::get<1>(std::get<1>(*schema_result)) == 0);
    }

    // =====================================================================
    // 21. Tuple of two different structs (byte-level)
    // =====================================================================
    {
        constexpr auto codec = mu::tuple_codec(point::codec, address::codec);
        auto val = std::make_tuple(point{1, 2}, address{"Paris", "FR"});
        auto fmt_orig = mu::make_binary_format<char>(codec);
        auto bytes = to_bytes(fmt_orig, val);

        constexpr auto schema = mu::make_schema(codec);
        constexpr auto schema_codec = mu::make_codec(schema);
        auto fmt_schema = mu::make_binary_format<char>(schema_codec);
        auto schema_result = from_bytes(fmt_schema, bytes);
        assert(schema_result.has_value());
        assert(std::get<0>(std::get<0>(*schema_result)) == 1);
        assert(std::get<1>(std::get<0>(*schema_result)) == 2);
        assert(std::get<0>(std::get<1>(*schema_result)) == "Paris");
        assert(std::get<1>(std::get<1>(*schema_result)) == "FR");
    }

    // =====================================================================
    // 22. Repeated round-trip: point
    // =====================================================================
    {
        point p{77, -33};
        for (int i = 0; i < 5; ++i) {
            assert_bridge_round_trip(point::codec, p);
        }
    }

    // =====================================================================
    // 23. Repeated round-trip: person
    // =====================================================================
    {
        person p{99, "Stable"};
        for (int i = 0; i < 5; ++i) {
            assert_bridge_round_trip(person::codec, p);
        }
    }

    // =====================================================================
    // 24. Repeated round-trip: profile (struct with optional)
    // =====================================================================
    {
        profile p{"Stable", std::optional<std::string>{"A bio"}};
        for (int i = 0; i < 5; ++i) {
            assert_bridge_round_trip(profile::codec, p);
        }
    }

    // =====================================================================
    // 25. Adapter encode: point → schema tuple
    // =====================================================================
    {
        constexpr auto schema = mu::make_schema(point::codec);
        constexpr auto schema_codec = mu::make_codec(schema);
        auto adapter = mu::make_schema_adapter(point::codec, schema_codec);

        auto encoded = adapter.encode(point{42, -100});
        assert(encoded.has_value());
        assert(std::get<0>(*encoded) == 42);
        assert(std::get<1>(*encoded) == -100);
    }

    // =====================================================================
    // 26. Adapter decode: schema tuple → person
    // =====================================================================
    {
        constexpr auto schema = mu::make_schema(person::codec);
        constexpr auto schema_codec = mu::make_codec(schema);
        auto adapter = mu::make_schema_adapter(person::codec, schema_codec);

        using schema_val_t = typename decltype(schema_codec)::value_type;
        schema_val_t schema_val{30, std::string{"Alice"}};

        auto result = adapter.decode(schema_val);
        assert(result.has_value());
        assert(*result == (person{30, "Alice"}));
    }

    // =====================================================================
    // 27. Adapter round-trip: point encode then decode
    // =====================================================================
    {
        constexpr auto schema = mu::make_schema(point::codec);
        constexpr auto schema_codec = mu::make_codec(schema);
        auto adapter = mu::make_schema_adapter(point::codec, schema_codec);

        auto encoded = adapter.encode(point{10, 20});
        assert(encoded.has_value());

        auto decoded = adapter.decode(*encoded);
        assert(decoded.has_value());
        assert(*decoded == (point{10, 20}));
    }

    // =====================================================================
    // 28. Adapter round-trip: person encode then decode
    // =====================================================================
    {
        constexpr auto schema = mu::make_schema(person::codec);
        constexpr auto schema_codec = mu::make_codec(schema);
        auto adapter = mu::make_schema_adapter(person::codec, schema_codec);

        auto encoded = adapter.encode(person{99, "Zara"});
        assert(encoded.has_value());

        auto decoded = adapter.decode(*encoded);
        assert(decoded.has_value());
        assert(*decoded == (person{99, "Zara"}));
    }

    // =====================================================================
    // 29. Adapter decode: point from manually-constructed schema value
    // =====================================================================
    {
        constexpr auto schema = mu::make_schema(point::codec);
        constexpr auto schema_codec = mu::make_codec(schema);
        auto adapter = mu::make_schema_adapter(point::codec, schema_codec);

        using schema_val_t = typename decltype(schema_codec)::value_type;
        schema_val_t schema_val{42, -100};

        auto result = adapter.decode(schema_val);
        assert(result.has_value());
        assert(*result == (point{42, -100}));
    }

    // =====================================================================
    // 30. Adapter with nested struct: located_person
    // =====================================================================
    {
        constexpr auto schema = mu::make_schema(located_person::codec);
        constexpr auto schema_codec = mu::make_codec(schema);
        auto adapter = mu::make_schema_adapter(located_person::codec, schema_codec);

        located_person lp{25, "Bob", {3, 4}};
        auto encoded = adapter.encode(lp);
        assert(encoded.has_value());

        auto decoded = adapter.decode(*encoded);
        assert(decoded.has_value());
        assert(*decoded == lp);
    }

    // =====================================================================
    // 31. Migration scenario: schema codec and original codec produce
    //     identical bytes, enabling type-agnostic deserialization followed
    //     by re-serialization and typed deserialization.
    //     (Only primitive/string/optional/vector fields -- no nested structs
    //      so both codecs share the same value_type.)
    // =====================================================================
    {
        // Define a contact codec (tuple of primitives only)
        constexpr auto contact_codec = mu::tuple_codec(
            mu::string_codec,
            mu::int32_codec,
            mu::optional_codec(mu::string_codec),
            mu::vector_of(mu::string_codec)
        );

        // The original typed value
        auto myContact = std::make_tuple(
            std::string{"Alice"}, std::int32_t{30},
            std::optional<std::string>{"alice@example.com"},
            std::vector<std::string>{"dev", "music"}
        );

        // Build both formats
        auto fmt_orig = mu::make_binary_format<char>(contact_codec);
        constexpr auto schema = mu::make_schema(contact_codec);
        constexpr auto schema_codec = mu::make_codec(schema);
        auto fmt_schema = mu::make_binary_format<char>(schema_codec);

        // Key assertion: both codecs produce identical bytes
        assert(to_bytes(fmt_orig, myContact) == to_bytes(fmt_schema, myContact));

        // Full migration workflow:
        // 1. Serialize original value to bytes (simulating "bytes from disk")
        auto bytesFromDisk = to_bytes(fmt_orig, myContact);

        // 2. Deserialize with schema codec (type-agnostic)
        auto contactAgnostic = from_bytes(fmt_schema, bytesFromDisk);
        assert(contactAgnostic.has_value());

        // 3. Re-serialize with schema codec (after potential upgrade)
        auto someBytes = to_bytes(fmt_schema, *contactAgnostic);

        // 4. Deserialize with original codec to get the real typed value
        auto contactReal = from_bytes(fmt_orig, someBytes);
        assert(contactReal.has_value());
        assert(std::get<0>(*contactReal) == "Alice");
        assert(std::get<1>(*contactReal) == 30);
        assert(std::get<2>(*contactReal).has_value());
        assert(*std::get<2>(*contactReal) == "alice@example.com");
        assert(std::get<3>(*contactReal).size() == 2);
        assert(std::get<3>(*contactReal)[0] == "dev");
        assert(std::get<3>(*contactReal)[1] == "music");
    }

    // =====================================================================
    // 32. Migration scenario: with absent optional field
    // =====================================================================
    {
        constexpr auto contact_codec = mu::tuple_codec(
            mu::string_codec,
            mu::int32_codec,
            mu::optional_codec(mu::string_codec)
        );

        auto myContact = std::make_tuple(
            std::string{"Bob"}, std::int32_t{25},
            std::optional<std::string>{}
        );

        auto fmt_orig = mu::make_binary_format<char>(contact_codec);
        constexpr auto schema = mu::make_schema(contact_codec);
        constexpr auto schema_codec = mu::make_codec(schema);
        auto fmt_schema = mu::make_binary_format<char>(schema_codec);

        // Bytes are identical
        assert(to_bytes(fmt_orig, myContact) == to_bytes(fmt_schema, myContact));

        // Round-trip through schema codec
        auto bytesFromDisk = to_bytes(fmt_orig, myContact);
        auto agnostic = from_bytes(fmt_schema, bytesFromDisk);
        assert(agnostic.has_value());
        auto someBytes = to_bytes(fmt_schema, *agnostic);
        auto real = from_bytes(fmt_orig, someBytes);
        assert(real.has_value());
        assert(std::get<0>(*real) == "Bob");
        assert(std::get<1>(*real) == 25);
        assert(!std::get<2>(*real).has_value());
    }

    // =====================================================================
    // 33. Migration scenario: point struct -- serialize with original,
    //     deserialize with schema codec, then use adapter to get typed
    // =====================================================================
    {
        constexpr auto schema = mu::make_schema(point::codec);
        constexpr auto schema_codec = mu::make_codec(schema);

        auto fmt_orig = mu::make_binary_format<char>(point::codec);
        auto fmt_schema = mu::make_binary_format<char>(schema_codec);

        point p{42, -100};

        // Serialize with original codec (simulating bytes from disk)
        auto bytes = to_bytes(fmt_orig, p);

        // Deserialize with schema codec (type-agnostic tuple)
        auto agnostic = from_bytes(fmt_schema, bytes);
        assert(agnostic.has_value());

        // Use adapter to convert agnostic → typed
        auto adapter = mu::make_schema_adapter(point::codec, schema_codec);
        auto real = adapter.decode(*agnostic);
        assert(real.has_value());
        assert(*real == p);
    }

    return 0;
}

