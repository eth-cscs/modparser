set(RUNTIME_SOURCES
    # runtime library
    runtime.cpp

    # individual mechanism tests
    test_NaTs2_t.cpp
    test_Ca_HVA.cpp
    test_timestep.cpp

    # unit test driver
    driver.cpp
)

add_executable(test_runtime ${RUNTIME_SOURCES})

target_link_libraries(test_runtime LINK_PUBLIC compiler gtest)

# add papi library dependencies
if( "${TESTS_USE_PAPI}" STREQUAL "ON" )
    # order matters: papi-wrap before libpapi
    target_link_libraries(test_runtime LINK_PUBLIC ${PAPI_WRAP_PATH}/lib/libpapi_wrap.a)
    target_link_libraries(test_runtime LINK_PUBLIC ${PAPI_LIB_PATH}/libpapi.a)
endif()

set_target_properties( test_runtime
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)
