cmake_minimum_required (VERSION 2.8)
project (modparser)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
set(CMAKE_EXPORT_COMPILE_COMMANDS "YES")
include_directories(${CMAKE_SOURCE_DIR})

set(BASE_SOURCES
    constantfolder.cpp
    errorvisitor.cpp
    scope.cpp
    expression.cpp
    lexer.cpp
    module.cpp
    parser.cpp
)

set(TEST_SOURCES
    gtest-all.cpp
    test.cpp
)

add_library(compiler ${BASE_SOURCES})

#add_executable(modcc ${BASE_SOURCES} modcc.cpp)
add_executable(unittests.exe ${TEST_SOURCES})
add_executable(modcc modcc.cpp)

target_link_libraries(modcc         LINK_PUBLIC compiler)
target_link_libraries(unittests.exe LINK_PUBLIC compiler)

