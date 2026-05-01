cmake_minimum_required(VERSION 3.24)

project(AIFRED VERSION 0.3.5 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(AIFRED_BUILD_PLUGIN "Build the Aifred VST3 plugin" ON)

if(AIFRED_BUILD_PLUGIN)
  add_subdirectory(plugin-aifred)
endif()

if(WIN32)
  set(AIFRED_VST3_INSTALL_DIR "$ENV{ProgramFiles}/Common Files/VST3")
elseif(UNIX)
  set(AIFRED_VST3_INSTALL_DIR "lib/vst3")
endif()

install(CODE [[
  if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME OR CMAKE_INSTALL_CONFIG_NAME STREQUAL "")
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  set(plugin_path "${CMAKE_BINARY_DIR}/plugin-aifred/Aifred_artefacts/${CMAKE_INSTALL_CONFIG_NAME}/VST3/Aifred.vst3")
  if(NOT EXISTS "${plugin_path}")
    message(FATAL_ERROR "Aifred.vst3 was not found at ${plugin_path}; build the VST3 target before packaging.")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}" TYPE DIRECTORY FILES "${plugin_path}")
]])

set(CPACK_PACKAGE_NAME "AIFRED VST3")
set(CPACK_PACKAGE_VENDOR "North3rnLight3r")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "north3rnlight3rofficial@outlook.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AIFRED Diagnostic Suite VST3")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

if(WIN32)
  set(CPACK_GENERATOR "NSIS;ZIP")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "")
  set(CPACK_NSIS_DISPLAY_NAME "AIFRED VST3")
  set(CPACK_NSIS_PACKAGE_NAME "AIFRED VST3")
  set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "Common Files\\\\VST3")
  set(CMAKE_INSTALL_PREFIX "${AIFRED_VST3_INSTALL_DIR}" CACHE PATH "AIFRED install path" FORCE)
elseif(UNIX)
  set(CPACK_GENERATOR "DEB;TGZ")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "North3rnLight3r")
  set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "AIFRED install path" FORCE)
endif()

include(CPack)
