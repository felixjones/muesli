# Muesli - A C++20 library for serialisation

[![CI Status](https://github.com/felixjones/muesli/workflows/Test/badge.svg)](https://github.com/felixjones/muesli/actions)
[![License](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Header-only](https://img.shields.io/badge/header--only-yes-brightgreen.svg)]()

Muesli provides compile-time bi-directional serialisation, inspired by [Mojang/DataFixerUpper](https://github.com/Mojang/DataFixerUpper/tree/master/src/main/java/com/mojang/serialization) and [Haskell Codec](https://hackage.haskell.org/package/codec).

Muesli is designed to have _zero overhead_ via its compile-time first approach. In an optimised build, it will generate code equivalent to a simple and naive serialisation approach.

Muesli is extensible, allowing you to define custom codecs and formats.

You can read more about Muesli's inspiration in [this blog post](https://felixjones.co.uk/2025/03/01/serialisation.html).

## It's Easy & Fun

There's no special types required for standard containers such as `std::unordered_map` or `std::shared_ptr` as codecs treat all types equally (yes, that includes custom containers).

Building a codec is like putting pieces of a puzzle together. You can use built-in codecs for primitive types, and combine them using combinators to build more complex codecs.

```c++
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
```

### Try Now

[Open in Compiler Explorer](https://compiler-explorer.com/z/ndY4n61n8)

## Features

- Compile-time bi-directional serialisation
- Zero runtime overhead
- Extensible codecs and formats
- Built-in binary, JSON, and MessagePack formats
- Support for C++20 concepts and ranges
- Header-only library

## Installation

Muesli is a header-only library, making it easy to integrate into your project.

### CMake (Recommended)

#### Option 1: FetchContent

Add to your `CMakeLists.txt`:

```cmake
include(FetchContent)
FetchContent_Declare(
    muesli
    GIT_REPOSITORY https://github.com/felixjones/muesli.git
    GIT_TAG        v0.0.0
)
FetchContent_MakeAvailable(muesli)

target_link_libraries(your_target PRIVATE muesli)
```

#### Option 2: find_package

If you've installed Muesli:

```cmake
find_package(muesli REQUIRED)
target_link_libraries(your_target PRIVATE muesli::muesli)
```

### Manual Installation

Simply copy the `include/muesli` directory to your project and add it to your include path:

```cmake
target_include_directories(your_target PRIVATE path/to/muesli/include)
```

Or with compiler flags:
```bash
g++ -std=c++20 -I/path/to/muesli/include your_file.cpp
```

### Requirements

- C++20 compatible compiler:
  - GCC 13 or later
  - Clang 17 or later  
  - MSVC 2022 or later
  - MinGW with GCC 13+
- CMake 3.20+ (if using CMake)

## Documentation

For comprehensive documentation, guides, examples, and API reference, see the **[Muesli Wiki](https://github.com/felixjones/muesli/wiki)**.

Quick links:
- [API Reference](https://github.com/felixjones/muesli/wiki/API-Reference)

## Planned Features

- Generate schemas & documentation from codecs
- Data upgrade and migration tools
- More built-in formats (CBOR, YAML, TOML, XML)
- C++23 support for `std::expected`
- C++26 support for reflection to auto-generate codecs

## License

This project is licensed under the [BSD 3-Clause License](http://opensource.org/licenses/BSD-3-Clause). See the [LICENSE](./LICENSE) file for details.
