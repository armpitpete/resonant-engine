if(NOT DEFINED CORE_DIR)
    message(FATAL_ERROR "CORE_DIR is required")
endif()

file(GLOB_RECURSE CORE_FILES
    "${CORE_DIR}/*.h" "${CORE_DIR}/*.hpp" "${CORE_DIR}/*.c" "${CORE_DIR}/*.cc" "${CORE_DIR}/*.cpp")

set(FORBIDDEN_INCLUDE_REGEX "#[ \\t]*include[ \\t]*[<\\\"][^>\\\"]*(juce|vst|emscripten|windows\\\\.h|unistd\\\\.h|CoreAudio|AudioToolbox|WebAudio|midiusb|usb\\\\.h)[^>\\\"]*[>\\\"]")
set(FORBIDDEN_TEXT_REGEX "(std::random_device|printf[ \\t]*\\\\(|fprintf[ \\t]*\\\\(|std::cout|std::cerr|fopen[ \\t]*\\\\(|socket[ \\t]*\\\\()")

foreach(FILE_PATH IN LISTS CORE_FILES)
    file(READ "${FILE_PATH}" CONTENT)
    if(CONTENT MATCHES "${FORBIDDEN_INCLUDE_REGEX}")
        message(FATAL_ERROR "Portable core dependency boundary violation in ${FILE_PATH}: ${CMAKE_MATCH_0}")
    endif()
    if(CONTENT MATCHES "${FORBIDDEN_TEXT_REGEX}")
        message(FATAL_ERROR "Portable core real-time boundary violation in ${FILE_PATH}: ${CMAKE_MATCH_0}")
    endif()
endforeach()

message(STATUS "resonant_core dependency/RT boundary check passed (${CORE_DIR})")
