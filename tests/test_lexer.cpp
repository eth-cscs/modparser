#include <cmath>

#include "test.h"
#include "../src/lexer.h"

//#define PRINT_LEX_STRING std::cout << "________________\n" << string << "\n________________\n";
#define PRINT_LEX_STRING

/**************************************************************
 * lexer tests
 **************************************************************/
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
    char string[] = "NEURON UNITS SOLVE else TITLE";
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
    //EXPECT_EQ(t5.type, tok_title);
    EXPECT_NE(t5.type, tok_identifier);
    EXPECT_EQ(t5.name, "TITLE");

    auto t6 = lexer.parse();
    EXPECT_EQ(t6.type, tok_eof);
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
    char string[] = "foo \n    bar \n +\r\n-";
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

    // test for carriage return + newline, i.e. \r\n
    auto t4 = lexer.parse();
    EXPECT_EQ(t4.type, tok_minus);
    EXPECT_EQ(t4.name, "-");
    EXPECT_EQ(t4.location.line, 4);
    EXPECT_EQ(t4.location.column, 1);
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

TEST(Lexer, comparison_operators) {
    char string[] = "< <= > >= == != !";
    PRINT_LEX_STRING
    Lexer lexer(string, string+sizeof(string));

    auto t1 = lexer.parse();
    EXPECT_EQ(t1.type, tok_lt);
    auto t2 = lexer.parse();
    EXPECT_EQ(t2.type, tok_lte);
    auto t3 = lexer.parse();
    EXPECT_EQ(t3.type, tok_gt);
    auto t4 = lexer.parse();
    EXPECT_EQ(t4.type, tok_gte);
    auto t5 = lexer.parse();
    EXPECT_EQ(t5.type, tok_EQ);
    auto t6 = lexer.parse();
    EXPECT_EQ(t6.type, tok_ne);
    auto t7 = lexer.parse();
    EXPECT_EQ(t7.type, tok_not);

    auto t8 = lexer.parse();
    EXPECT_EQ(t8.type, tok_eof);
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
    EXPECT_EQ(std::stod(t1.name), 1.0);

    auto t2 = lexer.parse();
    EXPECT_EQ(t2.type, tok_number);
    EXPECT_EQ(std::stod(t2.name), 0.3);

    auto t3 = lexer.parse();
    EXPECT_EQ(t3.type, tok_number);
    EXPECT_EQ(std::stod(t3.name), 23.0);

    auto t4 = lexer.parse();
    EXPECT_EQ(t4.type, tok_number);
    EXPECT_EQ(std::stod(t4.name), 87.99);

    auto t5 = lexer.parse();
    EXPECT_EQ(t5.type, tok_number);
    EXPECT_EQ(std::stod(t5.name), 12.0);

    // the lexer does not decide where the - sign goes
    // the parser uses additional contextual information to
    // decide if the minus is a binary or unary expression
    auto t6 = lexer.parse();
    EXPECT_EQ(t6.type, tok_minus);

    auto t7 = lexer.parse();
    EXPECT_EQ(t7.type, tok_number);
    EXPECT_EQ(std::stod(t7.name), 3.0);

    auto t8 = lexer.parse();
    EXPECT_EQ(t8.type, tok_eof);
}


