add_library(aifred_engine STATIC src/Engine.cpp src/Loudness.cpp src/Spectrum.cpp src/BufferHunter.cpp src/Filter.cpp)
target_include_directories(aifred_engine PUBLIC include)
target_compile_features(aifred_engine PUBLIC cxx_std_20)
set_target_properties(aifred_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
  target_compile_options(aifred_engine PRIVATE /W4 /permissive-)
else()
  target_compile_options(aifred_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_executable(aifred_core_tests tests/core_tests.cpp)
target_link_libraries(aifred_core_tests PRIVATE aifred_engine)
add_test(NAME aifred_core_tests COMMAND aifred_core_tests)


add_library(aifred_pipeline STATIC src/Pipeline.cpp src/IntelligenceClient.cpp)
target_link_libraries(aifred_pipeline PUBLIC aifred_engine juce::juce_core)
target_compile_definitions(aifred_pipeline PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)

