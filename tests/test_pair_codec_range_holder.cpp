/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/tuple_codec>
#include <muesli/range_codec>
#include <muesli/fundamental_codecs>
#include <muesli/util/range_holder>

#include <cassert>
#include <vector>

int main() {
    // Test 1: pair_codec with range_holder members - const-encode path
    {
        // Create a pair of range_holders
        std::vector<int> data1{1, 2, 3};
        std::vector<int> data2{4, 5, 6};

        muesli::range_holder<int> rh1(data1);
        muesli::range_holder<int> rh2(data2);

        using pair_type = std::pair<muesli::range_holder<int>, muesli::range_holder<int>>;
        pair_type pair_val{std::move(rh1), std::move(rh2)};

        auto codec = muesli::pair_codec(
            muesli::range_codec(muesli::int_codec),
            muesli::range_codec(muesli::int_codec)
        );

        // Encode from const reference - this exercises the copy_or_clone path
        const pair_type& const_pair_ref = pair_val;
        auto encoded = codec.encode(const_pair_ref);

        // Verify structure: encoded is a pair of two range_holders
        // Iterate and collect to verify contents
        {
            std::vector<int> first_elements;
            for (auto elem : std::get<0>(encoded)) {
                first_elements.push_back(elem);
            }
            assert(first_elements.size() == 3);
            assert(first_elements[0] == 1);
            assert(first_elements[1] == 2);
            assert(first_elements[2] == 3);
        }

        {
            std::vector<int> second_elements;
            for (auto elem : std::get<1>(encoded)) {
                second_elements.push_back(elem);
            }
            assert(second_elements.size() == 3);
            assert(second_elements[0] == 4);
            assert(second_elements[1] == 5);
            assert(second_elements[2] == 6);
        }
    }

    // Test 2: pair_codec with one range_holder member - mixed copy and clone
    {
        std::vector<int> data1{10, 20, 30};
        muesli::range_holder<int> rh1(data1);

        using pair_type = std::pair<muesli::range_holder<int>, int>;
        pair_type pair_val{std::move(rh1), 42};

        auto codec = muesli::pair_codec(
            muesli::range_codec(muesli::int_codec),
            muesli::int_codec
        );

        // Const-encode to trigger copy_or_clone
        const pair_type& const_pair_ref = pair_val;
        auto encoded = codec.encode(const_pair_ref);

        // Verify first element (range_holder) is cloned correctly
        {
            std::vector<int> elements;
            for (auto elem : std::get<0>(encoded)) {
                elements.push_back(elem);
            }
            assert(elements.size() == 3);
            assert(elements[0] == 10);
            assert(elements[1] == 20);
            assert(elements[2] == 30);
        }

        // Verify second element (int) is copied correctly
        assert(std::get<1>(encoded) == 42);
    }

    // Test 3: rvalue pair_codec encode bypasses copy_or_clone (fast path)
    {
        std::vector<int> data1{100, 200};
        muesli::range_holder<int> rh1(data1);

        using pair_type = std::pair<muesli::range_holder<int>, int>;
        pair_type pair_val{std::move(rh1), 99};

        auto codec = muesli::pair_codec(
            muesli::range_codec(muesli::int_codec),
            muesli::int_codec
        );

        // Rvalue encode - direct move, no copy_or_clone
        auto encoded = codec.encode(std::move(pair_val));

        {
            std::vector<int> elements;
            for (auto elem : std::get<0>(encoded)) {
                elements.push_back(elem);
            }
            assert(elements.size() == 2);
            assert(elements[0] == 100);
            assert(elements[1] == 200);
        }
        assert(std::get<1>(encoded) == 99);
    }

    // Test 4: Round-trip with pair of range_holders
    {
        std::vector<int> data1{7, 8, 9};
        std::vector<int> data2{11, 12, 13};

        muesli::range_holder<int> rh1(data1);
        muesli::range_holder<int> rh2(data2);

        using pair_type = std::pair<muesli::range_holder<int>, muesli::range_holder<int>>;
        pair_type original{std::move(rh1), std::move(rh2)};

        auto codec = muesli::pair_codec(
            muesli::range_codec(muesli::int_codec),
            muesli::range_codec(muesli::int_codec)
        );

        // Encode const reference (tests copy_or_clone)
        const pair_type& const_ref = original;
        auto encoded = codec.encode(const_ref);

        // Decode back
        auto decoded = codec.decode(std::move(encoded));

        // Verify decoded values
        {
            std::vector<int> first_elems;
            for (auto elem : std::get<0>(decoded)) {
                first_elems.push_back(elem);
            }
            assert(first_elems.size() == 3);
            assert(first_elems[0] == 7);
            assert(first_elems[1] == 8);
            assert(first_elems[2] == 9);
        }

        {
            std::vector<int> second_elems;
            for (auto elem : std::get<1>(decoded)) {
                second_elems.push_back(elem);
            }
            assert(second_elems.size() == 3);
            assert(second_elems[0] == 11);
            assert(second_elems[1] == 12);
            assert(second_elems[2] == 13);
        }
    }

    return 0;
}





