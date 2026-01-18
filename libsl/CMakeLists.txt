#
# CMakeLists.txt
#
# Copyright (C) 2022 Sebastian Reimers
#

##############################################################################
#
# Project and Versioning
#
project(sl LANGUAGES C)

find_package(LMDB REQUIRED)

##############################################################################
#
# Source/Header section
#

set(SRCS
  src/account.c
  src/audio.c
  src/conf.c
  src/db.c
  src/flac.c
  src/http/client.c
  src/http/server.c
  src/main.c
  src/meters.c
  src/record.c
  src/tracks.c
  src/ws.c
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
  TrackView.js
  DebugView.js
  index.css
  DebugView.css
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

SET(OUTPUT_FILE cacert.h)
ADD_CUSTOM_COMMAND(
    OUTPUT ${OUTPUT_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../third_party 
    COMMAND ${RESOURCE_COMPILER} -i cacert.pem ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_FILE}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/cacert.pem
    COMMENT "Compiling cacert.pem to binary")
LIST(APPEND COMPILED_RESOURCES ${OUTPUT_FILE})


##############################################################################
#
# Main target object
#
set(LINKLIBS baresip re FLAC ${LMDB_LIBRARIES} stdc++)

if(WIN32)
  list(APPEND LINKLIBS winmm setupapi)
endif()

add_library(${PROJECT_NAME} STATIC ${SRCS} ${HEADERS} ${COMPILED_RESOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${LINKLIBS})
target_include_directories(${PROJECT_NAME} PUBLIC include PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${RE_DEFINITIONS})
  
if(WIN32)
  target_link_options(${PROJECT_NAME} PUBLIC -static)
endif()
