#include <muesli/tuple_codec>
#include <muesli/fundamental_codecs>

#include <tuple>
#include <cassert>

int main() {
    auto codec = muesli::tuple_codec(muesli::int_codec, muesli::float_codec);
    using tuple_t = std::tuple<int, float>;
    tuple_t v{10, 1.5f};
    auto enc = codec.encode(v);
    auto dec = codec.decode(enc);
    assert(std::get<0>(dec) == 10);
    assert(std::get<1>(dec) == 1.5f);

    // Test with three elements
    {
        auto codec3 = muesli::tuple_codec(muesli::int_codec, muesli::float_codec, muesli::bool_codec);
        std::tuple<int, float, bool> v3{99, 3.14f, true};
        auto enc3 = codec3.encode(v3);
        auto dec3 = codec3.decode(enc3);
        assert(std::get<0>(dec3) == 99);
        assert(std::get<1>(dec3) == 3.14f);
        assert(std::get<2>(dec3) == true);
    }

    // Test with different types
    {
        auto codec2 = muesli::tuple_codec(muesli::bool_codec, muesli::char_codec);
        std::tuple<bool, char> v2{false, 'z'};
        auto enc2 = codec2.encode(v2);
        auto dec2 = codec2.decode(enc2);
        assert(std::get<0>(dec2) == false);
        assert(std::get<1>(dec2) == 'z');
    }

    // Test single element
    {
        auto codec1 = muesli::tuple_codec(muesli::int_codec);
        std::tuple<int> v1{77};
        auto enc1 = codec1.encode(v1);
        auto dec1 = codec1.decode(enc1);
        assert(std::get<0>(dec1) == 77);
    }

    return 0;
}
