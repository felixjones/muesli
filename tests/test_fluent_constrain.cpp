#include <muesli/constrained_codec>
#include <muesli/fundamental_codecs>

#include <cassert>

int main() {
    // Test constrainable::constrain with positive predicate
    {
        auto codec = muesli::int_codec.constrain([](int v) { return v > 0; });
        auto enc = codec.encode(42);
        auto dec = codec.decode(enc);
        assert(dec == 42);
    }

    // Test constrainable::constrain with even predicate
    {
        auto codec = muesli::int_codec.constrain([](int v) { return v % 2 == 0; });
        auto enc = codec.encode(100);
        auto dec = codec.decode(enc);
        assert(dec == 100);
    }

    // Test constrainable::constrain with range predicate
    {
        auto codec = muesli::float_codec.constrain([](float f) { return f >= 0.0f && f <= 1.0f; });
        auto enc = codec.encode(0.5f);
        auto dec = codec.decode(enc);
        assert(dec == 0.5f);
    }

    // Test chaining constrainable with other fluent methods
    {
        auto codec = muesli::int_codec
            .constrain([](int v) { return v > 0; })
            .constrain([](int v) { return v < 1000; });
        auto enc = codec.encode(500);
        auto dec = codec.decode(enc);
        assert(dec == 500);
    }

    // Test constrainable with bool codec
    {
        auto codec = muesli::bool_codec.constrain([](bool b) { return true; });
        auto enc = codec.encode(true);
        auto dec = codec.decode(enc);
        assert(dec == true);
    }

    // Test constrainable with char codec and character validation
    {
        auto codec = muesli::char_codec.constrain([](char c) { return c >= 'a' && c <= 'z'; });
        auto enc = codec.encode('m');
        auto dec = codec.decode(enc);
        assert(dec == 'm');
    }

    // Test constrainable::constrain preserves concept satisfaction
    {
        auto codec = muesli::int_codec.constrain([](int v) { return v != 0; });
        static_assert(muesli::Codec<decltype(codec)>);
    }

    return 0;
}
