#include <muesli/range_codec>
#include <muesli/fundamental_codecs>
#include <muesli/util/range_holder>

#include <vector>
#include <cassert>

int main() {
    auto codec = muesli::range_codec(muesli::int_codec);
    std::vector<int> data{7, 8, 9};
    muesli::range_holder<int> rh(data);
    auto enc = codec.encode(rh);
    auto dec = codec.decode(std::move(enc));
    std::vector<int> out(dec.begin(), dec.end());
    assert(out == data);

    // Test with empty range
    {
        std::vector<int> empty_data;
        muesli::range_holder<int> empty_rh(empty_data);
        auto enc_empty = codec.encode(empty_rh);
        auto dec_empty = codec.decode(std::move(enc_empty));
        std::vector<int> out_empty(dec_empty.begin(), dec_empty.end());
        assert(out_empty.empty());
    }

    // Test with single element
    {
        std::vector<int> single{999};
        muesli::range_holder<int> single_rh(single);
        auto enc_single = codec.encode(single_rh);
        auto dec_single = codec.decode(std::move(enc_single));
        std::vector<int> out_single(dec_single.begin(), dec_single.end());
        assert(out_single.size() == 1);
        assert(out_single[0] == 999);
    }

    // Test with larger range
    {
        std::vector<int> large;
        for (int i = 0; i < 50; ++i) {
            large.push_back(i * 2);
        }
        muesli::range_holder<int> large_rh(large);
        auto enc_large = codec.encode(large_rh);
        auto dec_large = codec.decode(std::move(enc_large));
        std::vector<int> out_large(dec_large.begin(), dec_large.end());
        assert(out_large.size() == 50);
        for (int i = 0; i < 50; ++i) {
            assert(out_large[i] == i * 2);
        }
    }

    return 0;
}
