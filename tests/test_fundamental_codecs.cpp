#include <muesli/fundamental_codecs>

#include <cassert>
#include <limits>

int main() {
    // int_codec
    {
        auto enc = muesli::int_codec.encode(42);
        auto dec = muesli::int_codec.decode(enc);
        assert(dec == 42);

        auto enc_neg = muesli::int_codec.encode(-999);
        auto dec_neg = muesli::int_codec.decode(enc_neg);
        assert(dec_neg == -999);

        auto enc_zero = muesli::int_codec.encode(0);
        auto dec_zero = muesli::int_codec.decode(enc_zero);
        assert(dec_zero == 0);

        auto enc_max = muesli::int_codec.encode(std::numeric_limits<int>::max());
        auto dec_max = muesli::int_codec.decode(enc_max);
        assert(dec_max == std::numeric_limits<int>::max());
    }

    // float_codec
    {
        auto enc = muesli::float_codec.encode(3.5f);
        auto dec = muesli::float_codec.decode(enc);
        assert(dec == 3.5f);

        auto enc_zero = muesli::float_codec.encode(0.0f);
        auto dec_zero = muesli::float_codec.decode(enc_zero);
        assert(dec_zero == 0.0f);

        auto enc_neg = muesli::float_codec.encode(-1.5f);
        auto dec_neg = muesli::float_codec.decode(enc_neg);
        assert(dec_neg == -1.5f);
    }

    // double_codec
    {
        auto enc = muesli::double_codec.encode(2.71828);
        auto dec = muesli::double_codec.decode(enc);
        assert(dec == 2.71828);
    }

    // bool_codec
    {
        auto enc_true = muesli::bool_codec.encode(true);
        auto dec_true = muesli::bool_codec.decode(enc_true);
        assert(dec_true == true);

        auto enc_false = muesli::bool_codec.encode(false);
        auto dec_false = muesli::bool_codec.decode(enc_false);
        assert(dec_false == false);
    }

    // char_codec
    {
        auto enc = muesli::char_codec.encode('x');
        auto dec = muesli::char_codec.decode(enc);
        assert(dec == 'x');

        auto enc_space = muesli::char_codec.encode(' ');
        auto dec_space = muesli::char_codec.decode(enc_space);
        assert(dec_space == ' ');
    }

    // long_codec
    {
        auto enc = muesli::long_codec.encode(12345L);
        auto dec = muesli::long_codec.decode(enc);
        assert(dec == 12345L);
    }

    // Verify next_codec works
    {
        auto next = muesli::int_codec.next_codec();
        static_assert(muesli::Codec<decltype(next)>);
    }

    return 0;
}
