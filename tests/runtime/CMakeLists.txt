set(RUNTIME_SOURCES
    #../gtest-all.cpp
    runtime.cpp

    # google test library sources
    Na_test.cpp
)

add_executable(test_runtime ${RUNTIME_SOURCES})

target_link_libraries(test_runtime LINK_PUBLIC compiler)

set_target_properties( test_runtime
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)
