#include <muesli/projector_codec>
#include <muesli/fundamental_codecs>

#include <cassert>
#include <string>

struct user {
    int id;
    std::string name;
};

int main() {
    // Test projector_codec encodes the projected value
    {
        auto id_codec = muesli::int_codec.project<user>([](const user& u) -> int { return u.id; });
        int id_value = 9;
        auto enc = id_codec.encode(id_value);
        auto dec = id_codec.decode(enc);
        assert(dec == id_value);
    }

    // Test projector_codec with different values
    {
        auto id_codec = muesli::int_codec.project<user>([](const user& u) -> int { return u.id; });
        int id_value = 42;
        auto enc = id_codec.encode(id_value);
        auto dec = id_codec.decode(enc);
        assert(dec == 42);
    }

    // Test projector_codec with zero
    {
        auto id_codec = muesli::int_codec.project<user>([](const user& u) -> int { return u.id; });
        int id_value = 0;
        auto enc = id_codec.encode(id_value);
        auto dec = id_codec.decode(enc);
        assert(dec == 0);
    }

    return 0;
}
