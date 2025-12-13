#include <muesli/constrained_codec>
#include <muesli/fundamental_codecs>

#include <cassert>

int main() {
    auto codec = muesli::constrained_codec(muesli::int_codec, [](int v) { return v > 0; });
    auto enc = codec.encode(3);
    auto dec = codec.decode(enc);
    assert(dec == 3);

    // Test with different predicate
    {
        auto codec_even = muesli::constrained_codec(muesli::int_codec, [](int v) { return v % 2 == 0; });
        auto enc_even = codec_even.encode(42);
        auto dec_even = codec_even.decode(enc_even);
        assert(dec_even == 42);
    }

    // Test with float
    {
        auto codec_float = muesli::constrained_codec(muesli::float_codec, [](float f) { return f >= 0.0f && f <= 1.0f; });
        auto enc_f = codec_float.encode(0.5f);
        auto dec_f = codec_float.decode(enc_f);
        assert(dec_f == 0.5f);
    }

    // Test constrain method
    {
        auto codec2 = muesli::int_codec.constrain([](int v) { return v >= 0; });
        auto enc2 = codec2.encode(100);
        auto dec2 = codec2.decode(enc2);
        assert(dec2 == 100);
    }

    return 0;
}
