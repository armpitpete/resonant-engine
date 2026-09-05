set(RESONANT_VST3_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/CoreAdapter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EventTranslator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Controller.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PluginFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PluginIds.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Processor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Processor.hpp
)

smtg_add_vst3plugin(ResonantEngineBreathPipe ${RESONANT_VST3_SOURCES})

if(SMTG_MAC)
    target_sources(ResonantEngineBreathPipe
        PRIVATE
            ${vst3sdk_SOURCE_DIR}/public.sdk/source/main/macmain.cpp
    )
elseif(SMTG_WIN)
    target_sources(ResonantEngineBreathPipe
        PRIVATE
            ${vst3sdk_SOURCE_DIR}/public.sdk/source/main/dllmain.cpp
    )
elseif(SMTG_LINUX)
    target_sources(ResonantEngineBreathPipe
        PRIVATE
            ${vst3sdk_SOURCE_DIR}/public.sdk/source/main/linuxmain.cpp
    )
endif()

target_link_libraries(ResonantEngineBreathPipe
    PRIVATE
        ResonantEngine::core
        sdk
)

target_include_directories(ResonantEngineBreathPipe
    PRIVATE
        ${PROJECT_SOURCE_DIR}
)

# The Steinberg SDK is a pinned third-party dependency. Keep our host wrapper
# warnings-as-errors, but do not promote diagnostics originating in SDK headers
# (for example GCC -Wshadow in pluginterfaces/base/funknown.h) into project
# build failures.
target_include_directories(ResonantEngineBreathPipe
    SYSTEM BEFORE PRIVATE
        ${vst3sdk_SOURCE_DIR}
)

target_compile_features(ResonantEngineBreathPipe PRIVATE cxx_std_20)
resonant_apply_warnings(ResonantEngineBreathPipe)

if(SMTG_MAC)
    smtg_target_set_bundle(ResonantEngineBreathPipe
        BUNDLE_IDENTIFIER "uk.merrinworld.resonant-engine.breath-pipe"
        COMPANY_NAME "Merrin World"
    )
endif()
