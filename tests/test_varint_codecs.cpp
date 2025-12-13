#include <muesli/varint_codecs>

#include <cassert>
#include <cstdint>

int main() {
    // Test unsigned_varint_codec with small value
    {
        uint32_t value = 127;
        auto enc = muesli::unsigned_varint_codec.encode(value);
        auto dec = muesli::unsigned_varint_codec.decode(std::move(enc));
        assert(dec == value);
    }

    // Test unsigned_varint_codec with large value
    {
        uint32_t value = 16384;
        auto enc = muesli::unsigned_varint_codec.encode(value);
        auto dec = muesli::unsigned_varint_codec.decode(std::move(enc));
        assert(dec == value);
    }

    // Test unsigned_varint_codec with zero
    {
        uint32_t value = 0;
        auto enc = muesli::unsigned_varint_codec.encode(value);
        auto dec = muesli::unsigned_varint_codec.decode(std::move(enc));
        assert(dec == value);
    }


    // Test signed_varint_codec with positive
    {
        int32_t value = 42;
        auto enc = muesli::signed_varint_codec.encode(value);
        auto dec = muesli::signed_varint_codec.decode(std::move(enc));
        assert(dec == value);
    }

    // Test signed_varint_codec with negative
    {
        int32_t value = -42;
        auto enc = muesli::signed_varint_codec.encode(value);
        auto dec = muesli::signed_varint_codec.decode(std::move(enc));
        assert(dec == value);
    }

    // Test signed_varint_codec with zero
    {
        int32_t value = 0;
        auto enc = muesli::signed_varint_codec.encode(value);
        auto dec = muesli::signed_varint_codec.decode(std::move(enc));
        assert(dec == value);
    }

    return 0;
}
