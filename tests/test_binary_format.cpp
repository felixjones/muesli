#include <muesli/format/binary_format>
#include <muesli/fundamental_codecs>

#include <sstream>
#include <cassert>

int main() {
    // Test basic int serialization
    {
        auto format = muesli::make_binary_format<char>(muesli::int_codec);
        std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
        int value = 77;

        bool ok = format.serialize(value, ss);
        assert(ok);

        ss.seekg(0);
        auto decoded = format.deserialize(ss);
        assert(decoded.has_value());
        assert(*decoded == value);
    }

    // Test with float codec
    {
        auto format_f = muesli::make_binary_format<char>(muesli::float_codec);
        std::stringstream ss_f(std::ios::in | std::ios::out | std::ios::binary);
        float fvalue = 3.14f;

        bool ok_f = format_f.serialize(fvalue, ss_f);
        assert(ok_f);

        ss_f.seekg(0);
        auto decoded_f = format_f.deserialize(ss_f);
        assert(decoded_f.has_value());
        assert(*decoded_f == 3.14f);
    }

    // Test multiple serialize/deserialize
    {
        auto format = muesli::make_binary_format<char>(muesli::int_codec);
        std::stringstream ss2(std::ios::in | std::ios::out | std::ios::binary);
        int v1 = 100;
        int v2 = 200;
        int v3 = 300;

        format.serialize(v1, ss2);
        format.serialize(v2, ss2);
        format.serialize(v3, ss2);

        ss2.seekg(0);
        auto d1 = format.deserialize(ss2);
        auto d2 = format.deserialize(ss2);
        auto d3 = format.deserialize(ss2);
        assert(*d1 == v1);
        assert(*d2 == v2);
        assert(*d3 == v3);
    }

    // Test with byte type
    {
        auto format_byte = muesli::make_binary_format<char>(muesli::int_codec);
        std::stringstream ss_b(std::ios::in | std::ios::out | std::ios::binary);
        int val_b = 42;

        bool ok_b = format_byte.serialize(val_b, ss_b);
        assert(ok_b);

        ss_b.seekg(0);
        auto dec_b = format_byte.deserialize(ss_b);
        assert(dec_b.has_value());
        assert(*dec_b == val_b);
    }

    return 0;
}
