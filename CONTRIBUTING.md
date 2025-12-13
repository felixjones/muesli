# Contributing to Muesli

## Table of Contents
- Code of Conduct
- Getting Started
- Development Setup
- Coding Standards
- Testing
- Documentation
- Pull Requests
- Branch Strategy
- Questions

## Code of Conduct
We will be adopting a code of conduct in the near future. Meanwhile, follow standard open source etiquette; be respectful, constructive, and collaborative.

## Getting Started
1) Fork and clone the repo.
2) Create a branch: `git checkout -b feature/your-feature`.
3) Build and test (see below).

## Development Setup
- C++20 compiler: GCC 13+, Clang 17+, MSVC 2022+, or MinGW GCC 13+.
- CMake 3.20+.
- Python 3.8+ for tooling; Doxygen optional for local docs.
- Configure and build:
  ```bash
  cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
  cmake --build build
  ```
- Run tests:
  ```bash
  cd build
  ctest --output-on-failure
  ```

## Coding Standards
- Format with clang-format; lint with clang-tidy (configs in repo).
- Style: 4-space indent, 120-col limit, attach braces, no space in `template<>`, requires on same line.
- Naming: locals/params `camelCase`; globals/public/const `snake_case`; protected/private `m_prefixedCamelCase`; types/namespaces/enums `snake_case`; prefer `struct` over `class`.
- Use `constexpr`/`noexcept` where appropriate; avoid raw ownership; prefer RAII.

## Testing
- Add tests for new behavior and failure cases.
- Place in `tests/`, name `test_<topic>.cpp`, register in `CMakeLists.txt`.
- Keep tests deterministic; avoid timing dependencies.

## Documentation
- Public APIs need Doxygen: brief, params, returns, template params, and a short example.
- Cross-link with `@see` where relevant.
- Preview wiki docs locally:
  ```bash
  python tools/wiki_gen.py
  # Output: temp_wiki/
  ```

## Pull Requests
- Checklist:
  - Tests pass locally (`ctest`).
  - clang-format/clang-tidy clean.
  - License headers on new files.
  - No new warnings.
- Open a PR against `main`; concise description; reference issues.

## Branch Strategy
- `main`: stable. Use feature/fix branches (`feature/<name>`, `fix/<name>`).
