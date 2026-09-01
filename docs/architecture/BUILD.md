# M0.14 — Build System & Toolchain

Status: **APPROVED**

C++20 and CMake 3.20+ are the baseline. `resonant_core` is a static library with public headers under `core/include/resonant` and private implementation sources under `core/src`.

## Targets

- `resonant_core` / `ResonantEngine::core` — portable core;
- `resonant_tests` — zero-dependency unit/RT contract suite;
- `resonant_property_tests` — bounded property checks;
- `resonant_regression_tests` — deterministic render-signature regression;
- `resonant_render` — offline host/WAV renderer.

## Warnings and analysis

MSVC uses `/W4 /permissive-`; GCC/Clang use `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`. Warnings-as-errors is optional locally and **enabled in CI**. `.clang-format`, `.clang-tidy` and `.editorconfig` establish formatting/static-analysis baselines. clang-tidy is advisory in M0; it is not a required CI dependency.

ASan + UBSan are available via `RESONANT_ENGINE_ENABLE_SANITIZERS=ON` on GCC/Clang. ThreadSanitizer is intentionally not a baseline M0 job: the demonstrated core has no background-thread support code and TSan does not prove hard real-time safety. It should be added when non-real-time concurrency enters the project.

## Compiler matrix

CI proves:

- Windows latest / MSVC;
- Ubuntu latest / GCC;
- macOS latest / Apple Clang;
- Debug and Release for all three;
- Ubuntu ASan+UBSan;
- a Linux `-fno-exceptions -fno-rtti` compile probe for the core translation unit.

## Local commands

```sh
cmake -S . -B build -DRESONANT_ENGINE_BUILD_TESTS=ON -DRESONANT_ENGINE_BUILD_RENDER=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Sanitizers:

```sh
cmake -S . -B build-sanitize -DRESONANT_ENGINE_ENABLE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-sanitize
ctest --test-dir build-sanitize --output-on-failure
```

Dependencies remain standard-library/CMake only; Python is optional and used only to inspect WAV structure in tests when available.

A clean source checkout builds out-of-tree; `.gitignore` excludes generated build products.

**M0.14 Build System & Toolchain: APPROVED.**
