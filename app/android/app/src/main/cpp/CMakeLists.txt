
# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("api")

# target environment on the build host system
set(CMAKE_FIND_ROOT_PATH
        ${CMAKE_CURRENT_SOURCE_DIR}/external/third_party/${CMAKE_ANDROID_ARCH_ABI}
        ${CMAKE_CURRENT_SOURCE_DIR}/external/re
        ${CMAKE_CURRENT_SOURCE_DIR}/external/libsl/include
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
        dtls_srtp
        g711
        ice
        opus
        turn
        netroam
        opensles
        CACHE STRING "")

# Baresip dependencies
add_subdirectory(external/re EXCLUDE_FROM_ALL)
add_subdirectory(external/baresip EXCLUDE_FROM_ALL)

find_package(re CONFIG REQUIRED)

add_compile_definitions(SL_VERSION="${PROJECT_VERSION}")
add_subdirectory(external/libsl EXCLUDE_FROM_ALL)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.
#
# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.
add_library(${CMAKE_PROJECT_NAME} SHARED
    # List C/C++ source files with relative paths to this CMakeLists.txt.
    api.c)

# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(${CMAKE_PROJECT_NAME}
    # List libraries link to the target library
    android
    log sl baresip re)
