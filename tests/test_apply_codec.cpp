// Suppress narrowing conversion warnings in this test (expected behavior)
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"
#endif

#include <muesli/apply_codec>
#include <muesli/fundamental_codecs>

#include <cassert>

int main() {
    // Note: MSVC treats narrowing conversion error (C2397) in C++20 mode
#if !defined(_MSC_VER)
    // Adapt identity<int> to accept float values by constructing int
    {
        auto base = muesli::identity_codec<int>{};
        auto codec = base.apply<float>();
        float v = 42.7f;
        auto enc = codec.encode(v);         // encoded_type = int
        auto dec = codec.decode(enc);       // value_type = float
        assert(static_cast<int>(dec) == 42);
    }

    // Adapt identity<int> to accept double values
    {
        auto base = muesli::identity_codec<int>{};
        auto codec = base.apply<double>();
        double v = -13.9;
        auto enc = codec.encode(v);
        auto dec = codec.decode(enc);
        assert(static_cast<int>(dec) == -13);
    }

    // Adapt identity<int> to accept bool values
    {
        auto base = muesli::identity_codec<int>{};
        auto codec = base.apply<bool>();
        bool v = true;
        auto enc = codec.encode(v);
        auto dec = codec.decode(enc);
        assert(static_cast<int>(dec) == 1);
    }
#endif
    return 0;
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
