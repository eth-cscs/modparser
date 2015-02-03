cmake_minimum_required (VERSION 2.8)
project (modparser)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -Wall -Wno-unused-function")
set(CMAKE_EXPORT_COMPILE_COMMANDS "YES")
include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(src)

