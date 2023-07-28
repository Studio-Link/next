#
# CMakeLists.txt
#
# Copyright (C) 2022 Sebastian Reimers
#

##############################################################################
#
# Project and Versioning
#
project(studiolink VERSION 0.0.1 LANGUAGES C)

##############################################################################
#
# Source/Header section
#

set(SRCS
  src/main.c
)

##############################################################################
#
# Main target object
#

add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} PUBLIC sl baresip re)
target_compile_definitions(${PROJECT_NAME} PRIVATE ${RE_DEFINITIONS})
