#include <muesli/vector_codec>
#include <muesli/fundamental_codecs>

#include <vector>
#include <cassert>

int main() {
    auto codec = muesli::vector_of(muesli::int_codec);
    std::vector<int> v{4, 5, 6};
    auto enc = codec.encode(v);
    auto dec = codec.decode(enc);
    assert(dec == v);
    static_assert(muesli::Codec<decltype(codec.next_codec())>);

    // Test empty vector
    {
        std::vector<int> empty;
        auto enc_empty = codec.encode(empty);
        auto dec_empty = codec.decode(enc_empty);
        assert(dec_empty.empty());
    }

    // Test single element
    {
        std::vector<int> single{99};
        auto enc_single = codec.encode(single);
        auto dec_single = codec.decode(enc_single);
        assert(dec_single.size() == 1);
        assert(dec_single[0] == 99);
    }

    // Test large vector
    {
        std::vector<int> large;
        for (int i = 0; i < 100; ++i) {
            large.push_back(i);
        }
        auto enc_large = codec.encode(large);
        auto dec_large = codec.decode(enc_large);
        assert(dec_large.size() == 100);
        for (int i = 0; i < 100; ++i) {
            assert(dec_large[i] == i);
        }
    }

    // Test with float codec
    {
        auto codec_float = muesli::vector_of(muesli::float_codec);
        std::vector<float> vf{1.1f, 2.2f, 3.3f};
        auto encf = codec_float.encode(vf);
        auto decf = codec_float.decode(encf);
        assert(decf.size() == 3);
        assert(decf[0] == 1.1f);
    }

    return 0;
}
