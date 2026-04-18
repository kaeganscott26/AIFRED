include(FetchContent)

set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.10
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Aifred
  COMPANY_NAME "North3rnLight3r"
  COMPANY_WEBSITE "https://www.north3rnlight3r.com"
  COMPANY_EMAIL "north3rnlight3rofficial@outlook.com"
  BUNDLE_ID "com.aifred.plugin"
  PLUGIN_MANUFACTURER_CODE N3Lr
  PLUGIN_CODE Aifr
  FORMATS VST3
  PRODUCT_NAME "Aifred"
  DESCRIPTION "Reference-aware mix alignment and Halo diagnostics"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  VST3_CATEGORIES "Analyzer"
)

juce_add_binary_data(AifredAssets
  SOURCES
    Assets/aifred-mascot.jpg
)

target_sources(Aifred PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp
  Source/HaloState.cpp
  Source/AnalysisEngine.cpp
  Source/DiagnosticInterpreter.cpp
  Source/AifredEngineClient.cpp
  Source/AifredLookAndFeel.cpp
)

target_include_directories(Aifred PRIVATE Source)

target_compile_definitions(Aifred
  PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    AIFRED_VERSION_STRING="${PROJECT_VERSION}"
)

target_link_libraries(Aifred
  PRIVATE
    AifredAssets
    juce::juce_audio_utils
    $<$<PLATFORM_ID:Windows>:winhttp>
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
