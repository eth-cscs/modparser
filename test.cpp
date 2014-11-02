/**************************************************************
 * unit test driver
 **************************************************************/
#include "gtest.h"

/**************************************************************
 * lexer tests
 **************************************************************/
#include "lex.h"

// test something
TEST(Lexer, constructor) {
    char string[] = "a ba c";
    size_t stringsize = sizeof(string);
    Lexer lexer(string, string+stringsize);
    //EXPECT_EQ(1, 1);
}

/**************************************************************
 * main
 **************************************************************/
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

