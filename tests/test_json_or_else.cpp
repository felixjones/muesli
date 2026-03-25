/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#include <muesli/format/json_format>
#include <muesli/format/nlohmann_json_backend>
#include <muesli/codecs>

#include <cassert>

using B = muesli::nlohmann_json_backend;
namespace mu = muesli;

// -- Test structs --------------------------------------------------------

// versioned_config: or_else defaults (value overload)
struct versioned_config {
    int version;
    std::string name;
    int max_connections;

    static constexpr auto codec = mu::tuple_codec(
        mu::int_codec.member<&versioned_config::version>().named("version"),
        mu::string_codec.member<&versioned_config::name>().named("name"),
        mu::int_codec
            .or_else(100)
            .member<&versioned_config::max_connections>()
            .named("max_connections")
    ).apply<versioned_config>();
};

// config_with_defaults: multiple or_else defaults (mix of value and callable)
struct config_with_defaults {
    std::string name;
    int port;
    bool debug;
    std::string loglevel;

    static constexpr auto codec = mu::tuple_codec(
        mu::string_codec.member<&config_with_defaults::name>().named("name"),
        mu::int_codec
            .or_else(8080)
            .member<&config_with_defaults::port>()
            .named("port"),
        mu::bool_codec
            .or_else(false)
            .member<&config_with_defaults::debug>()
            .named("debug"),
        mu::string_codec
            .or_else([]{ return std::string("info"); })
            .member<&config_with_defaults::loglevel>()
            .named("loglevel")
    ).apply<config_with_defaults>();
};

int main() {
    // -- 1: Round-trip with all keys present --
    {
        auto fmt = mu::make_json_format<B>(versioned_config::codec);
        versioned_config orig{1, "app", 200};
        auto j = fmt.serialize(orig);
        auto result = fmt.deserialize(j);
        assert(result);
        assert(result->version == 1 && result->name == "app" && result->max_connections == 200);
    }

    // -- 2: JSON missing optional field uses or_else fallback --
    {
        auto fmt = mu::make_json_format<B>(versioned_config::codec);
        auto j = nlohmann::json::parse(R"({"version": 1, "name": "app"})");
        auto result = fmt.deserialize(j);
        assert(result);
        assert(result->version == 1);
        assert(result->name == "app");
        assert(result->max_connections == 100);  // Fallback used
    }

    // -- 3: JSON with optional field overrides fallback --
    {
        auto fmt = mu::make_json_format<B>(versioned_config::codec);
        auto j = nlohmann::json::parse(R"({"version": 1, "name": "app", "max_connections": 50})");
        auto result = fmt.deserialize(j);
        assert(result);
        assert(result->max_connections == 50);  // Explicit value
    }

    // -- 4: Multiple fallbacks all used when missing --
    {
        auto fmt = mu::make_json_format<B>(config_with_defaults::codec);
        auto j = nlohmann::json::parse(R"({"name": "myapp"})");
        auto result = fmt.deserialize(j);
        assert(result);
        assert(result->name == "myapp");
        assert(result->port == 8080);
        assert(result->debug == false);
        assert(result->loglevel == "info");
    }

    // -- 5: Partial fallbacks (some provided, some default) --
    {
        auto fmt = mu::make_json_format<B>(config_with_defaults::codec);
        auto j = nlohmann::json::parse(R"({"name": "myapp", "port": 9000, "loglevel": "debug"})");
        auto result = fmt.deserialize(j);
        assert(result);
        assert(result->name == "myapp");
        assert(result->port == 9000);
        assert(result->debug == false);       // Fallback
        assert(result->loglevel == "debug");  // Explicit
    }

    // -- 6: Round-trip with defaults (all values provided) --
    {
        auto fmt = mu::make_json_format<B>(config_with_defaults::codec);
        config_with_defaults orig{"server", 3000, true, "verbose"};
        auto j = fmt.serialize(orig);
        auto result = fmt.deserialize(j);
        assert(result);
        assert(result->name == "server");
        assert(result->port == 3000);
        assert(result->debug == true);
        assert(result->loglevel == "verbose");
    }

    return 0;
}
