cmake_minimum_required(VERSION 3.20)

project(resonant_engine VERSION 0.0.1 LANGUAGES CXX)

option(RESONANT_ENGINE_BUILD_TESTS "Build Resonant Engine tests" ON)

# M0 core target. Host adapters must depend on this target; the core must not
# depend on browser, DAW, plugin-SDK, GUI, device, filesystem or OS APIs.
add_library(resonant_core INTERFACE)
add_library(ResonantEngine::core ALIAS resonant_core)

target_include_directories(resonant_core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(resonant_core INTERFACE cxx_std_20)

if(RESONANT_ENGINE_BUILD_TESTS)
    enable_testing()
    add_executable(resonant_core_smoke tests/smoke.cpp)
    target_link_libraries(resonant_core_smoke PRIVATE ResonantEngine::core)
    add_test(NAME resonant_core_smoke COMMAND resonant_core_smoke)
endif()
