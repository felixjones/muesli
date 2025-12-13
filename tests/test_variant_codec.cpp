#include <muesli/variant_codec>
#include <muesli/fundamental_codecs>

#include <variant>
#include <cassert>

int main() {
    auto codec = muesli::variant_codec(muesli::int_codec, muesli::float_codec);
    using var_t = std::variant<int, float>;

    var_t v1{42};
    auto e1 = codec.encode(v1);
    auto d1 = codec.decode(e1);
    assert(std::get<int>(d1) == 42);

    var_t v2{2.5f};
    auto e2 = codec.encode(v2);
    auto d2 = codec.decode(e2);
    assert(std::get<float>(d2) == 2.5f);

    static_assert(muesli::Codec<decltype(codec.next_codec())>);

    // Test with three alternatives
    {
        auto codec3 = muesli::variant_codec(muesli::int_codec, muesli::float_codec, muesli::bool_codec);
        using var3_t = std::variant<int, float, bool>;

        var3_t v3a{100};
        auto e3a = codec3.encode(v3a);
        auto d3a = codec3.decode(e3a);
        assert(std::get<int>(d3a) == 100);

        var3_t v3b{true};
        auto e3b = codec3.encode(v3b);
        auto d3b = codec3.decode(e3b);
        assert(std::get<bool>(d3b) == true);
    }

    // Test alternative order preservation
    {
        using var_t2 = std::variant<char, int>;
        auto codec2 = muesli::variant_codec(muesli::char_codec, muesli::int_codec);

        var_t2 vc{'x'};
        auto ec = codec2.encode(vc);
        auto dc = codec2.decode(ec);
        assert(std::get<char>(dc) == 'x');
    }

    return 0;
}
