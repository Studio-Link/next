#
# CMakeLists.txt
#
# Copyright (C) 2022 Sebastian Reimers
#

##############################################################################
#
# Project and Versioning
#
project(test VERSION 0.0.1 LANGUAGES C)

##############################################################################
#
# Source/Header section
#

set(SRCS
  src/db.c
  src/http.c
  src/main.c
  src/tracks.c
  src/mock/mock_auplay.c
  src/mock/mock_ausrc.c
)

##############################################################################
#
# Main target object
#

add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} PRIVATE sl)
target_compile_definitions(${PROJECT_NAME} PRIVATE ${RE_DEFINITIONS})
