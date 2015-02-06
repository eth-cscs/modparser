/**************************************************************
 * unit test driver
 **************************************************************/

#include <gtest.h>

#include <runtime.h>
#ifdef PROFILING_PAPI
#include <papi_wrap.h>
#endif

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    auto success = RUN_ALL_TESTS();

    #ifdef PROFILING_PAPI
    //if(!success) pw_print_table();
    if(!success) pw_print();
    #endif

    return success;
}

