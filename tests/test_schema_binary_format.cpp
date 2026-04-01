#include <muesli/codecs>
#include <muesli/format/binary_format>
#include <muesli/runtime/compile>
#include <muesli/runtime/value>
#include <muesli/schema/binary_schema_format>
#include <muesli/schema/compile>
#include <muesli/schema/lowering>
#include <muesli/schema/types>

#include <cassert>
#include <cstddef>
#include <cstring>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace mu = muesli;
namespace sc = muesli::schema;
namespace rt = muesli::runtime;

// Helper: build_schema -> serialize -> deserialize -> assert equality
template<mu::Codec C>
void assert_schema_roundtrip(const C& codec) {
    auto original = sc::build_schema(codec);
    auto bytes = sc::serialize_schema(original);
    auto restored = sc::deserialize_schema(bytes);
    assert(restored.has_value());
    assert(*restored == original);
}

int main() {
    // =================================================================
    // 1. Fundamental types
    // =================================================================

    // -- 1a: int32
    {
        auto schema = sc::build_schema(mu::int32_codec);
        auto* f = std::get_if<sc::fundamental_schema>(&schema.data);
        assert(f);
        assert(f->size == sizeof(std::int32_t));
        assert(f->alignment == alignof(std::int32_t));
        assert_schema_roundtrip(mu::int32_codec);
    }
    // -- 1b: double
    {
        auto schema = sc::build_schema(mu::double_codec);
        auto* f = std::get_if<sc::fundamental_schema>(&schema.data);
        assert(f);
        assert(f->size == sizeof(double));
        assert(f->alignment == alignof(double));
        assert_schema_roundtrip(mu::double_codec);
    }
    // -- 1c: bool
    {
        auto schema = sc::build_schema(mu::bool_codec);
        auto* f = std::get_if<sc::fundamental_schema>(&schema.data);
        assert(f);
        assert(f->size == sizeof(bool));
        assert_schema_roundtrip(mu::bool_codec);
    }
    // -- 1d: uint8
    {
        auto schema = sc::build_schema(mu::uint8_codec);
        auto* f = std::get_if<sc::fundamental_schema>(&schema.data);
        assert(f && f->size == 1 && f->alignment == 1);
        assert_schema_roundtrip(mu::uint8_codec);
    }
    // -- 1e: uint64
    {
        assert_schema_roundtrip(mu::uint64_codec);
    }
    // -- 1f: float
    {
        assert_schema_roundtrip(mu::float_codec);
    }

    // =================================================================
    // 2. Monostate / constant
    // =================================================================

    // -- 2a: monostate_codec
    {
        auto schema = sc::build_schema(mu::monostate_codec);
        assert(std::holds_alternative<sc::monostate_schema>(schema.data));
        assert_schema_roundtrip(mu::monostate_codec);
    }

    // =================================================================
    // 3. Tuple codecs
    // =================================================================

    // -- 3a: tuple<int32, float>
    {
        auto codec = mu::tuple_codec(mu::int32_codec, mu::float_codec);
        auto schema = sc::build_schema(codec);
        auto* ts = std::get_if<sc::tuple_schema>(&schema.data);
        assert(ts && ts->elements.size() == 2);
        assert(std::holds_alternative<sc::fundamental_schema>(ts->elements[0]->data));
        assert(std::holds_alternative<sc::fundamental_schema>(ts->elements[1]->data));
        assert_schema_roundtrip(codec);
    }
    // -- 3b: pair<uint32, double>
    {
        auto codec = mu::pair_codec(mu::uint32_codec, mu::double_codec);
        auto schema = sc::build_schema(codec);
        auto* ts = std::get_if<sc::tuple_schema>(&schema.data);
        assert(ts && ts->elements.size() == 2);
        assert_schema_roundtrip(codec);
    }
    // -- 3c: nested tuple
    {
        auto inner = mu::tuple_codec(mu::uint8_codec, mu::uint8_codec);
        auto outer = mu::tuple_codec(mu::int32_codec, inner);
        auto schema = sc::build_schema(outer);
        auto* ts = std::get_if<sc::tuple_schema>(&schema.data);
        assert(ts && ts->elements.size() == 2);
        assert(std::holds_alternative<sc::tuple_schema>(ts->elements[1]->data));
        assert_schema_roundtrip(outer);
    }

    // =================================================================
    // 4. Variant codecs
    // =================================================================

    // -- 4a: variant<int32, float>
    {
        auto codec = mu::variant_codec(mu::int32_codec, mu::float_codec);
        auto schema = sc::build_schema(codec);
        auto* vs = std::get_if<sc::variant_schema>(&schema.data);
        assert(vs && vs->alternatives.size() == 2);
        assert(vs->index_bytes >= 1);
        assert_schema_roundtrip(codec);
    }
    // -- 4b: variant<monostate, int32> (like optional)
    {
        auto codec = mu::variant_codec(mu::monostate_codec, mu::int32_codec);
        auto schema = sc::build_schema(codec);
        auto* vs = std::get_if<sc::variant_schema>(&schema.data);
        assert(vs && vs->alternatives.size() == 2);
        assert(std::holds_alternative<sc::monostate_schema>(vs->alternatives[0]->data));
        assert_schema_roundtrip(codec);
    }
    // -- 4c: variant<bool, double, monostate>
    {
        auto codec = mu::variant_codec(mu::bool_codec, mu::double_codec, mu::monostate_codec);
        assert_schema_roundtrip(codec);
    }

    // =================================================================
    // 5. Vector codecs
    // =================================================================

    // -- 5a: vector<int32>
    {
        auto codec = mu::vector_of(mu::int32_codec);
        auto schema = sc::build_schema(codec);
        auto* vs = std::get_if<sc::vector_schema>(&schema.data);
        assert(vs);
        assert(vs->size_bytes == sizeof(std::size_t));
        assert(vs->size_alignment == alignof(std::size_t));
        assert(std::holds_alternative<sc::fundamental_schema>(vs->element->data));
        assert_schema_roundtrip(codec);
    }
    // -- 5b: vector<vector<uint8>>
    {
        auto codec = mu::vector_of(mu::vector_of(mu::uint8_codec));
        auto schema = sc::build_schema(codec);
        auto* vs = std::get_if<sc::vector_schema>(&schema.data);
        assert(vs);
        assert(std::holds_alternative<sc::vector_schema>(vs->element->data));
        assert_schema_roundtrip(codec);
    }

    // =================================================================
    // 6. Array codecs
    // =================================================================

    // -- 6a: array<float, 3>
    {
        auto codec = mu::array_of<3>(mu::float_codec);
        auto schema = sc::build_schema(codec);
        auto* as = std::get_if<sc::array_schema>(&schema.data);
        assert(as && as->count == 3);
        assert(std::holds_alternative<sc::fundamental_schema>(as->element->data));
        assert_schema_roundtrip(codec);
    }

    // =================================================================
    // 7. String codec (delimited)
    // =================================================================

    // -- 7a: string_codec
    {
        auto schema = sc::build_schema(mu::string_codec);
        auto* ds = std::get_if<sc::delimited_schema>(&schema.data);
        assert(ds);
        assert(ds->delimiter_mask == 0xFF);
        assert(ds->delimiter_expected == 0x00);
        auto* elem = std::get_if<sc::fundamental_schema>(&ds->element->data);
        assert(elem && elem->size == 1);
        assert_schema_roundtrip(mu::string_codec);
    }

    // =================================================================
    // 8. Varint codec (delimited)
    // =================================================================

    // -- 8a: unsigned_varint_codec
    {
        auto schema = sc::build_schema(mu::unsigned_varint_codec);
        auto* ds = std::get_if<sc::delimited_schema>(&schema.data);
        assert(ds);
        assert(ds->delimiter_mask == 0x80);
        assert(ds->delimiter_expected == 0x00);
        assert_schema_roundtrip(mu::unsigned_varint_codec);
    }
    // -- 8b: signed_varint_codec (transform wrapping unsigned)
    {
        auto schema = sc::build_schema(mu::signed_varint_codec);
        auto* ds = std::get_if<sc::delimited_schema>(&schema.data);
        assert(ds);
        assert(ds->delimiter_mask == 0x80);
        assert(ds->delimiter_expected == 0x00);
        assert_schema_roundtrip(mu::signed_varint_codec);
    }

    // =================================================================
    // 9. Wrapper transparency
    // =================================================================

    // -- 9a: optional_codec -> variant(monostate, T)
    {
        auto codec = mu::optional_codec(mu::int32_codec);
        auto schema = sc::build_schema(codec);
        auto* vs = std::get_if<sc::variant_schema>(&schema.data);
        assert(vs && vs->alternatives.size() == 2);
        assert(std::holds_alternative<sc::monostate_schema>(vs->alternatives[0]->data));
        assert(std::holds_alternative<sc::fundamental_schema>(vs->alternatives[1]->data));
        assert_schema_roundtrip(codec);
    }
    // -- 9b: constrained int -> fundamental (constraint is transparent)
    {
        auto codec = mu::int32_codec.constrain([](std::int32_t v) { return v > 0; });
        auto schema = sc::build_schema(codec);
        assert(std::holds_alternative<sc::fundamental_schema>(schema.data));
    }
    // -- 9c: transform -> transparent
    {
        auto codec = mu::int32_codec.transform(
            [](std::int32_t v) { return v; },
            [](std::int32_t v) { return v; }
        );
        auto schema = sc::build_schema(codec);
        assert(std::holds_alternative<sc::fundamental_schema>(schema.data));
    }
    // -- 9d: member -> transparent
    {
        struct point { int x, y; };
        auto codec = mu::int_codec.member<&point::x>();
        auto schema = sc::build_schema(codec);
        assert(std::holds_alternative<sc::fundamental_schema>(schema.data));
    }
    // -- 9e: apply -> transparent to arg_codec
    {
        struct point { int x, y; };
        auto codec = mu::tuple_codec(
            mu::int_codec.member<&point::x>(),
            mu::int_codec.member<&point::y>()
        ).apply<point>();
        auto schema = sc::build_schema(codec);
        auto* ts = std::get_if<sc::tuple_schema>(&schema.data);
        assert(ts && ts->elements.size() == 2);
        assert_schema_roundtrip(codec);
    }
    // -- 9f: or_else -> transparent
    {
        auto codec = mu::int32_codec.or_else([] { return std::int32_t{0}; });
        auto schema = sc::build_schema(codec);
        assert(std::holds_alternative<sc::fundamental_schema>(schema.data));
    }

    // =================================================================
    // 10. Complex compositions
    // =================================================================

    // -- 10a: tuple<string, vector<int32>>
    {
        auto codec = mu::tuple_codec(mu::string_codec, mu::vector_of(mu::int32_codec));
        auto schema = sc::build_schema(codec);
        auto* ts = std::get_if<sc::tuple_schema>(&schema.data);
        assert(ts && ts->elements.size() == 2);
        assert(std::holds_alternative<sc::delimited_schema>(ts->elements[0]->data));
        assert(std::holds_alternative<sc::vector_schema>(ts->elements[1]->data));
        assert_schema_roundtrip(codec);
    }
    // -- 10b: vector<optional<string>>
    {
        auto codec = mu::vector_of(mu::optional_codec(mu::string_codec));
        auto schema = sc::build_schema(codec);
        auto* vs = std::get_if<sc::vector_schema>(&schema.data);
        assert(vs);
        // Element should be variant(monostate, delimited)
        auto* inner = std::get_if<sc::variant_schema>(&vs->element->data);
        assert(inner && inner->alternatives.size() == 2);
        assert_schema_roundtrip(codec);
    }

    // =================================================================
    // 11. Malformed input
    // =================================================================

    // -- 11a: empty byte vector
    {
        auto result = sc::deserialize_schema(std::vector<std::byte>{});
        assert(!result.has_value());
    }
    // -- 11b: invalid tag
    {
        std::vector<std::byte> bad = {std::byte{0xFF}};
        auto result = sc::deserialize_schema(bad);
        assert(!result.has_value());
    }
    // -- 11c: truncated fundamental
    {
        std::vector<std::byte> bad = {std::byte{0x01}, std::byte{4}};
        auto result = sc::deserialize_schema(bad);
        assert(!result.has_value());
    }

    // =================================================================
    // 12. End-to-end pipeline: typed codec -> schema bytes -> runtime codec
    //     -> deserialize data that was serialized by the typed codec
    //
    //     This simulates the disk round-trip:
    //       Write side:  schema::compile(codec) -> write bytes to disk
    //       Read side:   read bytes from disk -> runtime::compile(bytes)
    //                    -> rtCodec.deserialize(data_stream)
    // =================================================================

    // -- 12a: int32 through full pipeline
    {
        // Write side: compile schema to bytes
        auto schemaBytes = sc::compile(mu::int32_codec);

        // Serialize a typed value to a data stream
        auto typedFmt = mu::make_binary_format<char>(mu::int32_codec);
        std::stringstream dataSS(std::ios::in | std::ios::out | std::ios::binary);
        bool serialized = typedFmt.serialize(std::int32_t{-42}, dataSS);
        assert(serialized);
        std::string dataBytes = dataSS.str();

        // Read side: compile runtime codec from schema bytes (as if loaded from disk)
        auto rtCodec = rt::compile(schemaBytes);
        assert(rtCodec.has_value());

        // Deserialize the typed data with the runtime codec
        std::istringstream dataIn(dataBytes, std::ios::binary);
        auto decoded = rtCodec->deserialize(dataIn);
        assert(decoded.has_value());

        // Verify the value matches
        auto expected = rt::make_fundamental_from(std::int32_t{-42});
        assert(*decoded == expected);
    }

    // -- 12b: struct with string through full pipeline
    {
        struct person { int age; std::string name; };
        auto codec = mu::tuple_codec(
            mu::int_codec.member<&person::age>(),
            mu::string_codec.member<&person::name>()
        ).apply<person>();

        // Write side
        auto schemaBytes = sc::compile(codec);
        auto typedFmt = mu::make_binary_format<char>(codec);
        std::stringstream dataSS(std::ios::in | std::ios::out | std::ios::binary);
        person val{25, "Bob"};
        bool serialized = typedFmt.serialize(val, dataSS);
        assert(serialized);
        std::string dataBytes = dataSS.str();

        // Read side: reconstruct runtime codec from schema bytes
        auto rtCodec = rt::compile(schemaBytes);
        assert(rtCodec.has_value());

        // Deserialize
        std::istringstream dataIn(dataBytes, std::ios::binary);
        auto decoded = rtCodec->deserialize(dataIn);
        assert(decoded.has_value());

        // Verify: top-level is a tuple with two elements
        auto* tv = std::get_if<rt::tuple_value>(&decoded->data);
        assert(tv && tv->elements.size() == 2);

        // First element: age == 25
        auto* age = std::get_if<rt::fundamental_value>(&tv->elements[0]->data);
        assert(age && age->data.size() == sizeof(int));
        int decodedAge = 0;
        std::memcpy(&decodedAge, age->data.data(), sizeof(int));
        assert(decodedAge == 25);

        // Second element: name == "Bob" (delimited sequence: 'B','o','b','\0')
        auto* name = std::get_if<rt::sequence_value>(&tv->elements[1]->data);
        assert(name && name->elements.size() == 4); // 3 chars + null
        std::string decodedName;
        for (std::size_t i = 0; i < name->elements.size(); ++i) {
            auto* ch = std::get_if<rt::fundamental_value>(&name->elements[i]->data);
            assert(ch && ch->data.size() == 1);
            char c = static_cast<char>(ch->data[0]);
            if (c == '\0') break;
            decodedName += c;
        }
        assert(decodedName == "Bob");
    }

    // -- 12c: vector<int32> through full pipeline
    {
        auto codec = mu::vector_of(mu::int32_codec);

        // Write side
        auto schemaBytes = sc::compile(codec);
        auto typedFmt = mu::make_binary_format<char>(codec);
        std::vector<std::int32_t> val = {10, 20, 30};
        std::stringstream dataSS(std::ios::in | std::ios::out | std::ios::binary);
        bool serialized = typedFmt.serialize(val, dataSS);
        assert(serialized);
        std::string dataBytes = dataSS.str();

        // Read side
        auto rtCodec = rt::compile(schemaBytes);
        assert(rtCodec.has_value());
        std::istringstream dataIn(dataBytes, std::ios::binary);
        auto decoded = rtCodec->deserialize(dataIn);
        assert(decoded.has_value());

        // Verify: sequence with 3 elements
        auto* sv = std::get_if<rt::sequence_value>(&decoded->data);
        assert(sv && sv->elements.size() == 3);
        for (std::size_t i = 0; i < 3; ++i) {
            auto* fv = std::get_if<rt::fundamental_value>(&sv->elements[i]->data);
            assert(fv && fv->data.size() == sizeof(std::int32_t));
            std::int32_t elem = 0;
            std::memcpy(&elem, fv->data.data(), sizeof(std::int32_t));
            assert(elem == static_cast<std::int32_t>((i + 1) * 10));
        }
    }

    // -- 12d: optional<string> through full pipeline
    {
        auto codec = mu::optional_codec(mu::string_codec);

        // Write side: present value
        auto schemaBytes = sc::compile(codec);
        auto typedFmt = mu::make_binary_format<char>(codec);
        std::optional<std::string> val = "hello";
        std::stringstream dataSS(std::ios::in | std::ios::out | std::ios::binary);
        bool serialized = typedFmt.serialize(val, dataSS);
        assert(serialized);
        std::string dataBytes = dataSS.str();

        // Read side
        auto rtCodec = rt::compile(schemaBytes);
        assert(rtCodec.has_value());
        std::istringstream dataIn(dataBytes, std::ios::binary);
        auto decoded = rtCodec->deserialize(dataIn);
        assert(decoded.has_value());

        // Verify: variant with index 1 (present), containing delimited "hello\0"
        auto* vv = std::get_if<rt::variant_value>(&decoded->data);
        assert(vv && vv->index == 1 && vv->active);
        auto* inner = std::get_if<rt::sequence_value>(&vv->active->data);
        assert(inner && inner->elements.size() == 6); // h,e,l,l,o,\0

        // Write side: absent value
        std::optional<std::string> absent;
        std::stringstream dataSS2(std::ios::in | std::ios::out | std::ios::binary);
        bool serializedAbsent = typedFmt.serialize(absent, dataSS2);
        assert(serializedAbsent);
        std::string dataBytes2 = dataSS2.str();

        std::istringstream dataIn2(dataBytes2, std::ios::binary);
        auto decoded2 = rtCodec->deserialize(dataIn2);
        assert(decoded2.has_value());
        auto* vv2 = std::get_if<rt::variant_value>(&decoded2->data);
        assert(vv2 && vv2->index == 0); // absent
    }

    // -- 12e: runtime codec also serializes, and typed codec can read it back
    {
        auto codec = mu::vector_of(mu::int32_codec);

        // Compile schema to bytes, then compile runtime codec from those bytes
        auto schemaBytes = sc::compile(codec);
        auto rtCodec = rt::compile(schemaBytes);
        assert(rtCodec.has_value());

        // Build a runtime value and serialize it
        rt::sequence_value sv;
        for (std::int32_t v : {100, 200, 300}) {
            sv.elements.push_back(rt::make_value(rt::make_fundamental_from(v)));
        }
        rt::value_node rtVal{std::move(sv)};
        std::stringstream rtOut(std::ios::in | std::ios::out | std::ios::binary);
        bool serialized = rtCodec->serialize(rtVal, rtOut);
        assert(serialized);
        std::string rtBytes = rtOut.str();

        // Typed codec deserializes the runtime-produced bytes
        auto typedFmt = mu::make_binary_format<char>(codec);
        std::istringstream typedIn(rtBytes, std::ios::binary);
        auto typedDecoded = typedFmt.deserialize(typedIn);
        assert(typedDecoded.has_value());
        assert(typedDecoded->size() == 3);
        assert((*typedDecoded)[0] == 100);
        assert((*typedDecoded)[1] == 200);
        assert((*typedDecoded)[2] == 300);
    }

    // -- 12f: malformed schema bytes produce nullopt from runtime::compile
    {
        std::vector<std::byte> bad = {std::byte{0xFF}};
        auto rtCodec = rt::compile(bad);
        assert(!rtCodec.has_value());
    }

    return 0;
}
