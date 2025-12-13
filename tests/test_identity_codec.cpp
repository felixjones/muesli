#include <muesli/identity_codec>

#include <cassert>
#include <string>

int main() {
    // identity_codec<int>
    {
        muesli::identity_codec<int> codec;
        static_assert(muesli::Codec<decltype(codec)>);

        auto enc = codec.encode(5);
        auto dec = codec.decode(enc);
        assert(dec == 5);

        auto enc2 = codec.encode(-99);
        auto dec2 = codec.decode(enc2);
        assert(dec2 == -99);
    }

    // identity_codec<float>
    {
        muesli::identity_codec<float> codec;
        auto enc = codec.encode(2.5f);
        auto dec = codec.decode(enc);
        assert(dec == 2.5f);
    }

    // identity_codec<std::string>
    {
        muesli::identity_codec<std::string> codec;
        auto enc = codec.encode(std::string{"hello"});
        auto dec = codec.decode(enc);
        assert(dec == "hello");
    }

    // Test rvalue and const lvalue overloads
    {
        muesli::identity_codec<int> codec;

        // rvalue encode
        int v1 = 10;
        auto enc1 = codec.encode(std::move(v1));
        assert(enc1 == 10);

        // const lvalue encode
        const int cv = 20;
        auto enc2 = codec.encode(cv);
        assert(enc2 == 20);

        // rvalue decode
        int v2 = 30;
        auto dec1 = codec.decode(std::move(v2));
        assert(dec1 == 30);

        // const lvalue decode
        const int cv2 = 40;
        auto dec2 = codec.decode(cv2);
        assert(dec2 == 40);
    }

    // Verify next_codec returns itself
    {
        muesli::identity_codec<int> codec;
        auto next = codec.next_codec();
        static_assert(std::same_as<decltype(next), muesli::identity_codec<int>>);

        auto enc = next.encode(77);
        auto dec = next.decode(enc);
        assert(dec == 77);
    }

    return 0;
}
