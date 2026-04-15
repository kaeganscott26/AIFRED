cmake_minimum_required(VERSION 3.24)

project(AIFRED VERSION 0.3.2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(AIFRED_BUILD_PLUGIN "Build the Aifred VST3 plugin" ON)

if(AIFRED_BUILD_PLUGIN)
  add_subdirectory(plugin-aifred)
endif()
