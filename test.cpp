/**************************************************************
 * unit test driver
 **************************************************************/
#include "gtest.h"

#include "lexer.h"
#include "module.h"
#include "parser.h"
#include "util.h"

/**************************************************************
 * lexer tests
 **************************************************************/
//#define PRINT_LEX_STRING std::cout << "________________\n" << string << "\n________________\n";
#define PRINT_LEX_STRING

// test identifiers
TEST(Lexer, identifiers) {
    char string[] = "_foo:\nbar, buzz f_zz";
    PRINT_LEX_STRING
    Lexer lexer(string, string+sizeof(string));

    auto t1 = lexer.parse();
    EXPECT_EQ(t1.type, tok_identifier);
    EXPECT_EQ(t1.name, "_foo");
    // odds are _foo will never be a keyword
    EXPECT_EQ(is_keyword(t1), false);

    auto t2 = lexer.parse();
    EXPECT_EQ(t2.type, tok_identifier);
    EXPECT_EQ(t2.name, "bar");

    auto t3 = lexer.parse();
    EXPECT_EQ(t3.type, tok_comma);

    auto t4 = lexer.parse();
    EXPECT_EQ(t4.type, tok_identifier);
    EXPECT_EQ(t4.name, "buzz");

    auto t5 = lexer.parse();
    EXPECT_EQ(t5.type, tok_identifier);
    EXPECT_EQ(t5.name, "f_zz");

    auto t6 = lexer.parse();
    EXPECT_EQ(t6.type, tok_eof);
}

// test keywords
TEST(Lexer, keywords) {
    char string[] = "NEURON UNITS SOLVE else";
    PRINT_LEX_STRING
    Lexer lexer(string, string+sizeof(string));

    // should skip all white space and go straight to eof
    auto t1 = lexer.parse();
    EXPECT_EQ(t1.type, tok_neuron);
    EXPECT_EQ(is_keyword(t1), true);
    EXPECT_EQ(t1.name, "NEURON");

    auto t2 = lexer.parse();
    EXPECT_EQ(t2.type, tok_units);
    EXPECT_EQ(t2.name, "UNITS");

    auto t3 = lexer.parse();
    EXPECT_EQ(t3.type, tok_solve);
    EXPECT_EQ(t3.name, "SOLVE");

    auto t4 = lexer.parse();
    EXPECT_EQ(t4.type, tok_else);
    EXPECT_EQ(t4.name, "else");

    auto t5 = lexer.parse();
    EXPECT_EQ(t5.type, tok_eof);
}

// test white space
TEST(Lexer, whitespace) {
    char string[] = " \t\v\f";
    PRINT_LEX_STRING
    Lexer lexer(string, string+sizeof(string));

    // should skip all white space and go straight to eof
    auto t1 = lexer.parse();
    EXPECT_EQ(t1.type, tok_eof);
}

// test new line
TEST(Lexer, newline) {
    char string[] = "foo \n    bar \n +";
    PRINT_LEX_STRING
    Lexer lexer(string, string+sizeof(string));

    // get foo
    auto t1 = lexer.parse();
    EXPECT_EQ(t1.type, tok_identifier);
    EXPECT_EQ(t1.name, "foo");
    EXPECT_EQ(t1.location.line, 1);
    EXPECT_EQ(t1.location.column, 1);

    auto t2 = lexer.parse();
    EXPECT_EQ(t2.type, tok_identifier);
    EXPECT_EQ(t2.name, "bar");
    EXPECT_EQ(t2.location.line, 2);
    EXPECT_EQ(t2.location.column, 5);

    auto t3 = lexer.parse();
    EXPECT_EQ(t3.type, tok_plus);
    EXPECT_EQ(t3.name, "+");
    EXPECT_EQ(t3.location.line, 3);
    EXPECT_EQ(t3.location.column, 2);
}

// test operators
TEST(Lexer, symbols) {
    char string[] = "+-/*, t= ^ h'";
    PRINT_LEX_STRING
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

    // test that identifier followed by = is parsed correctly
    auto t6 = lexer.parse();
    EXPECT_EQ(t6.type, tok_identifier);

    auto t7 = lexer.parse();
    EXPECT_EQ(t7.type, tok_eq);

    auto t8 = lexer.parse();
    EXPECT_EQ(t8.type, tok_pow);

    auto t9 = lexer.parse();
    EXPECT_EQ(t9.type, tok_identifier);

    // check that prime' is parsed properly after symbol
    // as this is how it is used to indicate a derivative
    auto t10 = lexer.parse();
    EXPECT_EQ(t10.type, tok_prime);

    auto t11 = lexer.parse();
    EXPECT_EQ(t11.type, tok_eof);
}

