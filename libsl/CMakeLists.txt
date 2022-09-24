#
# CMakeLists.txt
#
# Copyright (C) 2022 Sebastian Reimers
#

##############################################################################
#
# Project and Versioning
#
project(sl VERSION 0.0.1 LANGUAGES C)

##############################################################################
#
# Source/Header section
#

set(SRCS
  src/audio.c
  src/http.c
  src/main.c
  src/tracks.c
  src/ws.c
  src/ws_tracks.c
)

set(HEADERS
  include/studiolink.h
)

##############################################################################
#
# Assets
#

find_program(RESOURCE_COMPILER xxd)

set(COMPILED_RESOURCES
  index.html
  index.js
  vendor.js
  index.css
  roboto-mono-latin-400.woff2
  roboto-mono-latin-500.woff2
  roboto-mono-latin-600.woff2
  roboto-mono-latin-700.woff2
  logo_standalone.svg
  logo_solo.svg
) 

FOREACH(INPUT_FILE ${COMPILED_RESOURCES})
    SET(OUTPUT_FILE ${INPUT_FILE}.h)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${OUTPUT_FILE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../webui 
        COMMAND ${RESOURCE_COMPILER} -i dist/${INPUT_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_FILE}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../webui/dist/${INPUT_FILE}
        COMMENT "Compiling ${INPUT_FILE} to binary")
    LIST(APPEND COMPILED_RESOURCES ${OUTPUT_FILE})
ENDFOREACH()


##############################################################################
#
# Main target object
#

add_library(${PROJECT_NAME} STATIC ${SRCS} ${HEADERS} ${COMPILED_RESOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC baresip rem re)
target_include_directories(${PROJECT_NAME} PUBLIC include PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${RE_DEFINITIONS})
