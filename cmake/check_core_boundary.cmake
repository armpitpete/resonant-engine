if(NOT DEFINED CORE_DIR)
    message(FATAL_ERROR "CORE_DIR is required")
endif()

file(GLOB_RECURSE CORE_FILES
    "${CORE_DIR}/*.h" "${CORE_DIR}/*.hpp" "${CORE_DIR}/*.c" "${CORE_DIR}/*.cc" "${CORE_DIR}/*.cpp")

set(FORBIDDEN_TOKENS
    "juce"
    "vst"
    "emscripten"
    "windows.h"
    "unistd.h"
    "CoreAudio"
    "AudioToolbox"
    "WebAudio"
    "midiusb"
    "usb.h"
    "std::random_device"
    "std::cout"
    "std::cerr"
    "printf("
    "fprintf("
    "fopen("
    "socket("
)

foreach(FILE_PATH IN LISTS CORE_FILES)
    file(READ "${FILE_PATH}" CONTENT)
    foreach(TOKEN IN LISTS FORBIDDEN_TOKENS)
        string(FIND "${CONTENT}" "${TOKEN}" TOKEN_INDEX)
        if(NOT TOKEN_INDEX EQUAL -1)
            message(FATAL_ERROR "Portable core dependency/RT boundary violation in ${FILE_PATH}: ${TOKEN}")
        endif()
    endforeach()
endforeach()

message(STATUS "resonant_core dependency/RT boundary check passed (${CORE_DIR})")
