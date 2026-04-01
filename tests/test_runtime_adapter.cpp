/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/codecs>
#include <muesli/format/binary_format>
#include <muesli/runtime/adapter>
#include <muesli/runtime/compile>
#include <muesli/runtime/value>
#include <muesli/schema/lowering>

#include <cassert>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>

namespace mu = muesli;
namespace rt = muesli::runtime;
namespace sc = muesli::schema;

template<typename Fmt, typename T>
std::string typed_to_bytes(const Fmt& fmt, const T& value) {
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    bool ok = fmt.serialize(value, ss);
    assert(ok);
    return ss.str();
}

template<mu::Codec C>
rt::codec make_runtime_codec(const C& codec) {
    return rt::compile(sc::build_schema(codec));
}

int main() {
    // 1) Fundamental: runtime bytes -> runtime value -> adapter -> typed
    {
        auto typed_codec = mu::int32_codec;
        auto typed_format = mu::make_binary_format<char>(typed_codec);
        auto runtime_codec = make_runtime_codec(typed_codec);
        auto a = rt::adapter(typed_codec);

        std::int32_t expected = -123;
        std::string bytes = typed_to_bytes(typed_format, expected);

        std::istringstream input(bytes, std::ios::binary);
        auto runtime_value = runtime_codec.deserialize(input);
        assert(runtime_value.has_value());

        auto adapted = a.adapt(runtime_value);
        assert(adapted.has_value());
        assert(*adapted == expected);
    }

    // 2) Optional<string>: present and absent
    {
        auto typed_codec = mu::optional_codec(mu::string_codec);
        auto typed_format = mu::make_binary_format<char>(typed_codec);
        auto runtime_codec = make_runtime_codec(typed_codec);
        auto a = rt::adapter(typed_codec);

        std::optional<std::string> present = "hello";
        std::string present_bytes = typed_to_bytes(typed_format, present);
        std::istringstream present_input(present_bytes, std::ios::binary);
        auto present_runtime = runtime_codec.deserialize(present_input);
        assert(present_runtime.has_value());

        auto present_adapted = a.adapt(present_runtime);
        assert(present_adapted.has_value());
        assert(present_adapted->has_value());
        assert(**present_adapted == "hello");

        std::optional<std::string> absent;
        std::string absent_bytes = typed_to_bytes(typed_format, absent);
        std::istringstream absent_input(absent_bytes, std::ios::binary);
        auto absent_runtime = runtime_codec.deserialize(absent_input);
        assert(absent_runtime.has_value());

        auto absent_adapted = a.adapt(absent_runtime);
        assert(absent_adapted.has_value());
        assert(!absent_adapted->has_value());
    }

    // 3) apply<T>: tuple members adapt back to struct value_type
    {
        struct point { int x, y; };
        auto typed_codec = mu::tuple_codec(
            mu::int_codec.member<&point::x>(),
            mu::int_codec.member<&point::y>()
        ).apply<point>();
        auto typed_format = mu::make_binary_format<char>(typed_codec);
        auto runtime_codec = make_runtime_codec(typed_codec);
        auto a = rt::adapter(typed_codec);

        point expected{10, 20};
        std::string bytes = typed_to_bytes(typed_format, expected);
        std::istringstream input(bytes, std::ios::binary);
        auto runtime_value = runtime_codec.deserialize(input);
        assert(runtime_value.has_value());

        auto adapted = a.adapt(runtime_value);
        assert(adapted.has_value());
        assert(adapted->x == 10);
        assert(adapted->y == 20);
    }

    // 4) Mismatched runtime value shape is rejected
    {
        auto tuple_codec = mu::tuple_codec(mu::int32_codec, mu::int32_codec);
        auto a = rt::adapter(tuple_codec);

        auto bad_value = rt::make_fundamental_from(std::int32_t{7});
        auto adapted = a.adapt(bad_value);
        assert(!adapted.has_value());
    }

    return 0;
}


