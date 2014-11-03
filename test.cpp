/**************************************************************
 * unit test driver
 **************************************************************/
#include "gtest.h"

/**************************************************************
 * lexer tests
 **************************************************************/
#include "lex.h"

// test identifiers
TEST(Lexer, identifiers) {
    char string[] = "foo bar";
    Lexer lexer(string, string+sizeof(string));

    auto t1 = lexer.parse();
    EXPECT_EQ(t1.type, tok_identifier);
    EXPECT_EQ(t1.name, "foo");

    auto t2 = lexer.parse();
    EXPECT_EQ(t2.type, tok_identifier);
    EXPECT_EQ(t2.name, "bar");

    auto t3 = lexer.parse();
    EXPECT_EQ(t3.type, tok_eof);
}

// test white space
TEST(Lexer, whitespace) {
    char string[] = " \t\v\f";
    Lexer lexer(string, string+sizeof(string));

    // should skip all white space and go straight to eof
    auto t1 = lexer.parse();
    EXPECT_EQ(t1.type, tok_eof);
}

// test operators
TEST(Lexer, symbols) {
    char string[] = "+-/*,";
    Lexer lexer(string, string+sizeof(string));

    auto t1 = lexer.parse();
    EXPECT_EQ(t1.type, tok_plus);

    auto t2 = lexer.parse();
    EXPECT_EQ(t2.type, tok_minus);

    auto t3 = lexer.parse();
    EXPECT_EQ(t3.type, tok_divide);

    auto t4 = lexer.parse();
    EXPECT_EQ(t4.type, tok_times);

    auto t5 = lexer.parse();
    EXPECT_EQ(t5.type, tok_comma);

    auto t6 = lexer.parse();
    EXPECT_EQ(t6.type, tok_eof);
}

// test numbers
TEST(Lexer, numbers) {
    char string[] = "1 .3 23 87.99 12.";
    Lexer lexer(string, string+sizeof(string));

    auto t1 = lexer.parse();
    EXPECT_EQ(t1.type, tok_number);
    EXPECT_EQ(t1.value, 1.0);

    auto t2 = lexer.parse();
    EXPECT_EQ(t2.type, tok_number);
    EXPECT_EQ(t2.value, 0.3);

    auto t3 = lexer.parse();
    EXPECT_EQ(t3.type, tok_number);
    EXPECT_EQ(t3.value, 23.0);

    auto t4 = lexer.parse();
    EXPECT_EQ(t4.type, tok_number);
    EXPECT_EQ(t4.value, 87.99);

    auto t5 = lexer.parse();
    EXPECT_EQ(t5.type, tok_number);
    EXPECT_EQ(t5.value, 12.0);

    auto t6 = lexer.parse();
    EXPECT_EQ(t6.type, tok_eof);
}
/**************************************************************
 * main
 **************************************************************/
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

