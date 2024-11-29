# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required(VERSION 3.25)

project(svchostify CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    string(APPEND CMAKE_CXX_FLAGS " -frtti -fexceptions -fvisibility=hidden")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    string(APPEND CMAKE_C_FLAGS " /utf-8")
else()
    string(APPEND CMAKE_C_FLAGS " -fvisibility=hidden")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    string(APPEND CMAKE_CXX_FLAGS " /utf-8")
endif()

include(CTest)
include(Dependencies.cmake)

add_subdirectory(src)
add_subdirectory(samples/cpp/test-service)
