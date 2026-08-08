#
# CMakeLists.txt
#
# StudioLink VST3 plug-in
#
# Copyright (C) 2026 Sebastian Reimers
#

##############################################################################
#
# Project and Versioning
#
project(studiolink_vst3 VERSION 26.01.0 LANGUAGES C CXX)

set(SL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(vst3sdk_SOURCE_DIR ${SL_ROOT}/external/vst3sdk)

if(NOT EXISTS ${vst3sdk_SOURCE_DIR}/CMakeLists.txt)
  message(STATUS "VST3 SDK not found in external/vst3sdk"
                 " - skipping VST3 plug-in (run: make vst3sdk)")
  return()
endif()

##############################################################################
#
# Compile options
#
# The top level CMakeLists.txt adds a set of C only warning flags. They would
# be rejected when compiling the C++ sources of the SDK, so start from a
# clean slate here and re-apply them per language below.
#

set_directory_properties(PROPERTIES COMPILE_OPTIONS "")

set(SL_VST_C_FLAGS
    -Wall
    -Wextra
    -Wmissing-declarations
    -Wmissing-prototypes
    -Wold-style-definition
    -Wshadow
    -Wstrict-prototypes
    -Wuninitialized
    -Wvla)

if(MSVC)
  set(SL_VST_C_FLAGS "/W3")
endif()

##############################################################################
#
# VST3 SDK
#
# Examples, hosting tools and VSTGUI are not needed: StudioLink is operated
# through its web frontend, so the plug-in ships without an editor.
#

set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)
set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)
set(SMTG_RUN_VST_VALIDATOR OFF CACHE BOOL "" FORCE)

list(APPEND CMAKE_MODULE_PATH ${vst3sdk_SOURCE_DIR}/cmake/modules)

add_subdirectory(${vst3sdk_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/vst3sdk
                 EXCLUDE_FROM_ALL)

smtg_enable_vst3_sdk()

##############################################################################
#
# Source section
#

find_package(SampleRate REQUIRED)

# vstsinglecomponenteffect.cpp is not part of the "sdk" library target and
# has to be compiled into the plug-in, like the SDK samples do.
set(SRCS
  ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/vstsinglecomponenteffect.cpp
  src/driver.c
  src/engine.c
  src/io.c
  src/entry.cpp
  src/plugin.cpp
)

set(HEADERS
  include/sl_vst.h
  src/cids.h
  src/plugin.h
  src/version.h
)

##############################################################################
#
# Main target
#

set(target studiolink_vst3)

smtg_add_vst3plugin(${target} PACKAGE_NAME "StudioLink" SOURCES_LIST ${SRCS}
                    ${HEADERS})

target_include_directories(${target}
  PRIVATE
    include
    ${SAMPLERATE_INCLUDE_DIR}
)

target_link_libraries(${target}
  PRIVATE
    sdk
    sl
    baresip
    re
    ${SAMPLERATE_LIBRARIES}
)

target_compile_definitions(${target} PRIVATE ${RE_DEFINITIONS})

target_compile_features(${target} PRIVATE cxx_std_17)

target_compile_options(${target}
  PRIVATE
    $<$<COMPILE_LANGUAGE:C>:${SL_VST_C_FLAGS}>
)

if(SMTG_MAC)
  smtg_target_set_bundle(${target}
    BUNDLE_IDENTIFIER "de.studio-link.vst3.studiolink"
    COMPANY_NAME "Studio Link"
  )
elseif(SMTG_WIN)
  target_sources(${target} PRIVATE resource/studiolink.rc)
endif()
