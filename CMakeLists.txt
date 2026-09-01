cmake_minimum_required(VERSION 3.20)

project(resonant_engine VERSION 0.0.1 LANGUAGES CXX)

option(RESONANT_ENGINE_BUILD_TESTS "Build Resonant Engine tests" ON)
option(RESONANT_ENGINE_BUILD_RENDER "Build offline render host" ON)
option(RESONANT_ENGINE_WARNINGS_AS_ERRORS "Treat project warnings as errors" OFF)
option(RESONANT_ENGINE_ENABLE_SANITIZERS "Enable ASan/UBSan on supported compilers" OFF)

add_library(resonant_core STATIC core/src/Core.cpp)
add_library(ResonantEngine::core ALIAS resonant_core)

target_include_directories(resonant_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(resonant_core PUBLIC cxx_std_20)

function(resonant_apply_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
        if(RESONANT_ENGINE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        if(RESONANT_ENGINE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

function(resonant_apply_sanitizers target)
    if(RESONANT_ENGINE_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
endfunction()

resonant_apply_warnings(resonant_core)
resonant_apply_sanitizers(resonant_core)

if(RESONANT_ENGINE_BUILD_RENDER)
    add_executable(resonant_render tools/render/main.cpp)
    target_link_libraries(resonant_render PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_render)
    resonant_apply_sanitizers(resonant_render)

    add_executable(resonant_m1_render tools/m1_render/main.cpp)
    target_link_libraries(resonant_m1_render PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_m1_render)
    resonant_apply_sanitizers(resonant_m1_render)
endif()

if(RESONANT_ENGINE_BUILD_TESTS)
    include(CTest)
    enable_testing()

    add_executable(resonant_tests tests/unit/test_main.cpp)
    target_link_libraries(resonant_tests PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_tests)
    resonant_apply_sanitizers(resonant_tests)
    add_test(NAME resonant_tests COMMAND resonant_tests)

    add_executable(resonant_failure_tests tests/unit/test_failure.cpp)
    target_link_libraries(resonant_failure_tests PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_failure_tests)
    resonant_apply_sanitizers(resonant_failure_tests)
    add_test(NAME resonant_failure_tests COMMAND resonant_failure_tests)

    add_executable(resonant_stability_tests tests/unit/test_stability.cpp)
    target_link_libraries(resonant_stability_tests PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_stability_tests)
    resonant_apply_sanitizers(resonant_stability_tests)
    add_test(NAME resonant_stability_tests COMMAND resonant_stability_tests)

    add_executable(resonant_first_resonator_tests tests/unit/test_first_resonator.cpp)
    target_link_libraries(resonant_first_resonator_tests PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_first_resonator_tests)
    resonant_apply_sanitizers(resonant_first_resonator_tests)
    add_test(NAME resonant_first_resonator_tests COMMAND resonant_first_resonator_tests)

    add_executable(resonant_first_resonator_allocation_tests tests/unit/test_first_resonator_allocation.cpp)
    target_link_libraries(resonant_first_resonator_allocation_tests PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_first_resonator_allocation_tests)
    resonant_apply_sanitizers(resonant_first_resonator_allocation_tests)
    add_test(NAME resonant_first_resonator_allocation_tests COMMAND resonant_first_resonator_allocation_tests)

    add_executable(resonant_property_tests tests/property/property_contract.cpp)
    target_link_libraries(resonant_property_tests PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_property_tests)
    resonant_apply_sanitizers(resonant_property_tests)
    add_test(NAME resonant_property_tests COMMAND resonant_property_tests)

    add_executable(resonant_first_resonator_property_tests tests/property/first_resonator_property.cpp)
    target_link_libraries(resonant_first_resonator_property_tests PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_first_resonator_property_tests)
    resonant_apply_sanitizers(resonant_first_resonator_property_tests)
    add_test(NAME resonant_first_resonator_property_tests COMMAND resonant_first_resonator_property_tests)

    add_executable(resonant_regression_tests tests/regression/render_regression.cpp)
    target_link_libraries(resonant_regression_tests PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_regression_tests)
    resonant_apply_sanitizers(resonant_regression_tests)
    add_test(NAME resonant_regression_tests COMMAND resonant_regression_tests)

    add_executable(resonant_first_resonator_regression_tests tests/regression/first_resonator_regression.cpp)
    target_link_libraries(resonant_first_resonator_regression_tests PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_first_resonator_regression_tests)
    resonant_apply_sanitizers(resonant_first_resonator_regression_tests)
    add_test(NAME resonant_first_resonator_regression_tests COMMAND resonant_first_resonator_regression_tests)

    add_test(
        NAME resonant_core_dependency_boundary
        COMMAND ${CMAKE_COMMAND}
            -DCORE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/core
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_core_boundary.cmake
    )

    if(RESONANT_ENGINE_BUILD_RENDER)
        set(M0_RENDER_WAV "${CMAKE_CURRENT_BINARY_DIR}/m0-canonical.wav")
        add_test(
            NAME resonant_render_smoke
            COMMAND resonant_render --sample-rate 48000 --block-size 64 --duration 0.1 --seed 777 --output ${M0_RENDER_WAV}
        )
        find_package(Python3 COMPONENTS Interpreter QUIET)
        if(Python3_Interpreter_FOUND)
            add_test(
                NAME resonant_render_wav_structure
                COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/regression/verify_wav.py ${M0_RENDER_WAV}
            )
            set_tests_properties(resonant_render_wav_structure PROPERTIES DEPENDS resonant_render_smoke)

            set(M0_SILENT_WAV "${CMAKE_CURRENT_BINARY_DIR}/m0-silent.wav")
            add_test(
                NAME resonant_render_silent_smoke
                COMMAND resonant_render --silent --sample-rate 48000 --block-size 64 --duration 0.1 --seed 777 --output ${M0_SILENT_WAV}
            )
            add_test(
                NAME resonant_render_silent_wav_structure
                COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/regression/verify_wav.py ${M0_SILENT_WAV} silent
            )
            set_tests_properties(resonant_render_silent_wav_structure PROPERTIES DEPENDS resonant_render_silent_smoke)

            foreach(M1_SCENE IN ITEMS passive-pluck continuous feedback nonlinear sweep silent)
                string(REPLACE "-" "_" M1_SCENE_ID "${M1_SCENE}")
                set(M1_RENDER_WAV "${CMAKE_CURRENT_BINARY_DIR}/m1-${M1_SCENE}.wav")
                add_test(
                    NAME resonant_m1_render_${M1_SCENE_ID}
                    COMMAND resonant_m1_render --scene ${M1_SCENE} --sample-rate 48000 --block-size 64 --duration 1.0 --seed 777 --output ${M1_RENDER_WAV}
                )
                add_test(
                    NAME resonant_m1_verify_${M1_SCENE_ID}
                    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/regression/verify_m1_wav.py ${M1_RENDER_WAV} ${M1_SCENE}
                )
                set_tests_properties(resonant_m1_verify_${M1_SCENE_ID} PROPERTIES DEPENDS resonant_m1_render_${M1_SCENE_ID})
            endforeach()
        endif()
    endif()
endif()
