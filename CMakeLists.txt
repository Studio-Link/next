#
# CMakeLists.txt
#
# Copyright (C) 2023 Sebastian Reimers
#

# #############################################################################
#
# Project and Versioning
#

cmake_minimum_required(VERSION 3.14)

set(CMAKE_C_COMPILER clang)

project(
  studiolink
  VERSION 26.01.0
  LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# #############################################################################
#
# Compile options
#

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
option(SL_BUILD_VST3 "Build the VST3 plug-in" OFF)

if(SL_BUILD_VST3)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if(MSVC)
  add_compile_options("/W3")
else()
  add_compile_options(
    -pedantic
    -Wall
    -Wbad-function-cast
    -Wcast-align
    -Wextra
    -Wmissing-declarations
    -Wmissing-prototypes
    -Wnested-externs
    -Wno-strict-aliasing
    -Wold-style-definition
    -Wshadow
    -Waggregate-return
    -Wstrict-prototypes
    -Wuninitialized
    -Wvla)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  add_compile_options(
      -Wshorten-64-to-32
      -Watomic-implicit-seq-cst
      -Wno-gnu-zero-variadic-macro-arguments
  )
endif()

# #############################################################################
#
# Subdirectory section
#

# target environment on the build host system
set(CMAKE_FIND_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party
                         ${CMAKE_CURRENT_SOURCE_DIR}/external)

# modify default behavior of FIND_XXX() commands
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

if(${CMAKE_BUILD_TYPE} MATCHES "[Rr]el" AND NOT UNIX)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()

set(RE_LIBRARY
    re
    CACHE STRING "")

set(LIBRE_BUILD_SHARED
    OFF
    CACHE STRING "")

set(STATIC
    ON
    CACHE BOOL "")

set(MODULES
    auconv
    auresamp
    vp8
    dtls_srtp
    g711
    ice
    netroam
    opus
    turn
    CACHE STRING "")

if(NOT SL_BUILD_VST3)
    list(APPEND MODULES alsa coreaudio jack pulse portaudio)
endif()

# Baresip dependencies
add_subdirectory(external/re EXCLUDE_FROM_ALL)
add_subdirectory(external/baresip EXCLUDE_FROM_ALL)

# Studio Link
find_package(re CONFIG REQUIRED)

add_compile_definitions(SL_VERSION="${PROJECT_VERSION}")


add_subdirectory(libsl)
add_subdirectory(test)
add_subdirectory(app/cli)

if(SL_BUILD_VST3)
  add_subdirectory(app/vst3)
endif()
