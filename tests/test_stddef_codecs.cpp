#include <muesli/stddef_codecs>

#include <cassert>
#include <cstddef>
#include <limits>

int main() {
    // size_codec
    {
        auto enc = muesli::size_codec.encode(std::size_t{64});
        auto dec = muesli::size_codec.decode(enc);
        assert(dec == std::size_t{64});

        auto enc_zero = muesli::size_codec.encode(std::size_t{0});
        auto dec_zero = muesli::size_codec.decode(enc_zero);
        assert(dec_zero == 0);

        auto enc_max = muesli::size_codec.encode(std::numeric_limits<std::size_t>::max());
        auto dec_max = muesli::size_codec.decode(enc_max);
        assert(dec_max == std::numeric_limits<std::size_t>::max());
    }

    // ptrdiff_codec
    {
        auto enc = muesli::ptrdiff_codec.encode(std::ptrdiff_t{-4});
        auto dec = muesli::ptrdiff_codec.decode(enc);
        assert(dec == std::ptrdiff_t{-4});

        auto enc_pos = muesli::ptrdiff_codec.encode(std::ptrdiff_t{1024});
        auto dec_pos = muesli::ptrdiff_codec.decode(enc_pos);
        assert(dec_pos == std::ptrdiff_t{1024});

        auto enc_zero = muesli::ptrdiff_codec.encode(std::ptrdiff_t{0});
        auto dec_zero = muesli::ptrdiff_codec.decode(enc_zero);
        assert(dec_zero == 0);
    }

    // nullptr_codec (encodes std::nullptr_t)
    {
        auto enc = muesli::nullptr_codec.encode(nullptr);
        auto dec = muesli::nullptr_codec.decode(enc);
        // nullptr comparison
        assert(dec == nullptr);
    }

    // byte_codec
    {
        auto enc = muesli::byte_codec.encode(std::byte{42});
        auto dec = muesli::byte_codec.decode(enc);
        assert(dec == std::byte{42});

        auto enc_zero = muesli::byte_codec.encode(std::byte{0});
        auto dec_zero = muesli::byte_codec.decode(enc_zero);
        assert(dec_zero == std::byte{0});
    }

    return 0;
}
