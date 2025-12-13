#include <muesli/constant_codec>
#include <muesli/fundamental_codecs>

#include <cassert>

int main() {
    // Test constant_codec with int
    {
        constexpr int magic = 42;
        muesli::constant_codec<int, magic> codec;
        auto enc = codec.encode(magic);
        auto dec = codec.decode(enc);
        assert(dec == magic);
    }

    // Test constant_codec with bool true
    {
        constexpr bool flag = true;
        muesli::constant_codec<bool, flag> codec;
        auto enc = codec.encode(flag);
        auto dec = codec.decode(enc);
        assert(dec == flag);
    }

    // Test constant_codec with bool false
    {
        constexpr bool flag = false;
        muesli::constant_codec<bool, flag> codec;
        auto enc = codec.encode(flag);
        auto dec = codec.decode(enc);
        assert(dec == flag);
    }

    // Test constant_codec with char
    {
        constexpr char letter = 'X';
        muesli::constant_codec<char, letter> codec;
        auto enc = codec.encode(letter);
        auto dec = codec.decode(enc);
        assert(dec == letter);
    }

    return 0;
}
