#include <muesli/nullable_codec>
#include <muesli/fundamental_codecs>

#include <memory>
#include <cassert>

int main() {
    auto codec = muesli::make_nullable<std::shared_ptr>(muesli::int_codec);
    std::shared_ptr<int> p = std::make_shared<int>(33);
    auto e1 = codec.encode(p);
    auto d1 = codec.decode(std::move(e1));
    assert(d1 && *d1 == 33);

    std::shared_ptr<int> pnull;
    auto e2 = codec.encode(pnull);
    auto d2 = codec.decode(std::move(e2));
    assert(!d2);

    // Test with unique_ptr
    {
        auto codec_uptr = muesli::make_nullable<std::unique_ptr>(muesli::int_codec);
        std::unique_ptr<int> up = std::make_unique<int>(55);
        auto e = codec_uptr.encode(std::move(up));
        auto d = codec_uptr.decode(std::move(e));
        assert(d && *d == 55);
    }

    // Test with optional
    {
        auto codec_opt = muesli::make_nullable<std::optional>(muesli::int_codec);
        std::optional<int> opt = 77;
        auto e = codec_opt.encode(std::move(opt));
        auto d = codec_opt.decode(std::move(e));
        assert(d && *d == 77);

        std::optional<int> opt_empty;
        auto e_empty = codec_opt.encode(std::move(opt_empty));
        auto d_empty = codec_opt.decode(std::move(e_empty));
        assert(!d_empty);
    }

    // Test multiple encode/decode cycles
    {
        auto codec2 = muesli::make_nullable<std::shared_ptr>(muesli::int_codec);

        std::shared_ptr<int> p1 = std::make_shared<int>(10);
        auto e1 = codec2.encode(p1);
        auto d1 = codec2.decode(std::move(e1));
        assert(*d1 == 10);

        std::shared_ptr<int> p2;
        auto e2 = codec2.encode(p2);
        auto d2 = codec2.decode(std::move(e2));
        assert(!d2);
    }

     return 0;
 }
