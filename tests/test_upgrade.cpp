/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/upgrade/apply>
#include <muesli/upgrade/registry>
#include <muesli/upgrade/types>

#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace up = muesli::upgrade;

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

struct string_version_comparator {
    constexpr bool equal(const std::string& lhs, const std::string& rhs) const noexcept {
        return lhs == rhs;
    }

    constexpr bool less(const std::string& lhs, const std::string& rhs) const noexcept {
        return lhs < rhs;
    }
};

using agnostic_value = std::map<std::string, std::int64_t>;

static bool set_field_operation(agnostic_value& value, const up::upgrader_step& step) {
    if (step.target_path.size() != 1) {
        return false;
    }

    const auto* key = std::get_if<std::string>(&step.target_path[0]);
    if (key == nullptr) {
        return false;
    }

    const auto arg = up::get_argument_as<std::int64_t>(step, "value");
    if (!arg.has_value()) {
        return false;
    }

    value[*key] = *arg;
    return true;
}

static bool add_field_operation(agnostic_value& value, const up::upgrader_step& step) {
    if (step.target_path.size() != 1) {
        return false;
    }

    const auto* key = std::get_if<std::string>(&step.target_path[0]);
    if (key == nullptr) {
        return false;
    }

    const auto arg = up::get_argument_as<std::int64_t>(step, "delta");
    if (!arg.has_value()) {
        return false;
    }

    value[*key] += *arg;
    return true;
}

int main() {
    // Semver migration chain
    {
        up::operation_registry<agnostic_value> registry;
        assert(registry.register_operation("set_field", set_field_operation));
        assert(registry.register_operation("add_field", add_field_operation));

        std::vector<up::upgrader_document<semver>> docs;
        docs.push_back(up::upgrader_document<semver>{
            semver{1, 0, 0},
            semver{2, 0, 0},
            std::vector<up::upgrader_step>{
                up::upgrader_step{"set_field", {std::string{"schema_version"}}, {{"value", std::int64_t{2}}}}
            },
            "v1_to_v2",
            "Set schema version to 2"
        });

        docs.push_back(up::upgrader_document<semver>{
            semver{2, 0, 0},
            semver{3, 0, 0},
            std::vector<up::upgrader_step>{
                up::upgrader_step{"set_field", {std::string{"schema_version"}}, {{"value", std::int64_t{3}}}},
                up::upgrader_step{"add_field", {std::string{"counter"}}, {{"delta", std::int64_t{10}}}}
            },
            "v2_to_v3",
            "Bump version and add counter offset"
        });

        const semver start{1, 0, 0};
        const semver target{3, 0, 0};
        const semver_comparator cmp{};

        auto path = up::find_upgrade_path(docs, start, target, cmp);
        assert(path.has_value());
        assert(path->size() == 2);

        agnostic_value input{{"schema_version", 1}, {"counter", 1}};
        auto upgraded = up::apply_upgrades(input, docs, start, target, cmp, registry);
        assert(upgraded.has_value());
        assert(upgraded->at("schema_version") == 3);
        assert(upgraded->at("counter") == 11);
    }

    // String version migration with custom comparator
    {
        up::operation_registry<agnostic_value> registry;
        assert(registry.register_operation("set_field", set_field_operation));

        std::vector<up::upgrader_document<std::string>> docs;
        docs.push_back(up::upgrader_document<std::string>{
            "alpha",
            "beta",
            std::vector<up::upgrader_step>{
                up::upgrader_step{"set_field", {std::string{"schema_version"}}, {{"value", std::int64_t{2}}}}
            },
            "alpha_to_beta",
            "String version upgrade"
        });

        docs.push_back(up::upgrader_document<std::string>{
            "beta",
            "release",
            std::vector<up::upgrader_step>{
                up::upgrader_step{"set_field", {std::string{"schema_version"}}, {{"value", std::int64_t{3}}}}
            },
            "beta_to_release",
            "String version upgrade"
        });

        agnostic_value input{{"schema_version", 1}};
        auto upgraded = up::apply_upgrades(
            input,
            docs,
            std::string{"alpha"},
            std::string{"release"},
            string_version_comparator{},
            registry
        );

        assert(upgraded.has_value());
        assert(upgraded->at("schema_version") == 3);
    }

    // Missing operation should fail execution
    {
        up::operation_registry<agnostic_value> registry;

        std::vector<up::upgrader_document<int>> docs;
        docs.push_back(up::upgrader_document<int>{
            1,
            2,
            std::vector<up::upgrader_step>{
                up::upgrader_step{"unknown_op", {std::string{"schema_version"}}, {{"value", std::int64_t{2}}}}
            },
            "v1_to_v2",
            "Should fail"
        });

        struct int_comparator {
            constexpr bool equal(const int lhs, const int rhs) const noexcept { return lhs == rhs; }
            constexpr bool less(const int lhs, const int rhs) const noexcept { return lhs < rhs; }
        };

        agnostic_value input{{"schema_version", 1}};
        auto upgraded = up::apply_upgrades(input, docs, 1, 2, int_comparator{}, registry);
        assert(!upgraded.has_value());
    }

    return 0;
}

