#include <muesli/codecs>
#include <muesli/format/msgpack_format>
#include <muesli/format/msgpack_timestamp_ext>

#include <chrono>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace mu = muesli;

struct address {
    std::string city;
    std::string country;

    static constexpr auto codec = mu::tuple_codec(
        mu::string_codec.member<&address::city>().named("city"),
        mu::string_codec.member<&address::country>().named("country")
    ).apply<address>();
};

struct person {
    std::string name;
    std::chrono::sys_days date_of_birth;
    std::optional<std::string> email;
    address home;
    std::vector<std::string> tags;

    static constexpr auto codec = mu::tuple_codec(
        mu::string_codec.member<&person::name>().named("name"),
        mu::member_of<&person::date_of_birth>(
            mu::make_msgpack_timestamp_ext_codec<std::chrono::days>()
        ).named("date_of_birth"),
        mu::optional_codec(mu::string_codec).member<&person::email>().named("email"),
        address::codec.member<&person::home>().named("home"),
        mu::vector_of(mu::string_codec).member<&person::tags>().named("tags")
    ).apply<person>();
};

static constexpr std::chrono::sys_days make_date(int year, unsigned month, unsigned day) {
    return std::chrono::sys_days{std::chrono::year{year} / month / day};
}

int main() {
    static constexpr auto format = mu::make_msgpack_format<char>(person::codec);

    person michael{
        "Michael",
        make_date(1995, 5, 5),
        "michael@example.com",
        {"London", "UK"},
        {"developer", "film student"}
    };

    std::stringstream encoded(std::ios::in | std::ios::out | std::ios::binary);
    if (!format.serialize(michael, encoded)) {
        std::cout << "Serialize failed\n";
        return 1;
    }

    std::cout << "Serialized " << encoded.str().size() << " bytes\n\n";

    person pichael{
        "Pichael",
        make_date(1993, 3, 3),
        std::nullopt,
        {"Bradford", "UK"},
        {"confidence man"}
    };

    std::stringstream input(std::ios::in | std::ios::out | std::ios::binary);
    if (!format.serialize(pichael, input)) {
        std::cout << "Input serialize failed\n";
        return 1;
    }

    if (auto result = format.deserialize(input)) {
        auto& [name, dateOfBirth, email, home, tags] = *result;
        auto ymd = std::chrono::year_month_day{dateOfBirth};
        std::cout << "Deserialized:\n";
        std::cout << "  name:    " << name << "\n";
        std::cout << "  dob:     "
                  << int(ymd.year()) << "-"
                  << static_cast<unsigned>(ymd.month()) << "-"
                  << static_cast<unsigned>(ymd.day()) << "\n";
        std::cout << "  email:   " << (email ? *email : "(none)") << "\n";
        std::cout << "  home:    " << home.city << ", " << home.country << "\n";
        std::cout << "  tags:    ";
        for (const auto& t : tags) std::cout << t << " ";
        std::cout << "\n";
    }
}

