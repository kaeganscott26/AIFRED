cmake_minimum_required(VERSION 3.24)

project(AIFRED VERSION 0.4.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(AIFRED_BUILD_PLUGIN "Build the Aifred VST3 plugin" ON)

enable_testing()

add_subdirectory(plugin-aifred)
