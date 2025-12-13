#include <muesli/string_codecs>

#include <cassert>
#include <string>

int main() {
    auto enc = muesli::string_codec.encode(std::string{"hi"});
    auto dec = muesli::string_codec.decode(std::move(enc));
    assert(dec == "hi");

    auto u8enc = muesli::u8string_codec.encode(std::u8string{u8"ok"});
    auto u8dec = muesli::u8string_codec.decode(std::move(u8enc));
    assert(u8dec == std::u8string{u8"ok"});

    // Test empty strings
    {
        auto enc_empty = muesli::string_codec.encode(std::string{});
        auto dec_empty = muesli::string_codec.decode(std::move(enc_empty));
        assert(dec_empty.empty());
        assert(dec_empty == "");
    }

    // Test longer strings
    {
        std::string long_str = "The quick brown fox jumps over the lazy dog";
        auto enc_long = muesli::string_codec.encode(long_str);
        auto dec_long = muesli::string_codec.decode(std::move(enc_long));
        assert(dec_long == long_str);
    }

    // Test strings with special characters
    {
        std::string special = "!@#$%^&*()[]{}";
        auto enc_special = muesli::string_codec.encode(special);
        auto dec_special = muesli::string_codec.decode(std::move(enc_special));
        assert(dec_special == special);
    }

    // Test u16string_codec
    {
        auto enc_u16 = muesli::u16string_codec.encode(std::u16string{u"test"});
        auto dec_u16 = muesli::u16string_codec.decode(std::move(enc_u16));
        assert(dec_u16 == std::u16string{u"test"});
    }

    // Test u32string_codec
    {
        auto enc_u32 = muesli::u32string_codec.encode(std::u32string{U"data"});
        auto dec_u32 = muesli::u32string_codec.decode(std::move(enc_u32));
        assert(dec_u32 == std::u32string{U"data"});
    }

    return 0;
}
