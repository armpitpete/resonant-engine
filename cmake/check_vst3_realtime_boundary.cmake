if(NOT DEFINED VST3_DIR)
    message(FATAL_ERROR "VST3_DIR is required")
endif()

set(M4_REALTIME_FILES
    "${VST3_DIR}/Processor.cpp"
    "${VST3_DIR}/CoreAdapter.hpp"
    "${VST3_DIR}/EventTranslator.hpp"
)

set(M4_FORBIDDEN_REALTIME_TOKENS
    "std::mutex"
    "std::recursive_mutex"
    "std::timed_mutex"
    "std::lock_guard"
    "std::unique_lock"
    "std::scoped_lock"
    "std::condition_variable"
    "std::this_thread::sleep"
    "std::this_thread::yield"
    "std::future"
    "std::async"
    "std::vector"
    "std::deque"
    "std::list"
    "std::map"
    "std::unordered_map"
    "std::unordered_set"
    "std::function"
    "malloc("
    "calloc("
    "realloc("
)

foreach(path IN LISTS M4_REALTIME_FILES)
    if(NOT EXISTS "${path}")
        message(FATAL_ERROR "Missing M4 realtime source: ${path}")
    endif()

    file(READ "${path}" content)
    foreach(token IN LISTS M4_FORBIDDEN_REALTIME_TOKENS)
        string(FIND "${content}" "${token}" hit)
        if(NOT hit EQUAL -1)
            message(FATAL_ERROR
                "Forbidden realtime construct '${token}' found in ${path}")
        endif()
    endforeach()
endforeach()

message(STATUS
    "PASS: M4.8 VST3 realtime sources contain no blocking/heap-container APIs")
