/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/or_else_codec>
#include <muesli/constrained_codec>
#include <muesli/fundamental_codecs>
#include <muesli/string_codecs>

#include <cassert>
#include <string>

int main() {
    // -- 1: or_else with callable supplier -------------------------------
    {
        auto codec = muesli::int_codec.or_else([]{ return 42; });
        auto fallback = codec.get_fallback();
        assert(fallback == 42);
    }

    // -- 2: or_else with plain value -------------------------------------
    {
        auto codec = muesli::int_codec.or_else(42);
        auto fallback = codec.get_fallback();
        assert(fallback == 42);
    }

    // -- 3: or_else value round-trip (encode/decode passthrough) ---------
    {
        auto codec = muesli::int_codec.or_else(100);
        auto enc = codec.encode(55);
        auto dec = codec.decode(enc);
        assert(dec == 55);
    }

    // -- 4: or_else callable round-trip ----------------------------------
    {
        auto codec = muesli::int_codec.or_else([]{ return 100; });
        auto enc = codec.encode(55);
        auto dec = codec.decode(enc);
        assert(dec == 55);
    }

    // -- 5: or_else with string value ------------------------------------
    {
        auto codec = muesli::string_codec.or_else(std::string("fallback"));
        auto fallback = codec.get_fallback();
        assert(fallback == "fallback");
    }

    // -- 6: or_else with string callable ---------------------------------
    {
        auto codec = muesli::string_codec.or_else([]{ return std::string("fallback"); });
        auto fallback = codec.get_fallback();
        assert(fallback == "fallback");
    }

    // -- 7: or_else preserves next_codec ---------------------------------
    {
        auto codec = muesli::int_codec.or_else(42);
        auto next = codec.next_codec();
        static_assert(muesli::Codec<decltype(next)>);
    }

    // -- 8: or_else catches constraint violations (callable) -------------
    {
        auto codec = muesli::int_codec
            .constrain([](int v) {
                if (v <= 0) throw std::invalid_argument("must be positive");
                return true;
            })
            .or_else([]{ return 1; });

        auto dec = codec.decode(42);
        assert(dec == 42);

        auto bad_dec = codec.decode(-5);
        assert(bad_dec == 1);
    }

    // -- 9: or_else catches constraint violations (value) ----------------
    {
        auto codec = muesli::int_codec
            .constrain([](int v) {
                if (v <= 0) throw std::invalid_argument("must be positive");
                return true;
            })
            .or_else(1);

        auto dec = codec.decode(42);
        assert(dec == 42);

        auto bad_dec = codec.decode(-5);
        assert(bad_dec == 1);
    }

    // -- 10: or_else round-trip with constrained codec -------------------
    {
        auto base = muesli::int_codec
            .constrain([](int v) {
                if (v < 0) throw std::invalid_argument("must be non-negative");
                return true;
            });
        auto codec = base.or_else(0);

        // Encode a valid value through constrained codec, round-trip succeeds
        auto enc = base.encode(42);
        auto dec = codec.decode(enc);
        assert(dec == 42);

        // Encode -1 without constraint, decode through constrained triggers fallback
        auto enc_bad = muesli::int_codec.encode(-1);
        auto bad_dec = codec.decode(enc_bad);
        assert(bad_dec == 0);
    }

    // -- 11: or_else_make single-arg ctor (const char*) ------------------
    {
        auto codec = muesli::string_codec.or_else_make("fallback");
        auto fallback = codec.get_fallback();
        assert(fallback == "fallback");
    }

    // -- 12: or_else_make multi-arg ctor (repeated chars) ----------------
    {
        auto codec = muesli::string_codec.or_else_make(5, 'x');
        auto fallback = codec.get_fallback();
        assert(fallback == "xxxxx");
    }

    // -- 13: or_else_make with int (trivial single-arg) ------------------
    {
        auto codec = muesli::int_codec.or_else_make(99);
        auto fallback = codec.get_fallback();
        assert(fallback == 99);
    }

    // -- 14: or_else_make with zero args (default-constructs) ------------
    {
        auto codec = muesli::string_codec.or_else_make();
        auto fallback = codec.get_fallback();
        assert(fallback.empty());
    }

    // -- 15: or_else_make with constraint violation recovery -------------
    {
        auto codec = muesli::int_codec
            .constrain([](int v) {
                if (v <= 0) throw std::invalid_argument("must be positive");
                return true;
            })
            .or_else_make(42);

        auto dec = codec.decode(10);
        assert(dec == 10);

        auto bad_dec = codec.decode(-1);
        assert(bad_dec == 42);
    }

    return 0;
}
