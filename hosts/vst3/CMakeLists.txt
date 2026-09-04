set(RESONANT_VST3_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/CoreAdapter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Controller.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PluginFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PluginIds.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Processor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Processor.hpp
)

smtg_add_vst3plugin(ResonantEngineBreathPipe ${RESONANT_VST3_SOURCES})

target_link_libraries(ResonantEngineBreathPipe
    PRIVATE
        ResonantEngine::core
        sdk
)

target_include_directories(ResonantEngineBreathPipe
    PRIVATE
        ${PROJECT_SOURCE_DIR}
)

target_compile_features(ResonantEngineBreathPipe PRIVATE cxx_std_20)
resonant_apply_warnings(ResonantEngineBreathPipe)

if(SMTG_MAC)
    smtg_target_set_bundle(ResonantEngineBreathPipe
        BUNDLE_IDENTIFIER "uk.merrinworld.resonant-engine.breath-pipe"
        COMPANY_NAME "Merrin World"
    )
endif()
