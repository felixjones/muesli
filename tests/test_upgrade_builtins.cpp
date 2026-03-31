/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/upgrade/accessor>
#include <muesli/upgrade/builtins>
#include <muesli/upgrade/registry>
#include <muesli/upgrade/types>
#include <muesli/upgrade/literals>

#include <cassert>
#include <cstdint>
#include <string>
#include <tuple>

namespace up = muesli::upgrade;
using namespace muesli::upgrade::literals;

int main() {
    // =========================================================================
    // 1. accessor: tuple_get / tuple_set / tuple_stringify
    // =========================================================================
    {
        using T = std::tuple<std::int32_t, std::string, double>;
        T t{42, "hello", 3.14};

        // tuple_get
        auto v0 = up::tuple_get(t, 0);
        assert(v0.has_value());
        assert(std::get<std::int64_t>(*v0) == 42);

        auto v1 = up::tuple_get(t, 1);
        assert(v1.has_value());
        assert(std::get<std::string>(*v1) == "hello");

        auto v2 = up::tuple_get(t, 2);
        assert(v2.has_value());
        assert(std::get<double>(*v2) == 3.14);

        // out of range
        assert(!up::tuple_get(t, 3).has_value());

        // tuple_set
        assert(up::tuple_set(t, 0, up::literal{std::int64_t{99}}));
        assert(std::get<0>(t) == 99);

        assert(up::tuple_set(t, 1, up::literal{std::string{"world"}}));
        assert(std::get<1>(t) == "world");

        assert(up::tuple_set(t, 2, up::literal{double{2.72}}));
        assert(std::get<2>(t) == 2.72);

        // tuple_stringify
        assert(up::tuple_stringify(t, 0) == "99");
        assert(up::tuple_stringify(t, 1) == "world");
    }

    // =========================================================================
    // 2. builtin "set" operation
    // =========================================================================
    {
        using T = std::tuple<std::int32_t, std::string, double>;
        T t{0, "", 0.0};

        up::operation_registry<T> reg;
        up::register_set(reg);

        // set int
        up::upgrader_step step_int{"set", {std::size_t{0}}, {"value"_arg(42)}};
        assert(reg.apply_step(step_int, t));
        assert(std::get<0>(t) == 42);

        // set string
        up::upgrader_step step_str{"set", {std::size_t{1}}, {"value"_arg("hello")}};
        assert(reg.apply_step(step_str, t));
        assert(std::get<1>(t) == "hello");

        // set double
        up::upgrader_step step_dbl{"set", {std::size_t{2}}, {"value"_arg(3.14)}};
        assert(reg.apply_step(step_dbl, t));
        assert(std::get<2>(t) == 3.14);
    }

    // =========================================================================
    // 3. builtin "copy" operation
    // =========================================================================
    {
        using T = std::tuple<std::string, std::string>;
        T t{"source_val", ""};

        up::operation_registry<T> reg;
        up::register_copy(reg);

        up::upgrader_step step{"copy", {std::size_t{1}}, {"source"_arg(idx(0))}};
        assert(reg.apply_step(step, t));
        assert(std::get<1>(t) == "source_val");
    }

    // =========================================================================
    // 4. builtin "format" operation
    // =========================================================================
    {
        using T = std::tuple<std::int32_t, std::string, std::string>;
        T t{42, "michael", ""};

        up::operation_registry<T> reg;
        up::register_format(reg);

        // Template with field index substitution
        up::upgrader_step step{"format", {std::size_t{2}}, {"template"_arg("{1}@example.com")}};
        assert(reg.apply_step(step, t));
        assert(std::get<2>(t) == "michael@example.com");
    }

    // =========================================================================
    // 5. format with multiple placeholders
    // =========================================================================
    {
        using T = std::tuple<std::int32_t, std::string, std::string>;
        T t{7, "alice", ""};

        up::operation_registry<T> reg;
        up::register_format(reg);

        up::upgrader_step step{"format", {std::size_t{2}}, {"template"_arg("user-{1}-v{0}")}};
        assert(reg.apply_step(step, t));
        assert(std::get<2>(t) == "user-alice-v7");
    }

    // =========================================================================
    // 6. builtin "remove" operation
    // =========================================================================
    {
        using T = std::tuple<std::int32_t, std::string>;
        T t{42, "filled"};

        up::operation_registry<T> reg;
        up::register_remove(reg);

        up::upgrader_step step0{"remove", {std::size_t{0}}, {}};
        assert(reg.apply_step(step0, t));
        assert(std::get<0>(t) == 0);

        up::upgrader_step step1{"remove", {std::size_t{1}}, {}};
        assert(reg.apply_step(step1, t));
        assert(std::get<1>(t).empty());
    }

    // =========================================================================
    // 7. register_builtins convenience
    // =========================================================================
    {
        using T = std::tuple<std::int32_t, std::string, std::string>;
        T t{1, "bob", ""};

        up::operation_registry<T> reg;
        up::register_builtins(reg);

        assert(reg.has_operation("set"));
        assert(reg.has_operation("copy"));
        assert(reg.has_operation("format"));
        assert(reg.has_operation("remove"));

        // Full upgrade chain: set version, derive email
        up::upgrader_step s1{"set", {std::size_t{0}}, {"value"_arg(2)}};
        up::upgrader_step s2{"format", {std::size_t{2}}, {"template"_arg("{1}@example.com")}};
        assert(reg.apply_step(s1, t));
        assert(reg.apply_step(s2, t));
        assert(std::get<0>(t) == 2);
        assert(std::get<2>(t) == "bob@example.com");
    }

    // =========================================================================
    // 8. cross-type numeric set (uint64 arg -> int32 element)
    // =========================================================================
    {
        using T = std::tuple<std::int32_t>;
        T t{0};

        up::operation_registry<T> reg;
        up::register_set(reg);

        up::upgrader_step step{"set", {std::size_t{0}}, {"value"_arg(255u)}};
        assert(reg.apply_step(step, t));
        assert(std::get<0>(t) == 255);
    }

    // =========================================================================
    // 9. bool element
    // =========================================================================
    {
        using T = std::tuple<bool, std::string>;
        T t{false, ""};

        up::operation_registry<T> reg;
        up::register_set(reg);

        up::upgrader_step step{"set", {std::size_t{0}}, {"value"_arg(true)}};
        assert(reg.apply_step(step, t));
        assert(std::get<0>(t) == true);
    }

    // =========================================================================
    // 10. optional<string> element (matches real schema agnostic values)
    // =========================================================================
    {
        using T = std::tuple<std::int32_t, std::string, std::optional<std::string>>;
        T t{1, "alice", std::nullopt};

        up::operation_registry<T> reg;
        up::register_builtins(reg);

        // set optional<string> from string literal
        up::upgrader_step s1{"set", {std::size_t{2}}, {"value"_arg("test@mail.com")}};
        assert(reg.apply_step(s1, t));
        assert(std::get<2>(t).has_value());
        assert(*std::get<2>(t) == "test@mail.com");

        // format into optional<string>
        up::upgrader_step s2{"format", {std::size_t{2}}, {"template"_arg("{1}@example.com")}};
        assert(reg.apply_step(s2, t));
        assert(std::get<2>(t).has_value());
        assert(*std::get<2>(t) == "alice@example.com");

        // copy string into optional<string>
        up::upgrader_step s3{"copy", {std::size_t{2}}, {"source"_arg(idx(1))}};
        assert(reg.apply_step(s3, t));
        assert(std::get<2>(t).has_value());
        assert(*std::get<2>(t) == "alice");

        // read optional<string> (via tuple_get)
        auto lit = up::tuple_get(t, 2);
        assert(lit.has_value());
        assert(std::get<std::string>(*lit) == "alice");

        // stringify optional<string>
        assert(up::tuple_stringify(t, 2) == "alice");

        // nullopt reads as monostate
        std::get<2>(t) = std::nullopt;
        auto lit_null = up::tuple_get(t, 2);
        assert(lit_null.has_value());
        assert(std::holds_alternative<std::monostate>(*lit_null));
        assert(up::tuple_stringify(t, 2).empty());
    }

    // =========================================================================
    // 11. optional<int32_t> element
    // =========================================================================
    {
        using T = std::tuple<std::optional<std::int32_t>>;
        T t{std::nullopt};

        up::operation_registry<T> reg;
        up::register_set(reg);

        up::upgrader_step step{"set", {std::size_t{0}}, {"value"_arg(42)}};
        assert(reg.apply_step(step, t));
        assert(std::get<0>(t).has_value());
        assert(*std::get<0>(t) == 42);
    }

    return 0;
}


