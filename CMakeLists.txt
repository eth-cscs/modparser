cmake_minimum_required (VERSION 2.8)

# project info
project (modparser)
enable_language(CXX)

# compilation flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -Wall -Wno-unused-function -Wno-sign-compare")

# this generates a .json file with full compilation command for each file
set(CMAKE_EXPORT_COMPILE_COMMANDS "YES")

# generated .a and .so go into /lib
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# turn on papi for runtime tests (off by default)
set( TESTS_USE_PAPI "OFF" CACHE BOOL "use PAPI instrumentation for runtime tests" )
if( "${TESTS_USE_PAPI}" STREQUAL "ON" )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPROFILING_PAPI -fopenmp")

    set( PAPI_WRAP_PATH ${CMAKE_SOURCE_DIR}/external/papi-wrap )
    set( PAPI_PATH "$ENV{PAPI_ROOT}" CACHE STRING "Path to papi library" )
    set( PAPI_LIB_PATH ${PAPI_PATH}/lib )
    include_directories(${PAPI_PATH}/include)
    link_directories   (${PAPI_PATH}/lib)
    include_directories(${PAPI_WRAP_PATH})
    link_directories   (${PAPI_WRAP_PATH}/lib)
endif()

include_directories(${CMAKE_SOURCE_DIR}/external)
include_directories(${CMAKE_SOURCE_DIR}/tests/runtime)
include_directories(${CMAKE_SOURCE_DIR}/tests/gtest)

add_subdirectory(src)
add_subdirectory(tests)

