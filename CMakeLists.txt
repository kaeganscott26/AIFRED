cmake_minimum_required(VERSION 3.24)

project(AIFRED VERSION 0.3.6 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(AIFRED_BUILD_PLUGIN "Build the Aifred VST3 plugin" ON)

enable_testing()

if(AIFRED_BUILD_PLUGIN)
  add_subdirectory(plugin-aifred)
endif()

add_executable(aifred_frontend_contract_tests tests/frontend_contract_tests.cpp)
target_include_directories(aifred_frontend_contract_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(aifred_frontend_contract_tests PRIVATE aifred_pipeline)
add_test(NAME aifred_frontend_contract_tests COMMAND aifred_frontend_contract_tests)

add_executable(aifred_state_contract_tests tests/state_contract_tests.cpp)
target_include_directories(aifred_state_contract_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR}/_deps/juce-src/modules)
target_compile_definitions(aifred_state_contract_tests PRIVATE JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1)
target_link_libraries(aifred_state_contract_tests PRIVATE Aifred aifred_pipeline)
add_test(NAME aifred_state_contract_tests COMMAND aifred_state_contract_tests)
