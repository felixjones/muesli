#include <muesli/member_codec>
#include <muesli/fundamental_codecs>
#include <muesli/string_codecs>
#include <muesli/apply_codec>
#include <muesli/tuple_codec>

#include <cassert>
#include <string>

struct widget {
    int id;
    std::string name;

    static constexpr auto codec = muesli::tuple_codec(
        muesli::int_codec.member<&widget::id>(),
        muesli::string_codec.member<&widget::name>()
    ).apply<widget>();
};

int main() {
    // Test member_codec with basic values
    {
        widget w{5, "abc"};
        auto enc = widget::codec.encode(w);
        auto dec = widget::codec.decode(enc);
        assert(dec.id == 5);
        assert(dec.name == "abc");
    }

    // Test member_codec with different values
    {
        widget w{100, "test"};
        auto enc = widget::codec.encode(w);
        auto dec = widget::codec.decode(enc);
        assert(dec.id == 100);
        assert(dec.name == "test");
    }

    // Test member_codec with empty string
    {
        widget w{0, ""};
        auto enc = widget::codec.encode(w);
        auto dec = widget::codec.decode(enc);
        assert(dec.id == 0);
        assert(dec.name == "");
    }

    return 0;
}
