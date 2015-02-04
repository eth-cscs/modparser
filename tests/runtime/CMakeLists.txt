set(RUNTIME_SOURCES
    # google test framework
    ../gtest-all.cpp

    # runtime library
    runtime.cpp

    # individual mechanism tests
    test_NaTs2_t.cpp

    # unit test driver
    driver.cpp
)

add_executable(test_runtime ${RUNTIME_SOURCES})

target_link_libraries(test_runtime LINK_PUBLIC compiler)

set_target_properties( test_runtime
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)
