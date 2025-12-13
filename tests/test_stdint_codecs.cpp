#include <muesli/stdint_codecs>

#include <cassert>
#include <cstdint>
#include <limits>

int main() {
    // int8_codec
    {
        auto enc = muesli::int8_codec.encode(int8_t{-50});
        auto dec = muesli::int8_codec.decode(enc);
        assert(dec == -50);

        auto enc_max = muesli::int8_codec.encode(std::numeric_limits<int8_t>::max());
        auto dec_max = muesli::int8_codec.decode(enc_max);
        assert(dec_max == std::numeric_limits<int8_t>::max());
    }

    // int16_codec
    {
        auto enc = muesli::int16_codec.encode(int16_t{-30000});
        auto dec = muesli::int16_codec.decode(enc);
        assert(dec == -30000);
    }

    // int32_codec
    {
        auto enc = muesli::int32_codec.encode(int32_t{-12});
        auto dec = muesli::int32_codec.decode(enc);
        assert(dec == -12);

        auto enc_zero = muesli::int32_codec.encode(int32_t{0});
        auto dec_zero = muesli::int32_codec.decode(enc_zero);
        assert(dec_zero == 0);
    }

    // int64_codec
    {
        auto enc = muesli::int64_codec.encode(int64_t{9223372036854775807LL});
        auto dec = muesli::int64_codec.decode(enc);
        assert(dec == int64_t{9223372036854775807LL});
    }

    // uint8_codec
    {
        auto enc = muesli::uint8_codec.encode(uint8_t{255});
        auto dec = muesli::uint8_codec.decode(enc);
        assert(dec == 255);

        auto enc_zero = muesli::uint8_codec.encode(uint8_t{0});
        auto dec_zero = muesli::uint8_codec.decode(enc_zero);
        assert(dec_zero == 0);
    }

    // uint16_codec
    {
        auto enc = muesli::uint16_codec.encode(uint16_t{60000});
        auto dec = muesli::uint16_codec.decode(enc);
        assert(dec == 60000);
    }

    // uint32_codec
    {
        auto enc = muesli::uint32_codec.encode(uint32_t{123456789});
        auto dec = muesli::uint32_codec.decode(enc);
        assert(dec == uint32_t{123456789});
    }

    // uint64_codec
    {
        auto enc = muesli::uint64_codec.encode(uint64_t{18446744073709551615ULL});
        auto dec = muesli::uint64_codec.decode(enc);
        assert(dec == uint64_t{18446744073709551615ULL});

        auto enc_zero = muesli::uint64_codec.encode(uint64_t{0});
        auto dec_zero = muesli::uint64_codec.decode(enc_zero);
        assert(dec_zero == 0);
    }

    return 0;
}
