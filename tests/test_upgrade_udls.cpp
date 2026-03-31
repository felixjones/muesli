/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/upgrade/types>
#include <muesli/upgrade/literals>

#include <cassert>
#include <cstdint>
#include <string>
#include <variant>

namespace up = muesli::upgrade;
using namespace muesli::upgrade::literals;

int main() {
    // Test integer UDLs
    {
        auto val = 42_i64;
        assert(std::holds_alternative<up::literal>(val));
        const auto& lit = std::get<up::literal>(val);
        assert(std::get_if<std::int64_t>(&lit) != nullptr);
        assert(std::get<std::int64_t>(lit) == 42);
    }

    // Test unsigned integer UDL
    {
        auto val = 100_u64;
        assert(std::holds_alternative<up::literal>(val));
        const auto& lit = std::get<up::literal>(val);
        assert(std::get_if<std::uint64_t>(&lit) != nullptr);
        assert(std::get<std::uint64_t>(lit) == 100);
    }

    // Test float UDL
    {
        auto val = 3.14_f64;
        assert(std::holds_alternative<up::literal>(val));
        const auto& lit = std::get<up::literal>(val);
        assert(std::get_if<double>(&lit) != nullptr);
        assert(std::get<double>(lit) == 3.14);
    }

    // Test string UDL
    {
        auto val = "hello world"_str;
        assert(std::holds_alternative<up::literal>(val));
        const auto& lit = std::get<up::literal>(val);
        assert(std::get_if<std::string>(&lit) != nullptr);
        assert(std::get<std::string>(lit) == "hello world");
    }

    // Test idx() path reference
    {
        auto val = idx(0);
        assert(std::holds_alternative<up::field_reference>(val));
        const auto& ref = std::get<up::field_reference>(val);
        assert(ref.source_path.size() == 1);
        assert(std::get_if<std::size_t>(&ref.source_path[0]) != nullptr);
        assert(std::get<std::size_t>(ref.source_path[0]) == 0);
    }

    // Test key() path reference
    {
        auto val = key("username");
        assert(std::holds_alternative<up::field_reference>(val));
        const auto& ref = std::get<up::field_reference>(val);
        assert(ref.source_path.size() == 1);
        assert(std::get_if<std::string>(&ref.source_path[0]) != nullptr);
        assert(std::get<std::string>(ref.source_path[0]) == "username");
    }

    // Test nested path() reference
    {
        auto val = path({std::string("user"), std::size_t(0), std::string("email")});
        assert(std::holds_alternative<up::field_reference>(val));
        const auto& ref = std::get<up::field_reference>(val);
        assert(ref.source_path.size() == 3);
        assert(std::get<std::string>(ref.source_path[0]) == "user");
        assert(std::get<std::size_t>(ref.source_path[1]) == 0);
        assert(std::get<std::string>(ref.source_path[2]) == "email");
    }

    // Test _arg UDL with plain signed integer binding
    {
        auto step_arg = "count"_arg(42);
        assert(step_arg.name == "count");
        assert(std::holds_alternative<up::literal>(step_arg.value));
        const auto& lit = std::get<up::literal>(step_arg.value);
        assert(std::get<std::int64_t>(lit) == 42);
    }

    // Test _arg UDL with plain C-string binding
    {
        auto step_arg = "template"_arg("hello {name}");
        assert(step_arg.name == "template");
        assert(std::holds_alternative<up::literal>(step_arg.value));
        const auto& lit = std::get<up::literal>(step_arg.value);
        assert(std::get<std::string>(lit) == "hello {name}");
    }

    // Test _arg UDL with std::string binding
    {
        const std::string input = "hello std::string";
        auto step_arg = "template"_arg(input);
        assert(step_arg.name == "template");
        assert(std::holds_alternative<up::literal>(step_arg.value));
        const auto& lit = std::get<up::literal>(step_arg.value);
        assert(std::get<std::string>(lit) == input);
    }

    // Test _arg UDL with plain unsigned integer binding
    {
        auto step_arg = "count"_arg(100u);
        assert(step_arg.name == "count");
        assert(std::holds_alternative<up::literal>(step_arg.value));
        const auto& lit = std::get<up::literal>(step_arg.value);
        assert(std::get<std::uint64_t>(lit) == 100u);
    }

    // Test _arg UDL with plain floating-point binding
    {
        auto step_arg = "ratio"_arg(2.5);
        assert(step_arg.name == "ratio");
        assert(std::holds_alternative<up::literal>(step_arg.value));
        const auto& lit = std::get<up::literal>(step_arg.value);
        assert(std::get<double>(lit) == 2.5);
    }

    // Test _arg UDL with bool binding
    {
        auto step_arg = "enabled"_arg(true);
        assert(step_arg.name == "enabled");
        assert(std::holds_alternative<up::literal>(step_arg.value));
        const auto& lit = std::get<up::literal>(step_arg.value);
        assert(std::get<bool>(lit));
    }

    // Test _arg UDL with field reference binding (idx)
    {
        auto step_arg = "source"_arg(idx(2));
        assert(step_arg.name == "source");
        assert(std::holds_alternative<up::field_reference>(step_arg.value));
        const auto& ref = std::get<up::field_reference>(step_arg.value);
        assert(ref.source_path.size() == 1);
        assert(std::get<std::size_t>(ref.source_path[0]) == 2);
    }

    // Test _arg UDL with field reference binding (key)
    {
        auto step_arg = "field"_arg(key("username"));
        assert(step_arg.name == "field");
        assert(std::holds_alternative<up::field_reference>(step_arg.value));
        const auto& ref = std::get<up::field_reference>(step_arg.value);
        assert(ref.source_path.size() == 1);
        assert(std::get<std::string>(ref.source_path[0]) == "username");
    }

    // Test _arg UDL with operator= syntax and plain signed integer
    {
        auto step_arg = "count"_arg = 99;
        assert(step_arg.name == "count");
        assert(std::holds_alternative<up::literal>(step_arg.value));
        const auto& lit = std::get<up::literal>(step_arg.value);
        assert(std::get<std::int64_t>(lit) == 99);
    }

    // Test _arg UDL with operator= and C-string
    {
        auto step_arg = "template"_arg = "hello assignment";
        assert(step_arg.name == "template");
        assert(std::holds_alternative<up::literal>(step_arg.value));
        const auto& lit = std::get<up::literal>(step_arg.value);
        assert(std::get<std::string>(lit) == "hello assignment");
    }

    // Test _arg UDL with operator= and floating-point
    {
        auto step_arg = "ratio"_arg = 2.5;
        assert(step_arg.name == "ratio");
        assert(std::holds_alternative<up::literal>(step_arg.value));
        const auto& lit = std::get<up::literal>(step_arg.value);
        assert(std::get<double>(lit) == 2.5);
    }

    // Test _arg UDL with operator= and unsigned integer
    {
        auto step_arg = "count"_arg = 100u;
        assert(step_arg.name == "count");
        assert(std::holds_alternative<up::literal>(step_arg.value));
        const auto& lit = std::get<up::literal>(step_arg.value);
        assert(std::get<std::uint64_t>(lit) == 100u);
    }

    // Compatibility check: explicit numeric suffix UDLs still work
    {
        auto step_arg = "count"_arg(77_i64);
        const auto& lit = std::get<up::literal>(step_arg.value);
        assert(std::get<std::int64_t>(lit) == 77);
    }

    return 0;
}



