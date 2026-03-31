/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/upgrade/codecs>
#include <muesli/upgrade/types>
#include <muesli/upgrade/literals>
#include <muesli/format/binary_format>
#include <muesli/codecs>

#include <cassert>
#include <cstdint>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace mu = muesli;
namespace up = muesli::upgrade;
using namespace muesli::upgrade::literals;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

template<typename Codec, typename T>
std::string to_bytes(const Codec& codec, const T& val) {
    auto fmt = mu::make_binary_format<char>(codec);
    std::ostringstream oss(std::ios::binary);
    assert(fmt.serialize(val, oss));
    return oss.str();
}

template<typename Codec>
auto from_bytes(const Codec& codec, const std::string& bytes) {
    auto fmt = mu::make_binary_format<char>(codec);
    std::istringstream iss(bytes, std::ios::binary);
    return fmt.deserialize(iss);
}

template<typename Codec, typename T>
void round_trip(const Codec& codec, const T& original) {
    auto bytes = to_bytes(codec, original);
    auto decoded = from_bytes(codec, bytes);
    assert(decoded.has_value());
    assert(*decoded == original);
}

// Comparison operators for upgrade types (needed by round_trip assert)
// Must be in the same namespace for ADL to find them in std::vector comparisons.

namespace muesli::upgrade {

inline bool operator==(const field_reference& a, const field_reference& b) {
    return a.source_path == b.source_path;
}

inline bool operator==(const step_argument& a, const step_argument& b) {
    return a.name == b.name && a.value == b.value;
}

inline bool operator==(const upgrader_step& a, const upgrader_step& b) {
    return a.op_id == b.op_id && a.target_path == b.target_path && a.arguments == b.arguments;
}

} // namespace muesli::upgrade

