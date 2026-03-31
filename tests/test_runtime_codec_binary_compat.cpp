#include <muesli/codecs>
#include <muesli/format/binary_format>
#include <muesli/runtime/codec>
#include <muesli/runtime/compile>
#include <muesli/runtime/value>
#include <muesli/schema/lowering>
#include <muesli/schema/types>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace mu = muesli;
namespace sc = muesli::schema;
namespace rt = muesli::runtime;

// =====================================================================
//  Helpers: typed encode -> bytes, runtime encode -> bytes
// =====================================================================

template<typename Fmt, typename T>
std::string typed_to_bytes(const Fmt& fmt, const T& value) {
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    bool ok = fmt.serialize(value, ss);
    assert(ok);
    return ss.str();
}

std::string runtime_to_bytes(const rt::codec& rtCodec, const rt::value_node& value) {
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    bool ok = rtCodec.serialize(value, ss);
    assert(ok);
    return ss.str();
}

std::optional<rt::value_node> runtime_from_bytes(const rt::codec& rtCodec,
                                                  const std::string& bytes) {
    std::istringstream ss(bytes, std::ios::binary);
    return rtCodec.deserialize(ss);
}

// Build a runtime codec from a typed codec
template<mu::Codec C>
rt::codec make_runtime(const C& codec) {
    auto schema = sc::build_schema(codec);
    return rt::compile(std::move(schema));
}

// =====================================================================
//  Fundamental value_node construction helpers
// =====================================================================

template<typename T>
rt::value_node fund(T value) {
    return rt::make_fundamental_from(value);
}

rt::value_node mono() {
    return rt::value_node{std::monostate{}};
}

rt::value_node make_tuple_val(std::vector<rt::value_node> elems) {
    rt::tuple_value tv;
    for (auto& e : elems) tv.elements.push_back(rt::make_value(std::move(e)));
    return rt::value_node{std::move(tv)};
}

rt::value_node make_variant_val(std::size_t index, rt::value_node active) {
    return rt::value_node{rt::variant_value{index, rt::make_value(std::move(active))}};
}

rt::value_node make_seq(std::vector<rt::value_node> elems) {
    rt::sequence_value sv;
    for (auto& e : elems) sv.elements.push_back(rt::make_value(std::move(e)));
    return rt::value_node{std::move(sv)};
}

// Build a delimited sequence for a null-terminated string
rt::value_node make_string_val(const std::string& str) {
    std::vector<rt::value_node> chars;
    for (char c : str) chars.push_back(fund(c));
    chars.push_back(fund(char{0})); // null terminator
    return make_seq(std::move(chars));
}

// Build a vector value_node (sequence of elements)
template<typename T>
rt::value_node make_vector_val(const std::vector<T>& vec) {
    std::vector<rt::value_node> elems;
    for (const auto& v : vec) elems.push_back(fund(v));
    return make_seq(std::move(elems));
}

