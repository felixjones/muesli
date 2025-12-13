#include <muesli/nullable_codec>
#include <muesli/fundamental_codecs>

#include <cassert>
#include <memory>

int main() {
    // Test make_nullable with shared_ptr
    {
        auto codec = muesli::make_nullable<std::shared_ptr>(muesli::int_codec);
        std::shared_ptr<int> ptr = std::make_shared<int>(42);
        auto enc = codec.encode(ptr);
        auto dec = codec.decode(std::move(enc));
        assert(dec && *dec == 42);
    }

    // Test make_nullable with shared_ptr null
    {
        auto codec = muesli::make_nullable<std::shared_ptr>(muesli::int_codec);
        std::shared_ptr<int> null_ptr;
        auto enc = codec.encode(null_ptr);
        auto dec = codec.decode(std::move(enc));
        assert(!dec);
    }

    // Test make_nullable with unique_ptr
    {
        auto codec = muesli::make_nullable<std::unique_ptr>(muesli::int_codec);
        std::unique_ptr<int> ptr = std::make_unique<int>(99);
        auto enc = codec.encode(std::move(ptr));
        auto dec = codec.decode(std::move(enc));
        assert(dec && *dec == 99);
    }

    // Test make_nullable with optional
    {
        auto codec = muesli::make_nullable<std::optional>(muesli::float_codec);
        std::optional<float> opt(3.5f);
        auto enc = codec.encode(std::move(opt));
        auto dec = codec.decode(std::move(enc));
        assert(dec && *dec == 3.5f);
    }

    // Test make_nullable with optional empty
    {
        auto codec = muesli::make_nullable<std::optional>(muesli::float_codec);
        std::optional<float> empty_opt;
        auto enc = codec.encode(std::move(empty_opt));
        auto dec = codec.decode(std::move(enc));
        assert(!dec);
    }

    // Test make_nullable with bool codec
    {
        auto codec = muesli::make_nullable<std::shared_ptr>(muesli::bool_codec);
        std::shared_ptr<bool> ptr = std::make_shared<bool>(true);
        auto enc = codec.encode(ptr);
        auto dec = codec.decode(std::move(enc));
        assert(dec && *dec == true);
    }

    // Test multiple cycles with different values
    {
        auto codec = muesli::make_nullable<std::shared_ptr>(muesli::int_codec);

        std::shared_ptr<int> p1 = std::make_shared<int>(10);
        auto e1 = codec.encode(p1);
        auto d1 = codec.decode(std::move(e1));
        assert(*d1 == 10);

        std::shared_ptr<int> p2 = std::make_shared<int>(20);
        auto e2 = codec.encode(p2);
        auto d2 = codec.decode(std::move(e2));
        assert(*d2 == 20);

        std::shared_ptr<int> p3;
        auto e3 = codec.encode(p3);
        auto d3 = codec.decode(std::move(e3));
        assert(!d3);
    }

    return 0;
}
