cmake_minimum_required(VERSION 3.20)

project(resonant_engine VERSION 0.0.1 LANGUAGES CXX)

option(RESONANT_ENGINE_BUILD_TESTS "Build Resonant Engine tests" ON)
option(RESONANT_ENGINE_BUILD_RENDER "Build offline render host" ON)
option(RESONANT_ENGINE_WARNINGS_AS_ERRORS "Treat project warnings as errors" OFF)
option(RESONANT_ENGINE_ENABLE_SANITIZERS "Enable ASan/UBSan on supported compilers" OFF)
option(RESONANT_ENGINE_BUILD_VST3 "Build the M4 VST3 reference host" OFF)

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

if(RESONANT_ENGINE_BUILD_VST3)
    if(CMAKE_VERSION VERSION_LESS 3.25)
        message(FATAL_ERROR "M4 VST3 host requires CMake 3.25 or newer")
    endif()
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/vst3_sdk.cmake)
    add_subdirectory(hosts/vst3)
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

    add_executable(resonant_breath_pipe_tests tests/unit/test_breath_pipe_core.cpp)
    target_link_libraries(resonant_breath_pipe_tests PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_breath_pipe_tests)
    resonant_apply_sanitizers(resonant_breath_pipe_tests)
    add_test(NAME resonant_breath_pipe_tests COMMAND resonant_breath_pipe_tests)

    add_executable(resonant_breath_pipe_contract_tests tests/unit/test_breath_pipe_contract.cpp)
    target_link_libraries(resonant_breath_pipe_contract_tests PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_breath_pipe_contract_tests)
    resonant_apply_sanitizers(resonant_breath_pipe_contract_tests)
    add_test(NAME resonant_breath_pipe_contract_tests COMMAND resonant_breath_pipe_contract_tests)

    add_executable(resonant_breath_pipe_allocation_tests tests/unit/test_breath_pipe_allocation.cpp)
    target_link_libraries(resonant_breath_pipe_allocation_tests PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_breath_pipe_allocation_tests)
    resonant_apply_sanitizers(resonant_breath_pipe_allocation_tests)
    add_test(NAME resonant_breath_pipe_allocation_tests COMMAND resonant_breath_pipe_allocation_tests)

    add_executable(resonant_m3_topology_tests tests/unit/test_m3_topology.cpp)
    target_link_libraries(resonant_m3_topology_tests PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_m3_topology_tests)
    resonant_apply_sanitizers(resonant_m3_topology_tests)
    add_test(NAME resonant_m3_topology_tests COMMAND resonant_m3_topology_tests)

    add_executable(resonant_lab_tests tests/unit/test_lab.cpp)
    target_include_directories(resonant_lab_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lab/include)
    target_link_libraries(resonant_lab_tests PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_lab_tests)
    resonant_apply_sanitizers(resonant_lab_tests)
    add_test(NAME resonant_lab_tests COMMAND resonant_lab_tests)

    add_executable(resonant_vst3_adapter_tests tests/unit/test_vst3_core_adapter.cpp)
    target_include_directories(resonant_vst3_adapter_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(resonant_vst3_adapter_tests PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_vst3_adapter_tests)
    resonant_apply_sanitizers(resonant_vst3_adapter_tests)
    add_test(NAME resonant_vst3_adapter_tests COMMAND resonant_vst3_adapter_tests)

    add_executable(resonant_vst3_event_translator_tests tests/unit/test_vst3_event_translator.cpp)
    target_include_directories(resonant_vst3_event_translator_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(resonant_vst3_event_translator_tests PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_vst3_event_translator_tests)
    resonant_apply_sanitizers(resonant_vst3_event_translator_tests)
    add_test(NAME resonant_vst3_event_translator_tests COMMAND resonant_vst3_event_translator_tests)

    add_executable(resonant_vst3_parameter_mapping_tests tests/unit/test_vst3_parameter_mapping.cpp)
    target_include_directories(resonant_vst3_parameter_mapping_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(resonant_vst3_parameter_mapping_tests PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_vst3_parameter_mapping_tests)
    resonant_apply_sanitizers(resonant_vst3_parameter_mapping_tests)
    add_test(NAME resonant_vst3_parameter_mapping_tests COMMAND resonant_vst3_parameter_mapping_tests)

    if(RESONANT_ENGINE_BUILD_VST3)
        add_executable(
            resonant_vst3_processor_flush_tests
            tests/unit/test_vst3_processor_flush.cpp
            hosts/vst3/Processor.cpp
        )
        target_include_directories(
            resonant_vst3_processor_flush_tests
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}
        )
        target_include_directories(
            resonant_vst3_processor_flush_tests
            SYSTEM BEFORE PRIVATE
                ${vst3sdk_SOURCE_DIR}
        )
        target_link_libraries(
            resonant_vst3_processor_flush_tests
            PRIVATE
                ResonantEngine::core
                sdk
        )
        target_compile_features(
            resonant_vst3_processor_flush_tests PRIVATE cxx_std_20)
        resonant_apply_warnings(resonant_vst3_processor_flush_tests)
        add_test(
            NAME resonant_vst3_processor_flush_tests
            COMMAND resonant_vst3_processor_flush_tests
        )
    endif()

    add_executable(resonant_breath_pipe_lab_tests tests/unit/test_breath_pipe_lab.cpp)
    target_include_directories(resonant_breath_pipe_lab_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lab/include)
    target_link_libraries(resonant_breath_pipe_lab_tests PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_breath_pipe_lab_tests)
    resonant_apply_sanitizers(resonant_breath_pipe_lab_tests)
    add_test(NAME resonant_breath_pipe_lab_tests COMMAND resonant_breath_pipe_lab_tests)

    add_executable(resonant_lab_signature tools/lab_signature/main.cpp)
    target_include_directories(resonant_lab_signature PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lab/include)
    target_link_libraries(resonant_lab_signature PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_lab_signature)
    resonant_apply_sanitizers(resonant_lab_signature)

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

    add_executable(resonant_breath_pipe_property_tests tests/property/breath_pipe_property.cpp)
    target_link_libraries(resonant_breath_pipe_property_tests PRIVATE ResonantEngine::core)
    resonant_apply_warnings(resonant_breath_pipe_property_tests)
    resonant_apply_sanitizers(resonant_breath_pipe_property_tests)
    add_test(NAME resonant_breath_pipe_property_tests COMMAND resonant_breath_pipe_property_tests)

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
        NAME resonant_vst3_dependency_boundary
        COMMAND ${CMAKE_COMMAND}
            -DCORE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/core
            -DVST3_DIR=${CMAKE_CURRENT_SOURCE_DIR}/hosts/vst3
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_vst3_boundary.cmake
    )

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
