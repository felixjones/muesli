#include <muesli/optional_codec>
#include <muesli/fundamental_codecs>

#include <cassert>
#include <optional>

int main() {
    // Test optional_codec wraps a value in optional
    {
        auto codec = muesli::optional_codec(muesli::int_codec);
        int value = 42;
        std::optional<int> wrapped_value(value);
        auto enc = codec.encode(wrapped_value);
        auto dec = codec.decode(enc);
        assert(dec == 42);
    }

    // Test optional_codec with nullopt
    {
        auto codec = muesli::optional_codec(muesli::int_codec);
        std::optional<int> nullopt_value = std::nullopt;
        auto enc = codec.encode(nullopt_value);
        auto dec = codec.decode(enc);
        assert(!dec.has_value());
    }

    // Test optional_codec with float
    {
        auto codec = muesli::optional_codec(muesli::float_codec);
        std::optional<float> value(3.14f);
        auto enc = codec.encode(value);
        auto dec = codec.decode(enc);
        assert(dec == 3.14f);
    }

    // Test optional_codec with bool preserving false
    {
        auto codec = muesli::optional_codec(muesli::bool_codec);
        std::optional<bool> false_value(false);
        auto enc = codec.encode(false_value);
        auto dec = codec.decode(enc);
        assert(dec.has_value());
        assert(*dec == false);
    }

    // Test optional_codec with char
    {
        auto codec = muesli::optional_codec(muesli::char_codec);
        std::optional<char> value('x');
        auto enc = codec.encode(value);
        auto dec = codec.decode(enc);
        assert(dec == 'x');
    }

    // Test optional_codec satisfies Codec concept
    {
        auto codec = muesli::optional_codec(muesli::int_codec);
        static_assert(muesli::Codec<decltype(codec)>);
    }

    // Test multiple encode/decode cycles
    {
        auto codec = muesli::optional_codec(muesli::int_codec);

        std::optional<int> v1(10);
        auto e1 = codec.encode(v1);
        auto d1 = codec.decode(e1);
        assert(*d1 == 10);

        std::optional<int> v2;
        auto e2 = codec.encode(v2);
        auto d2 = codec.decode(e2);
        assert(!d2.has_value());
    }

    return 0;
}
