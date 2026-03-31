/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/codecs>
#include <muesli/format/binary_format>
#include <muesli/schema/adapter_codec>
#include <muesli/schema/extract>
#include <muesli/schema/make_codec>
#include <muesli/upgrade/apply>
#include <muesli/upgrade/builtins>
#include <muesli/upgrade/types>
#include <muesli/upgrade/literals>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace mu = muesli;
namespace up = muesli::upgrade;
using namespace muesli::upgrade::literals;

struct semver {
    int major;
    int minor;
    int patch;
};

struct semver_comparator {
    constexpr bool equal(const semver& lhs, const semver& rhs) const noexcept {
        return std::tie(lhs.major, lhs.minor, lhs.patch) == std::tie(rhs.major, rhs.minor, rhs.patch);
    }

    constexpr bool less(const semver& lhs, const semver& rhs) const noexcept {
        return std::tie(lhs.major, lhs.minor, lhs.patch) < std::tie(rhs.major, rhs.minor, rhs.patch);
    }
};

struct profile {
    std::int32_t schema_version;
    std::string username;
    std::optional<std::string> email;

    static constexpr auto codec = mu::tuple_codec(
        mu::int32_codec.member<&profile::schema_version>().named("schema_version"),
        mu::string_codec.member<&profile::username>().named("username"),
        mu::optional_codec(mu::string_codec).member<&profile::email>().named("email")
    ).apply<profile>();
};

int main() {
    constexpr auto schema = mu::make_schema(profile::codec);
    constexpr auto schema_codec = mu::make_codec(schema);

    using agnostic_value = std::decay_t<decltype(schema_codec)>::value_type;

    // Simulate old payload from disk (v1 has no email and schema_version=1)
    profile old_data{1, "michael", std::nullopt};
    auto fmt_orig = mu::make_binary_format<char>(profile::codec);

    std::stringstream oldBytes(std::ios::in | std::ios::out | std::ios::binary);
    assert(fmt_orig.serialize(old_data, oldBytes));

    // Deserialize as agnostic schema value
    auto fmt_schema = mu::make_binary_format<char>(schema_codec);
    std::istringstream oldInput(oldBytes.str(), std::ios::binary);
    auto agnostic_old = fmt_schema.deserialize(oldInput);
    assert(agnostic_old.has_value());

    // Register generic built-in operations -- no lambdas needed!
    up::operation_registry<agnostic_value> registry;
    up::register_builtins(registry);

    // Declare upgrade: v1 -> v2
    //   Element 0 = schema_version (int32)
    //   Element 1 = username (string)
    //   Element 2 = email (string, was optional)
    std::vector<up::upgrader_document<semver>> docs;
    docs.push_back(up::upgrader_document{
        semver{1, 0, 0},
        semver{2, 0, 0},
        {
            up::upgrader_step{"set",    {std::size_t{0}}, {"value"_arg(2)}},
            up::upgrader_step{"format", {std::size_t{2}}, {"template"_arg("{1}@example.com")}},
        },
        "profile_v1_to_v2",
        "Set schema version to 2 and derive email from username"
    });

    auto agnostic_upgraded = up::apply_upgrades(
        *agnostic_old,
        docs,
        semver{1, 0, 0},
        semver{2, 0, 0},
        semver_comparator{},
        registry
    );
    assert(agnostic_upgraded.has_value());

    // Convert upgraded agnostic tuple back to real typed struct
    auto adapter = mu::make_schema_adapter(profile::codec, schema_codec);
    auto typed = adapter.decode(*agnostic_upgraded);
    assert(typed.has_value());

    std::cout << "Upgraded profile\n";
    std::cout << "  schema_version: " << typed->schema_version << "\n";
    std::cout << "  username:       " << typed->username << "\n";
    std::cout << "  email:          " << (typed->email ? *typed->email : "(none)") << "\n";

    return 0;
}
