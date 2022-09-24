#
# CMakeLists.txt
#
# Copyright (C) 2022 Sebastian Reimers
#

##############################################################################
#
# Project and Versioning
#

cmake_minimum_required(VERSION 3.18)

set(CMAKE_C_COMPILER clang)

project(studiolink VERSION 0.0.1 LANGUAGES C)


##############################################################################
#
# Compile options
#

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)

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
    -Wshadow -Waggregate-return
    -Wstrict-prototypes
    -Wuninitialized
    -Wvla
  )
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wshorten-64-to-32 -Watomic-implicit-seq-cst)
endif()


##############################################################################
#
# Subdirectory section
#

# target environment on the build host system
set(CMAKE_FIND_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party ${CMAKE_CURRENT_SOURCE_DIR}/external)

# modify default behavior of FIND_XXX() commands
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(RE_LIBRARY re CACHE STRING "")
set(REM_LIBRARY rem CACHE STRING "")

set(STATIC ON CACHE BOOL "")
set(MODULES ice dtls_srtp turn opus g711)

# Baresip dependencies
add_subdirectory(external/re)
add_subdirectory(external/rem)
add_subdirectory(external/baresip)

# Studio Link 
add_subdirectory(libsl)
add_subdirectory(app/linux)