int main() {
    // =========================================================================
    // 1. argument_value round-trip (all literal types)
    // =========================================================================
    {
        round_trip(up::argument_value_codec, up::argument_value{up::literal{std::monostate{}}});
        round_trip(up::argument_value_codec, up::argument_value{up::literal{true}});
        round_trip(up::argument_value_codec, up::argument_value{up::literal{false}});
        round_trip(up::argument_value_codec, up::argument_value{up::literal{std::int64_t{-42}}});
        round_trip(up::argument_value_codec, up::argument_value{up::literal{std::uint64_t{999}}});
        round_trip(up::argument_value_codec, up::argument_value{up::literal{3.14}});
        round_trip(up::argument_value_codec, up::argument_value{up::literal{std::string{"hello world"}}});
        round_trip(up::argument_value_codec, up::argument_value{up::literal{std::string{}}});
    }

    // =========================================================================
    // 2. argument_value round-trip (field references)
    // =========================================================================
    {
        round_trip(up::argument_value_codec, up::argument_value{up::field_reference{{std::size_t{0}}}});
        round_trip(up::argument_value_codec, up::argument_value{up::field_reference{{std::string{"user"}, std::size_t{0}, std::string{"email"}}}});
        round_trip(up::argument_value_codec, up::argument_value{up::field_reference{{}}});
    }

    // =========================================================================
    // 3. path_token_vector round-trip
    // =========================================================================
    {
        std::vector<up::path_token> empty{};
        round_trip(up::path_token_vector_codec, empty);

        std::vector<up::path_token> string_only{std::string{"field_name"}};
        round_trip(up::path_token_vector_codec, string_only);

        std::vector<up::path_token> index_only{std::size_t{42}};
        round_trip(up::path_token_vector_codec, index_only);

        std::vector<up::path_token> mixed{std::string{"user"}, std::size_t{0}, std::string{"email"}};
        round_trip(up::path_token_vector_codec, mixed);
    }

    // =========================================================================
    // 4. step_argument round-trip (via argument value codec)
    // =========================================================================
    {
        // Literal int
        up::argument_value arg_lit{up::literal{std::int64_t{7}}};
        round_trip(up::argument_value_codec, arg_lit);

        // Literal string
        up::argument_value arg_str{up::literal{std::string{"{1}@example.com"}}};
        round_trip(up::argument_value_codec, arg_str);

        // Field reference
        up::argument_value arg_ref{up::field_reference{{std::size_t{0}}}};
        round_trip(up::argument_value_codec, arg_ref);
    }

    // =========================================================================
    // 6. upgrader_step round-trip
    // =========================================================================
    {
        // Simple set step
        up::upgrader_step step_set{
            "set",
            {std::size_t{0}},
            {up::step_argument{"value", up::literal{std::int64_t{2}}}}
        };
        round_trip(up::upgrader_step_codec, step_set);

        // Format step with string template
        up::upgrader_step step_fmt{
            "format",
            {std::size_t{2}},
            {up::step_argument{"template", up::literal{std::string{"{1}@example.com"}}}}
        };
        round_trip(up::upgrader_step_codec, step_fmt);

        // Copy step with field reference
        up::upgrader_step step_copy{
            "copy",
            {std::size_t{2}},
            {up::step_argument{"source", up::field_reference{{std::size_t{1}}}}}
        };
        round_trip(up::upgrader_step_codec, step_copy);

        // Step with multiple arguments
        up::upgrader_step step_multi{
            "complex_op",
            {std::string{"nested"}, std::size_t{3}},
            {
                up::step_argument{"template", up::literal{std::string{"user-{0}"}}},
                up::step_argument{"fallback", up::literal{std::string{"unknown"}}},
                up::step_argument{"source", up::field_reference{{std::size_t{1}}}}
            }
        };
        round_trip(up::upgrader_step_codec, step_multi);

        // Empty step (no args, no target_path)
        up::upgrader_step step_empty{"noop", {}, {}};
        round_trip(up::upgrader_step_codec, step_empty);
    }

    // =========================================================================
    // 7. upgrader_document<int> round-trip
    // =========================================================================
    {
        constexpr auto doc_codec = up::make_upgrader_document_codec(mu::int_codec);

        up::upgrader_document<int> doc{
            1,
            2,
            {
                up::upgrader_step{"set", {std::size_t{0}}, {up::step_argument{"value", up::literal{std::int64_t{2}}}}},
                up::upgrader_step{"format", {std::size_t{2}}, {up::step_argument{"template", up::literal{std::string{"{1}@mail.com"}}}}}
            },
            "v1_to_v2",
            "Bump version and add email"
        };

        auto bytes = to_bytes(doc_codec, doc);
        auto decoded = from_bytes(doc_codec, bytes);
        assert(decoded.has_value());
        assert(decoded->from_version == 1);
        assert(decoded->to_version == 2);
        assert(decoded->steps.size() == 2);
        assert(decoded->steps[0].op_id == "set");
        assert(decoded->steps[1].op_id == "format");
        assert(decoded->name == "v1_to_v2");
        assert(decoded->description == "Bump version and add email");

        // Verify step arguments round-tripped correctly
        const auto& set_step = decoded->steps[0];
        assert(set_step.target_path.size() == 1);
        assert(std::get<std::size_t>(set_step.target_path[0]) == 0);
        assert(set_step.arguments.size() == 1);
        assert(set_step.arguments[0].name == "value");
        const auto* lit = std::get_if<up::literal>(&set_step.arguments[0].value);
        assert(lit != nullptr);
        assert(std::get<std::int64_t>(*lit) == 2);
    }

    // =========================================================================
    // 8. upgrader_document with string version
    // =========================================================================
    {
        constexpr auto doc_codec = up::make_upgrader_document_codec(mu::string_codec);

        up::upgrader_document<std::string> doc{
            "alpha",
            "beta",
            {
                up::upgrader_step{"set", {std::string{"version"}}, {up::step_argument{"value", up::literal{std::string{"beta"}}}}}
            },
            "alpha_to_beta",
            "String version migration"
        };

        auto bytes = to_bytes(doc_codec, doc);
        auto decoded = from_bytes(doc_codec, bytes);
        assert(decoded.has_value());
        assert(decoded->from_version == "alpha");
        assert(decoded->to_version == "beta");
        assert(decoded->steps.size() == 1);
        assert(decoded->name == "alpha_to_beta");
    }

    // =========================================================================
    // 9. Multiple documents sequentially in one stream
    // =========================================================================
    {
        constexpr auto doc_codec = up::make_upgrader_document_codec(mu::int_codec);

        up::upgrader_document<int> doc1{1, 2, {}, "v1_v2", "First"};
        up::upgrader_document<int> doc2{2, 3, {up::upgrader_step{"set", {std::size_t{0}}, {up::step_argument{"value", up::literal{std::int64_t{3}}}}}}, "v2_v3", "Second"};
        up::upgrader_document<int> doc3{3, 4, {}, "v3_v4", "Third"};

        auto fmt = mu::make_binary_format<char>(doc_codec);

        std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
        assert(fmt.serialize(doc1, ss));
        assert(fmt.serialize(doc2, ss));
        assert(fmt.serialize(doc3, ss));

        ss.seekg(0);
        auto d1 = fmt.deserialize(ss);
        auto d2 = fmt.deserialize(ss);
        auto d3 = fmt.deserialize(ss);

        assert(d1.has_value() && d1->from_version == 1 && d1->to_version == 2);
        assert(d2.has_value() && d2->from_version == 2 && d2->to_version == 3);
        assert(d3.has_value() && d3->from_version == 3 && d3->to_version == 4);

        assert(d1->name == "v1_v2");
        assert(d2->name == "v2_v3");
        assert(d2->steps.size() == 1);
        assert(d3->name == "v3_v4");
    }

    // =========================================================================
    // 10. Document with UDL-constructed arguments (verify UDL args serialize)
    // =========================================================================
    {
        constexpr auto doc_codec = up::make_upgrader_document_codec(mu::int_codec);

        up::upgrader_document<int> doc{
            1,
            2,
            {
                up::upgrader_step{"set", {std::size_t{0}}, {"value"_arg(42)}},
                up::upgrader_step{"format", {std::size_t{2}}, {"template"_arg("{1}@example.com")}},
                up::upgrader_step{"copy", {std::size_t{3}}, {"source"_arg(idx(1))}},
            },
            "udl_doc",
            "Built with UDL syntax"
        };

        auto bytes = to_bytes(doc_codec, doc);
        auto decoded = from_bytes(doc_codec, bytes);
        assert(decoded.has_value());
        assert(decoded->steps.size() == 3);

        // Verify set step
        const auto& s0 = decoded->steps[0];
        assert(s0.op_id == "set");
        assert(s0.arguments.size() == 1);
        assert(s0.arguments[0].name == "value");
        const auto* lit0 = std::get_if<up::literal>(&s0.arguments[0].value);
        assert(lit0 && std::get<std::int64_t>(*lit0) == 42);

        // Verify format step
        const auto& s1 = decoded->steps[1];
        assert(s1.op_id == "format");
        const auto* lit1 = std::get_if<up::literal>(&s1.arguments[0].value);
        assert(lit1 && std::get<std::string>(*lit1) == "{1}@example.com");

        // Verify copy step (field_reference)
        const auto& s2 = decoded->steps[2];
        assert(s2.op_id == "copy");
        const auto* ref = std::get_if<up::field_reference>(&s2.arguments[0].value);
        assert(ref != nullptr);
        assert(ref->source_path.size() == 1);
        assert(std::get<std::size_t>(ref->source_path[0]) == 1);
    }

    // =========================================================================
    // 11. Edge case: deeply nested field_reference path
    // =========================================================================
    {
        up::argument_value deep_ref{up::field_reference{{
            std::string{"root"},
            std::size_t{0},
            std::string{"child"},
            std::size_t{3},
            std::string{"leaf"}
        }}};
        round_trip(up::argument_value_codec, deep_ref);
    }

    // =========================================================================
    // 12. Edge case: step with all literal types in arguments
    // =========================================================================
    {
        up::upgrader_step step{
            "all_types",
            {std::size_t{0}},
            {
                up::step_argument{"mono", up::literal{std::monostate{}}},
                up::step_argument{"flag", up::literal{true}},
                up::step_argument{"signed", up::literal{std::int64_t{-100}}},
                up::step_argument{"unsigned", up::literal{std::uint64_t{200}}},
                up::step_argument{"floating", up::literal{2.718}},
                up::step_argument{"text", up::literal{std::string{"all types covered"}}},
            }
        };
        round_trip(up::upgrader_step_codec, step);
    }

    // =========================================================================
    // 13. Truncated stream detection
    // =========================================================================
    {
        up::upgrader_step step{"set", {std::size_t{0}}, {"value"_arg(42)}};
        auto bytes = to_bytes(up::upgrader_step_codec, step);

        // Truncate at various points and verify graceful failure
        for (std::size_t len = 0; len < bytes.size(); ++len) {
            auto truncated = bytes.substr(0, len);
            auto result = from_bytes(up::upgrader_step_codec, truncated);
            assert(!result.has_value());
        }
    }

    return 0;
}
