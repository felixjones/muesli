#include <muesli/monostate_codec>

#include <cassert>
#include <variant>

int main() {
    // Test monostate_codec
    {
        std::monostate m;
        auto enc = muesli::monostate_codec.encode(m);
        auto dec = muesli::monostate_codec.decode(enc);
        assert(true);
    }

    return 0;
}
