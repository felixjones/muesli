#include <muesli/transform_codec>
#include <muesli/fundamental_codecs>

#include <cassert>

int main() {
    // Test transform_codec with int doubling
    {
        auto codec = muesli::int_codec.transform(
            [](int v) { return v * 2; },
            [](int v) { return v / 2; }
        );
        auto enc = codec.encode(21);
        auto dec = codec.decode(enc);
        assert(dec == 21);
    }

    // Test transform_codec with negative numbers
    {
        auto codec = muesli::int_codec.transform(
            [](int v) { return v * 2; },
            [](int v) { return v / 2; }
        );
        auto enc = codec.encode(-50);
        auto dec = codec.decode(enc);
        assert(dec == -50);
    }

    // Test transform_codec with zero
    {
        auto codec = muesli::int_codec.transform(
            [](int v) { return v * 2; },
            [](int v) { return v / 2; }
        );
        auto enc = codec.encode(0);
        auto dec = codec.decode(enc);
        assert(dec == 0);
    }

    return 0;
}
