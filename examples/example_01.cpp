#include <muesli/codecs>
#include <muesli/format/binary_format>

#include <fstream>
#include <memory>
#include <unordered_map>

namespace mu = muesli;

struct my_record {
    uint8_t x, y;
    float z;

    static constexpr auto codec = mu::tuple_codec(
        mu::uint8_codec.member<&my_record::x>(),
        mu::uint8_codec.member<&my_record::y>(),
        mu::float_codec.member<&my_record::z>()
    ).apply<my_record>();
};

struct some_data {
    int32_t id;
    std::shared_ptr<std::unordered_map<uint32_t, my_record>> data;

    static constexpr auto codec = mu::tuple_codec(
        mu::int32_codec.transform_input([](int32_t) {
            static int32_t idGen = 0;
            return idGen++;
        }).member<&some_data::id>(),
        mu::make_nullable_range<std::shared_ptr>(
            mu::vector_of(mu::pair_codec(
                mu::uint32_codec,
                my_record::codec
            )).apply<std::unordered_map<uint32_t, my_record>>()
        ).member<&some_data::data>()
    ).apply<some_data>();
};

int main() {
    static constexpr auto format = mu::make_binary_format<char>(some_data::codec);

    std::ofstream os("out.bin", std::ios::binary);

    some_data myData;
    format.serialize(myData, os);
}
