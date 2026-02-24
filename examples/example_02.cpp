#include <muesli/codecs>
#include <muesli/format/json_format>
#include <muesli/format/nlohmann_json_backend>

#include <iostream>
#include <optional>
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
    int32_t age;
    std::optional<std::string> email;
    address home;
    std::vector<std::string> tags;

    static constexpr auto codec = mu::tuple_codec(
        mu::string_codec.member<&person::name>().named("name"),
        mu::int32_codec.member<&person::age>().named("age"),
        mu::optional_codec(mu::string_codec).member<&person::email>().named("email"),
        address::codec.member<&person::home>().named("home"),
        mu::vector_of(mu::string_codec).member<&person::tags>().named("tags")
    ).apply<person>();
};

int main() {
    static constexpr auto format = mu::make_json_format<mu::nlohmann_json_backend>(person::codec);

    person michael{
        "Michael",
        30,
        "michael@example.com",
        {"London", "UK"},
        {"developer", "film student"}
    };

    auto j = format.serialize(michael);
    std::cout << "Serialized:\n" << j.dump(2) << "\n\n";

    auto input = nlohmann::json::parse(R"({
        "name": "Pichael",
        "age": 31,
        "email": null,
        "home": { "city": "Bradford", "country": "UK" },
        "tags": ["confidence man"]
    })");

    if (auto result = format.deserialize(input)) {
        auto& [name, age, email, home, tags] = *result;
        std::cout << "Deserialized:\n";
        std::cout << "  name:    " << name << "\n";
        std::cout << "  age:     " << age << "\n";
        std::cout << "  email:   " << (email ? *email : "(none)") << "\n";
        std::cout << "  home:    " << home.city << ", " << home.country << "\n";
        std::cout << "  tags:    ";
        for (const auto& t : tags) std::cout << t << " ";
        std::cout << "\n";
    }
}
