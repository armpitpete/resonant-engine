cmake_minimum_required(VERSION 3.20)

project(resonant_engine VERSION 0.0.1 LANGUAGES CXX)

option(RESONANT_ENGINE_BUILD_TESTS "Build Resonant Engine tests" ON)

add_library(resonant_engine INTERFACE)
add_library(ResonantEngine::resonant_engine ALIAS resonant_engine)

target_include_directories(resonant_engine INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(resonant_engine INTERFACE cxx_std_17)

if(RESONANT_ENGINE_BUILD_TESTS)
    enable_testing()
    add_executable(resonant_engine_smoke tests/smoke.cpp)
    target_link_libraries(resonant_engine_smoke PRIVATE ResonantEngine::resonant_engine)
    add_test(NAME resonant_engine_smoke COMMAND resonant_engine_smoke)
endif()
