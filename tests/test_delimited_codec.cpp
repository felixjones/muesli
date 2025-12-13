#include <muesli/delimited_codec>
#include <muesli/fundamental_codecs>
#include <muesli/util/range_holder>

#include <cassert>
#include <string>

int main() {
    // Test delimited_codec with character-delimited string
    {
        auto codec = muesli::delimited_codec(muesli::char_codec, [](char c) { return c == ','; });
        std::string input = "abc";
        muesli::range_holder<char> holder(input);
        auto enc = codec.encode(std::move(holder));
        auto dec = codec.decode(std::move(enc));
        std::string result(dec.begin(), dec.end());
        assert(result == input);
    }

    // Test delimited_codec with empty input
    {
        auto codec = muesli::delimited_codec(muesli::char_codec, [](char c) { return c == ','; });
        std::string input = "";
        muesli::range_holder<char> holder(input);
        auto enc = codec.encode(std::move(holder));
        auto dec = codec.decode(std::move(enc));
        std::string result(dec.begin(), dec.end());
        assert(result.empty());
    }

    return 0;
}
