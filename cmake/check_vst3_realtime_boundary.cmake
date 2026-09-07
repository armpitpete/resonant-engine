if(NOT DEFINED VST3_DIR)
    message(FATAL_ERROR "VST3_DIR is required")
endif()

# Supplementary source guard only. Runtime allocation tests remain the primary
# proof for Processor::process(); this catches obvious future regressions that
# would introduce blocking primitives or heap-backed containers in the wrapper.
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
    "std::atomic_wait"
    ".wait("
    "std::counting_semaphore"
    "std::binary_semaphore"
    "std::latch"
    "std::barrier"
    "std::this_thread::sleep"
    "std::this_thread::yield"
    "std::thread"
    "std::jthread"
    "std::future"
    "std::async"
    "std::vector"
    "std::deque"
    "std::list"
    "std::map"
    "std::unordered_map"
    "std::unordered_set"
    "std::function"
    "std::basic_string"
    "std::make_unique"
    "std::make_shared"
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
    "PASS: supplementary M4.8 wrapper source guard found no obvious blocking/heap-container APIs")
