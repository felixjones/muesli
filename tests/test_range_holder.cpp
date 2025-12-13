#include <muesli/util/range_holder>

#include <cassert>
#include <vector>
#include <array>

int main() {
    std::vector<int> v{1, 2, 3};
    muesli::range_holder<int> rh(v);
    std::vector<int> out(rh.begin(), rh.end());
    assert(out == v);

    // Test with empty vector
    {
        std::vector<int> empty;
        muesli::range_holder<int> rh_empty(empty);
        std::vector<int> out_empty(rh_empty.begin(), rh_empty.end());
        assert(out_empty.empty());
    }

    // Test with single element
    {
        std::vector<int> single{42};
        muesli::range_holder<int> rh_single(single);
        int count = 0;
        for (int val : rh_single) {
            assert(val == 42);
            ++count;
        }
        assert(count == 1);
    }

    // Test iteration multiple times
    {
        std::vector<int> data{10, 20, 30};
        muesli::range_holder<int> rh_multi(data);

        // First iteration
        std::vector<int> first_pass(rh_multi.begin(), rh_multi.end());
        assert(first_pass == data);
    }

    // Test with different container type (array)
    {
        std::array<int, 3> arr{5, 6, 7};
        muesli::range_holder<int> rh_arr(arr);
        std::vector<int> out_arr(rh_arr.begin(), rh_arr.end());
        assert(out_arr.size() == 3);
        assert(out_arr[0] == 5);
        assert(out_arr[1] == 6);
        assert(out_arr[2] == 7);
    }

    return 0;
}