// test braces
TEST(Lexer, braces) {
    char string[] = "foo}";
    PRINT_LEX_STRING
    Lexer lexer(string, string+sizeof(string));

    auto t1 = lexer.parse();
    EXPECT_EQ(t1.type, tok_identifier);

    auto t2 = lexer.parse();
    EXPECT_EQ(t2.type, tok_rbrace);

    auto t3 = lexer.parse();
    EXPECT_EQ(t3.type, tok_eof);
}

// test comments
TEST(Lexer, comments) {
    char string[] = "foo:this is one line\nbar : another comment\n";
    PRINT_LEX_STRING
    Lexer lexer(string, string+sizeof(string));

    auto t1 = lexer.parse();
    EXPECT_EQ(t1.type, tok_identifier);

    auto t2 = lexer.parse();
    EXPECT_EQ(t2.type, tok_identifier);
    EXPECT_EQ(t2.name, "bar");
    EXPECT_EQ(t2.location.line, 2);

    auto t3 = lexer.parse();
    EXPECT_EQ(t3.type, tok_eof);
}

// test numbers
TEST(Lexer, numbers) {
    char string[] = "1 .3 23 87.99 12. -3";
    PRINT_LEX_STRING
    Lexer lexer(string, string+sizeof(string));

    auto t1 = lexer.parse();
    EXPECT_EQ(t1.type, tok_number);
    EXPECT_EQ(t1.value(), 1.0);

    auto t2 = lexer.parse();
    EXPECT_EQ(t2.type, tok_number);
    EXPECT_EQ(t2.value(), 0.3);

    auto t3 = lexer.parse();
    EXPECT_EQ(t3.type, tok_number);
    EXPECT_EQ(t3.value(), 23.0);

    auto t4 = lexer.parse();
    EXPECT_EQ(t4.type, tok_number);
    EXPECT_EQ(t4.value(), 87.99);

    auto t5 = lexer.parse();
    EXPECT_EQ(t5.type, tok_number);
    EXPECT_EQ(t5.value(), 12.0);

    // the lexer does not decide where the - sign goes
    // the parser uses additional contextual information to
    // decide if the minus is a binary or unary expression
    auto t6 = lexer.parse();
    EXPECT_EQ(t6.type, tok_minus);

    auto t7 = lexer.parse();
    EXPECT_EQ(t7.type, tok_number);
    EXPECT_EQ(t7.value(), 3.0);

    auto t8 = lexer.parse();
    EXPECT_EQ(t8.type, tok_eof);
}

// test errors
TEST(Lexer, errors) {
    char string[] = "foo 1a";
    PRINT_LEX_STRING
    Lexer lexer(string, string+sizeof(string));

    // first read a valid token 'foo'
    auto t1 = lexer.parse();
    EXPECT_EQ(t1.type, tok_identifier);
    EXPECT_EQ(lexer.status(), ls_happy);

    // try to scan '1a' which is invalid and check that an error is generated
    auto t2 = lexer.parse();
    EXPECT_EQ(t2.type, tok_reserved); // tok_reserved is a placeholder that indicates an error
    EXPECT_EQ(lexer.status(), ls_error);

    // assert that the correct error message was generated
    EXPECT_EQ(lexer.error_message(), "found undexpected character 'a' when reading a number '1a'");
}

/**************************************************************
 * module tests
 **************************************************************/
TEST(Module, open) {
    Module m("./modfiles/test.mod");
    Lexer lexer(m.buffer());
    auto t = lexer.parse();
    while(t.type != tok_eof) {
        t = lexer.parse();
        EXPECT_NE(t.type, tok_reserved);
    }
}

/**************************************************************
 * parser tests
 **************************************************************/
TEST(Parser, open) {
    Module m("./modfiles/test.mod");
    Parser p(m);
    EXPECT_EQ(p.status(), ls_happy);
}

/**************************************************************
 * main
 **************************************************************/
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

