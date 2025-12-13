#include <muesli/array_codec>
#include <muesli/fundamental_codecs>

#include <array>
#include <cassert>

int main() {
    auto codec = muesli::array_of<3>(muesli::int_codec);
    std::array<int, 3> v{1, 2, 3};
    auto enc = codec.encode(v);
    auto dec = codec.decode(enc);
    assert(dec == v);
    static_assert(muesli::Codec<decltype(codec.next_codec())>);

    // Test with different sizes
    {
        auto codec1 = muesli::array_of<1>(muesli::int_codec);
        std::array<int, 1> v1{42};
        auto enc1 = codec1.encode(v1);
        auto dec1 = codec1.decode(enc1);
        assert(dec1[0] == 42);
    }

    // Test with different element types
    {
        auto codec_float = muesli::array_of<2>(muesli::float_codec);
        std::array<float, 2> vf{1.5f, 2.5f};
        auto encf = codec_float.encode(vf);
        auto decf = codec_float.decode(encf);
        assert(decf[0] == 1.5f);
        assert(decf[1] == 2.5f);
    }

    // Test larger array
    {
        auto codec_large = muesli::array_of<10>(muesli::int_codec);
        std::array<int, 10> v_large{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        auto enc_large = codec_large.encode(v_large);
        auto dec_large = codec_large.decode(enc_large);
        for (int i = 0; i < 10; ++i) {
            assert(dec_large[i] == i);
        }
    }

    return 0;
}
