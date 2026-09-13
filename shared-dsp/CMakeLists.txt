add_library(aifred_shared_dsp STATIC src/Engine.cpp src/Loudness.cpp src/Spectrum.cpp src/BufferHunter.cpp src/Filter.cpp)
target_include_directories(aifred_shared_dsp PUBLIC include)
target_compile_features(aifred_shared_dsp PUBLIC cxx_std_20)
set_target_properties(aifred_shared_dsp PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
  target_compile_options(aifred_shared_dsp PRIVATE /W4 /permissive-)
else()
  target_compile_options(aifred_shared_dsp PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_executable(aifred_core_tests tests/core_tests.cpp)
target_link_libraries(aifred_core_tests PRIVATE aifred_shared_dsp)
add_test(NAME aifred_core_tests COMMAND aifred_core_tests)


add_library(aifred_pipeline STATIC src/Pipeline.cpp src/IntelligenceClient.cpp)
target_link_libraries(aifred_pipeline PUBLIC aifred_shared_dsp juce::juce_core)
target_compile_definitions(aifred_pipeline PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)


add_executable(aifred_fixture_meter tests/fixture_meter.cpp)
target_link_libraries(aifred_fixture_meter PRIVATE aifred_shared_dsp)
