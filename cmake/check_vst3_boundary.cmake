if(NOT DEFINED CORE_DIR)
    message(FATAL_ERROR "CORE_DIR is required")
endif()
if(NOT DEFINED VST3_DIR)
    message(FATAL_ERROR "VST3_DIR is required")
endif()

file(GLOB_RECURSE CORE_FILES
    "${CORE_DIR}/*.h" "${CORE_DIR}/*.hpp" "${CORE_DIR}/*.c" "${CORE_DIR}/*.cc" "${CORE_DIR}/*.cpp")

set(VST3_CORE_FORBIDDEN
    "pluginterfaces/"
    "public.sdk/"
    "Steinberg::"
    "Vst::"
    "smtg_"
)

foreach(FILE_PATH IN LISTS CORE_FILES)
    file(READ "${FILE_PATH}" CONTENT)
    foreach(TOKEN IN LISTS VST3_CORE_FORBIDDEN)
        string(FIND "${CONTENT}" "${TOKEN}" TOKEN_INDEX)
        if(NOT TOKEN_INDEX EQUAL -1)
            message(FATAL_ERROR "M4 VST3 boundary violation in core file ${FILE_PATH}: ${TOKEN}")
        endif()
    endforeach()
endforeach()

file(GLOB_RECURSE VST3_FILES
    "${VST3_DIR}/*.h" "${VST3_DIR}/*.hpp" "${VST3_DIR}/*.c" "${VST3_DIR}/*.cc" "${VST3_DIR}/*.cpp")

set(DUPLICATED_DSP_DEFINITIONS
    "class BreathPipeExciter"
    "class BreathPipeModalResonator"
    "class BreathPipeVoice"
)

foreach(FILE_PATH IN LISTS VST3_FILES)
    file(READ "${FILE_PATH}" CONTENT)
    foreach(TOKEN IN LISTS DUPLICATED_DSP_DEFINITIONS)
        string(FIND "${CONTENT}" "${TOKEN}" TOKEN_INDEX)
        if(NOT TOKEN_INDEX EQUAL -1)
            message(FATAL_ERROR "M4 host duplicated frozen Breath Pipe DSP in ${FILE_PATH}: ${TOKEN}")
        endif()
    endforeach()
endforeach()

message(STATUS "M4 VST3/core dependency boundary check passed")