int main() {
    // =================================================================
    // 1. Fundamental types -- binary compatibility
    // =================================================================

    // -- 1a: int32
    {
        auto typedFmt = mu::make_binary_format<char>(mu::int32_codec);
        auto rtCodec = make_runtime(mu::int32_codec);

        std::int32_t value = -42;
        auto typedBytes = typed_to_bytes(typedFmt, value);
        auto rtBytes = runtime_to_bytes(rtCodec, fund(value));
        assert(typedBytes == rtBytes);

        // Runtime can deserialize typed bytes
        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == fund(value));
    }
    // -- 1b: double
    {
        auto typedFmt = mu::make_binary_format<char>(mu::double_codec);
        auto rtCodec = make_runtime(mu::double_codec);

        double value = 3.14159265358979;
        auto typedBytes = typed_to_bytes(typedFmt, value);
        auto rtBytes = runtime_to_bytes(rtCodec, fund(value));
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == fund(value));
    }
    // -- 1c: bool
    {
        auto typedFmt = mu::make_binary_format<char>(mu::bool_codec);
        auto rtCodec = make_runtime(mu::bool_codec);

        auto tb = typed_to_bytes(typedFmt, true);
        auto rb = runtime_to_bytes(rtCodec, fund(true));
        assert(tb == rb);

        auto fb = typed_to_bytes(typedFmt, false);
        auto rfb = runtime_to_bytes(rtCodec, fund(false));
        assert(fb == rfb);

        auto decodedTrue = runtime_from_bytes(rtCodec, tb);
        assert(decodedTrue.has_value());
        assert(*decodedTrue == fund(true));

        auto decodedFalse = runtime_from_bytes(rtCodec, fb);
        assert(decodedFalse.has_value());
        assert(*decodedFalse == fund(false));
    }
    // -- 1d: uint8
    {
        auto typedFmt = mu::make_binary_format<char>(mu::uint8_codec);
        auto rtCodec = make_runtime(mu::uint8_codec);

        std::uint8_t value = 0xAB;
        auto typedBytes = typed_to_bytes(typedFmt, value);
        auto rtBytes = runtime_to_bytes(rtCodec, fund(value));
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == fund(value));
    }
    // -- 1e: uint64
    {
        auto typedFmt = mu::make_binary_format<char>(mu::uint64_codec);
        auto rtCodec = make_runtime(mu::uint64_codec);

        std::uint64_t value = 123456789012345ULL;
        auto typedBytes = typed_to_bytes(typedFmt, value);
        auto rtBytes = runtime_to_bytes(rtCodec, fund(value));
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == fund(value));
    }

    // =================================================================
    // 2. Monostate
    // =================================================================

    {
        auto typedFmt = mu::make_binary_format<char>(mu::monostate_codec);
        auto rtCodec = make_runtime(mu::monostate_codec);

        auto typedBytes = typed_to_bytes(typedFmt, std::monostate{});
        auto rtBytes = runtime_to_bytes(rtCodec, mono());
        assert(typedBytes == rtBytes);
        assert(typedBytes.empty());

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == mono());
    }

    // =================================================================
    // 3. Tuple -- binary compatibility
    // =================================================================

    // -- 3a: tuple<int32, float>
    {
        auto codec = mu::tuple_codec(mu::int32_codec, mu::float_codec);
        auto typedFmt = mu::make_binary_format<char>(codec);
        auto rtCodec = make_runtime(codec);

        auto val = std::tuple{std::int32_t{42}, 3.14f};
        auto rtVal = make_tuple_val({fund(std::int32_t{42}), fund(3.14f)});

        auto typedBytes = typed_to_bytes(typedFmt, val);
        auto rtBytes = runtime_to_bytes(rtCodec, rtVal);
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == rtVal);
    }
    // -- 3b: tuple<uint8, uint8, double> (alignment exercise)
    {
        auto codec = mu::tuple_codec(mu::uint8_codec, mu::uint8_codec, mu::double_codec);
        auto typedFmt = mu::make_binary_format<char>(codec);
        auto rtCodec = make_runtime(codec);

        auto val = std::tuple{std::uint8_t{1}, std::uint8_t{2}, 9.99};
        auto rtVal = make_tuple_val({fund(std::uint8_t{1}), fund(std::uint8_t{2}), fund(9.99)});

        auto typedBytes = typed_to_bytes(typedFmt, val);
        auto rtBytes = runtime_to_bytes(rtCodec, rtVal);
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == rtVal);
    }

    // =================================================================
    // 4. Variant -- binary compatibility
    // =================================================================

    // -- 4a: variant<int32, float> - int alternative
    {
        auto codec = mu::variant_codec(mu::int32_codec, mu::float_codec);
        auto typedFmt = mu::make_binary_format<char>(codec);
        auto rtCodec = make_runtime(codec);

        std::variant<std::int32_t, float> val = std::int32_t{77};
        auto rtVal = make_variant_val(0, fund(std::int32_t{77}));

        auto typedBytes = typed_to_bytes(typedFmt, val);
        auto rtBytes = runtime_to_bytes(rtCodec, rtVal);
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == rtVal);
    }
    // -- 4b: variant<int32, float> - float alternative
    {
        auto codec = mu::variant_codec(mu::int32_codec, mu::float_codec);
        auto typedFmt = mu::make_binary_format<char>(codec);
        auto rtCodec = make_runtime(codec);

        std::variant<std::int32_t, float> val = 2.5f;
        auto rtVal = make_variant_val(1, fund(2.5f));

        auto typedBytes = typed_to_bytes(typedFmt, val);
        auto rtBytes = runtime_to_bytes(rtCodec, rtVal);
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == rtVal);
    }
    // -- 4c: variant<monostate, int32> (like optional present)
    {
        auto codec = mu::variant_codec(mu::monostate_codec, mu::int32_codec);
        auto typedFmt = mu::make_binary_format<char>(codec);
        auto rtCodec = make_runtime(codec);

        std::variant<std::monostate, std::int32_t> val = std::int32_t{99};
        auto rtVal = make_variant_val(1, fund(std::int32_t{99}));

        auto typedBytes = typed_to_bytes(typedFmt, val);
        auto rtBytes = runtime_to_bytes(rtCodec, rtVal);
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == rtVal);
    }
    // -- 4d: variant<monostate, int32> (like optional absent)
    {
        auto codec = mu::variant_codec(mu::monostate_codec, mu::int32_codec);
        auto typedFmt = mu::make_binary_format<char>(codec);
        auto rtCodec = make_runtime(codec);

        std::variant<std::monostate, std::int32_t> val = std::monostate{};
        auto rtVal = make_variant_val(0, mono());

        auto typedBytes = typed_to_bytes(typedFmt, val);
        auto rtBytes = runtime_to_bytes(rtCodec, rtVal);
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == rtVal);
    }

    // =================================================================
    // 5. Vector -- binary compatibility
    // =================================================================

    // -- 5a: vector<int32>
    {
        auto codec = mu::vector_of(mu::int32_codec);
        auto typedFmt = mu::make_binary_format<char>(codec);
        auto rtCodec = make_runtime(codec);

        std::vector<std::int32_t> val = {10, 20, 30};
        auto rtVal = make_vector_val(val);

        auto typedBytes = typed_to_bytes(typedFmt, val);
        auto rtBytes = runtime_to_bytes(rtCodec, rtVal);
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == rtVal);
    }
    // -- 5b: empty vector
    {
        auto codec = mu::vector_of(mu::int32_codec);
        auto typedFmt = mu::make_binary_format<char>(codec);
        auto rtCodec = make_runtime(codec);

        std::vector<std::int32_t> val = {};
        auto rtVal = make_vector_val(val);

        auto typedBytes = typed_to_bytes(typedFmt, val);
        auto rtBytes = runtime_to_bytes(rtCodec, rtVal);
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == rtVal);
    }

    // =================================================================
    // 6. String (null-terminated delimited) -- binary compatibility
    // =================================================================

    // -- 6a: basic string
    {
        auto typedFmt = mu::make_binary_format<char>(mu::string_codec);
        auto rtCodec = make_runtime(mu::string_codec);

        std::string val = "hello";
        auto rtVal = make_string_val(val);

        auto typedBytes = typed_to_bytes(typedFmt, val);
        auto rtBytes = runtime_to_bytes(rtCodec, rtVal);
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == rtVal);
    }
    // -- 6b: empty string
    {
        auto typedFmt = mu::make_binary_format<char>(mu::string_codec);
        auto rtCodec = make_runtime(mu::string_codec);

        std::string val = "";
        auto rtVal = make_string_val(val);

        auto typedBytes = typed_to_bytes(typedFmt, val);
        auto rtBytes = runtime_to_bytes(rtCodec, rtVal);
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == rtVal);
    }

    // =================================================================
    // 7. Tuple with string -- binary compatibility
    // =================================================================

    // -- 7a: tuple<string, int32>
    {
        auto codec = mu::tuple_codec(mu::string_codec, mu::int32_codec);
        auto typedFmt = mu::make_binary_format<char>(codec);
        auto rtCodec = make_runtime(codec);

        auto val = std::tuple{std::string{"abc"}, std::int32_t{7}};
        auto rtVal = make_tuple_val({make_string_val("abc"), fund(std::int32_t{7})});

        auto typedBytes = typed_to_bytes(typedFmt, val);
        auto rtBytes = runtime_to_bytes(rtCodec, rtVal);
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == rtVal);
    }
    // -- 7b: tuple<int32, string> (reversed order)
    {
        auto codec = mu::tuple_codec(mu::int32_codec, mu::string_codec);
        auto typedFmt = mu::make_binary_format<char>(codec);
        auto rtCodec = make_runtime(codec);

        auto val = std::tuple{std::int32_t{-7}, std::string{"world"}};
        auto rtVal = make_tuple_val({fund(std::int32_t{-7}), make_string_val("world")});

        auto typedBytes = typed_to_bytes(typedFmt, val);
        auto rtBytes = runtime_to_bytes(rtCodec, rtVal);
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == rtVal);
    }

    // =================================================================
    // 8. Optional -- binary compatibility via variant lowering
    // =================================================================

    // -- 8a: optional<int32> present
    {
        auto codec = mu::optional_codec(mu::int32_codec);
        auto typedFmt = mu::make_binary_format<char>(codec);
        auto rtCodec = make_runtime(codec);

        std::optional<std::int32_t> val = 42;
        // optional encodes as variant(monostate, T): index 1 = present
        auto rtVal = make_variant_val(1, fund(std::int32_t{42}));

        auto typedBytes = typed_to_bytes(typedFmt, val);
        auto rtBytes = runtime_to_bytes(rtCodec, rtVal);
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == rtVal);
    }
    // -- 8b: optional<int32> absent
    {
        auto codec = mu::optional_codec(mu::int32_codec);
        auto typedFmt = mu::make_binary_format<char>(codec);
        auto rtCodec = make_runtime(codec);

        std::optional<std::int32_t> val = std::nullopt;
        // optional encodes as variant(monostate, T): index 0 = absent
        auto rtVal = make_variant_val(0, mono());

        auto typedBytes = typed_to_bytes(typedFmt, val);
        auto rtBytes = runtime_to_bytes(rtCodec, rtVal);
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == rtVal);
    }

    // =================================================================
    // 9. Struct codec (apply) -- binary compatibility
    // =================================================================

    // -- 9a: point struct
    {
        struct point { int x, y; };
        auto codec = mu::tuple_codec(
            mu::int_codec.member<&point::x>(),
            mu::int_codec.member<&point::y>()
        ).apply<point>();
        auto typedFmt = mu::make_binary_format<char>(codec);
        auto rtCodec = make_runtime(codec);

        point val{10, 20};
        auto rtVal = make_tuple_val({fund(int{10}), fund(int{20})});

        auto typedBytes = typed_to_bytes(typedFmt, val);
        auto rtBytes = runtime_to_bytes(rtCodec, rtVal);
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == rtVal);
    }
    // -- 9b: person struct with string
    {
        struct person { int age; std::string name; };
        auto codec = mu::tuple_codec(
            mu::int_codec.member<&person::age>(),
            mu::string_codec.member<&person::name>()
        ).apply<person>();
        auto typedFmt = mu::make_binary_format<char>(codec);
        auto rtCodec = make_runtime(codec);

        person val{30, "Alice"};
        auto rtVal = make_tuple_val({fund(int{30}), make_string_val("Alice")});

        auto typedBytes = typed_to_bytes(typedFmt, val);
        auto rtBytes = runtime_to_bytes(rtCodec, rtVal);
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == rtVal);
    }

    // =================================================================
    // 10. Array -- binary compatibility
    // =================================================================

    {
        auto codec = mu::array_of<3>(mu::float_codec);
        auto typedFmt = mu::make_binary_format<char>(codec);
        auto rtCodec = make_runtime(codec);

        std::array<float, 3> val = {1.0f, 2.0f, 3.0f};
        auto rtVal = make_seq({fund(1.0f), fund(2.0f), fund(3.0f)});

        auto typedBytes = typed_to_bytes(typedFmt, val);
        auto rtBytes = runtime_to_bytes(rtCodec, rtVal);
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == rtVal);
    }

    // =================================================================
    // 11. Cross-decode: runtime decodes typed bytes, typed decodes runtime bytes
    // =================================================================

    // -- 11a: vector<string> cross-decode
    {
        auto codec = mu::vector_of(mu::string_codec);
        auto typedFmt = mu::make_binary_format<char>(codec);
        auto rtCodec = make_runtime(codec);

        std::vector<std::string> val = {"alpha", "beta", "gamma"};

        // Typed -> bytes -> runtime decode
        auto typedBytes = typed_to_bytes(typedFmt, val);
        auto rtDecoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(rtDecoded.has_value());

        // Runtime -> bytes -> typed decode
        // Rebuild the runtime value
        std::vector<rt::value_node> strNodes;
        for (auto& s : val) strNodes.push_back(make_string_val(s));
        auto rtVal = make_seq(std::move(strNodes));
        auto rtBytes = runtime_to_bytes(rtCodec, rtVal);
        assert(rtBytes == typedBytes);

        // Verify typed can decode runtime bytes
        std::istringstream ss(rtBytes, std::ios::binary);
        auto typedDecoded = typedFmt.deserialize(ss);
        assert(typedDecoded.has_value());
        assert(*typedDecoded == val);
    }

    // =================================================================
    // 12. Complex nested: tuple<variant<int, string>, vector<double>>
    // =================================================================

    {
        auto codec = mu::tuple_codec(
            mu::variant_codec(mu::int32_codec, mu::string_codec),
            mu::vector_of(mu::double_codec)
        );
        auto typedFmt = mu::make_binary_format<char>(codec);
        auto rtCodec = make_runtime(codec);

        // Variant holding string alternative
        using var_t = std::variant<std::int32_t, std::string>;
        auto val = std::tuple{var_t{std::string{"test"}}, std::vector<double>{1.1, 2.2}};

        auto rtVar = make_variant_val(1, make_string_val("test"));
        std::vector<rt::value_node> dblElems;
        dblElems.push_back(fund(1.1));
        dblElems.push_back(fund(2.2));
        auto rtVec = make_seq(std::move(dblElems));
        auto rtVal = make_tuple_val({std::move(rtVar), std::move(rtVec)});

        auto typedBytes = typed_to_bytes(typedFmt, val);
        auto rtBytes = runtime_to_bytes(rtCodec, rtVal);
        assert(typedBytes == rtBytes);

        auto decoded = runtime_from_bytes(rtCodec, typedBytes);
        assert(decoded.has_value());
        assert(*decoded == rtVal);
    }

    return 0;
}

