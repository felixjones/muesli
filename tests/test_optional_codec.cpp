#include <muesli/optional_codec>
#include <muesli/fundamental_codecs>

#include <optional>
#include <cassert>

int main() {
    auto codec = muesli::optional_codec(muesli::int_codec);
    std::optional<int> v1 = 11;
    auto e1 = codec.encode(v1);
    auto d1 = codec.decode(e1);
    assert(d1 == 11);

    std::optional<int> v2 = std::nullopt;
    auto e2 = codec.encode(v2);
    auto d2 = codec.decode(e2);
    assert(!d2.has_value());

    // Test with float
    {
        auto codec_float = muesli::optional_codec(muesli::float_codec);

        std::optional<float> vf = 2.5f;
        auto ef = codec_float.encode(vf);
        auto df = codec_float.decode(ef);
        assert(df.has_value());
        assert(*df == 2.5f);

        std::optional<float> vf_empty = std::nullopt;
        auto ef_empty = codec_float.encode(vf_empty);
        auto df_empty = codec_float.decode(ef_empty);
        assert(!df_empty.has_value());
    }

    // Test zero/falsey values are preserved
    {
        std::optional<int> v_zero = 0;
        auto e_zero = codec.encode(v_zero);
        auto d_zero = codec.decode(e_zero);
        assert(d_zero.has_value());
        assert(*d_zero == 0);

        auto codec_bool = muesli::optional_codec(muesli::bool_codec);
        std::optional<bool> v_false = false;
        auto e_false = codec_bool.encode(v_false);
        auto d_false = codec_bool.decode(e_false);
        assert(d_false.has_value());
        assert(*d_false == false);
    }

    return 0;
}
