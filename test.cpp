/**************************************************************
 * unit test driver
 **************************************************************/
#include "gtest.h"

#include "lexer.h"
#include "module.h"
#include "parser.h"
#include "perfvisitor.h"
#include "util.h"

//#define VERBOSE_TEST

// helper for making a module
Module make_module(const char* str) {
        std::vector<char> input(str, str+strlen(str));
        return Module(input);
}

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
    //EXPECT_EQ(lexer.error_message(), "found unexpected character 'a' when reading a number '1a'");
}

/**************************************************************
 * visitors
 **************************************************************/
TEST(FlopVisitor, simple_expressions) {
    FlopVisitor *visitor = new FlopVisitor();
    const char* expression = "exp(x)";

    auto m = make_module(expression);
    Parser p(m, false);
    Expression *e = p.parse_expression();

    EXPECT_NE(e, nullptr);
    EXPECT_EQ(p.status(), ls_happy);

    e->accept(visitor);
    std::cout << e->to_string() << std::endl;
    std::cout << visitor->flops << std::endl;
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
TEST(Parser, parser_full_file) {
    Module m("./modfiles/test.mod");
    Parser p(m);
    EXPECT_EQ(p.status(), ls_happy);
}

TEST(Parser, procedure) {
    std::vector< const char*> calls =
{
"PROCEDURE foo(x, y) {"
"  LOCAL a\n"
"  LOCAL b\n"
"  LOCAL c\n"
"  a = 3\n"
"  b = x * y + 2\n"
"  y = x + y * 2\n"
"  y = a + b +c + a + b\n"
"  y = a + b *c + a + b\n"
"}"
,
"PROCEDURE trates(v) {\n"
"    LOCAL qt\n"
"    qt=q10^((celsius-22)/10)\n"
"    minf=1-1/(1+exp((v-vhalfm)/km))\n"
"    hinf=1/(1+exp((v-vhalfh)/kh))\n"
"    mtau = 0.6\n"
"    htau = 1500\n"
"}"
};
    for(auto const& str : calls) {
        std::vector<char> input(str, str+strlen(str));
        Module m(input);
        Parser p(m, false);
        Expression *e = p.parse_procedure();
#ifdef VERBOSE_TEST
        if(e) std::cout << e->to_string() << std::endl;
#endif
        EXPECT_NE(e, nullptr);
        EXPECT_EQ(p.status(), ls_happy);
        if(p.status()==ls_error) {
            std::cout << str << std::endl;
            std::cout << colorize("error ", kRed) << p.error_message() << std::endl;
        }
    }
}

TEST(Parser, parse_solve) {
    const char* expression = "SOLVE states METHOD cnexp";

    auto m = make_module(expression);
    Parser p(m, false);
    Expression *e = p.parse_solve();

#ifdef VERBOSE_TEST
    if(e) std::cout << e->to_string() << std::endl;
#endif
    EXPECT_NE(e, nullptr);
    EXPECT_EQ(p.status(), ls_happy);

    if(e) {
        SolveExpression* s = dynamic_cast<SolveExpression*>(e);
        EXPECT_EQ(s->method(), k_cnexp);
        EXPECT_EQ(s->name(), "states");
    }

    // always print the compiler errors, because they are unexpected
    if(p.status()==ls_error) {
        std::cout << colorize("error", kRed) << p.error_message() << std::endl;
    }
}


TEST(Parser, parse_local) {
    std::vector<const char*> good_expressions =
    {
"LOCAL xyz\n",
"LOCAL xyz : comment\n"
    };

    for(auto const& expression : good_expressions) {
        auto m = make_module(expression);
        Parser p(m, false);
        Expression *e = p.parse_local();

#ifdef VERBOSE_TEST
        if(e) std::cout << e->to_string() << std::endl;
#endif
        EXPECT_NE(e, nullptr);
        EXPECT_EQ(p.status(), ls_happy);

        // always print the compiler errors, because they are unexpected
        if(p.status()==ls_error)
            std::cout << colorize("error", kRed) << p.error_message() << std::endl;
    }

    std::vector<const char*> bad_expressions =
    {
"LOCAL 2",
"LOCAL x = 3",
"LOCAL x - 3",
    };

    for(auto const& expression : bad_expressions) {
        auto m = make_module(expression);
        Parser p(m, false);
        Expression *e = p.parse_local();

        EXPECT_EQ(e, nullptr);
        EXPECT_EQ(p.status(), ls_error);

#ifdef VERBOSE_TEST
        if(e) std::cout << e->to_string() << std::endl;
        if(p.status()==ls_error)
            std::cout << "in " << colorize(expression, kCyan) << "\t" << p.error_message() << std::endl;
#endif
    }
}

TEST(Parser, parse_unary_expression) {
    std::vector<const char*> good_expressions =
    {
"+x             ",
"-x             ",
"(x + -y)       ",
"-(x - + -y)    ",
"exp(x + y)     ",
"-exp(x + -y)   ",
    };

    for(auto const& expression : good_expressions) {
        auto m = make_module(expression);
        Parser p(m, false);
        Expression *e = p.parse_unaryop();

#ifdef VERBOSE_TEST
        if(e) std::cout << e->to_string() << std::endl;
#endif
        EXPECT_NE(e, nullptr);
        EXPECT_EQ(p.status(), ls_happy);

        // always print the compiler errors, because they are unexpected
        if(p.status()==ls_error)
            std::cout << colorize("error", kRed) << p.error_message() << std::endl;
    }
}

// test parsing of parenthesis expressions
TEST(Parser, parse_parenthesis_expression) {
    std::vector<const char*> good_expressions =
    {
"((celsius-22)/10)      ",
"((celsius-22)+10)      ",
"(x+2)                  ",
"((x))                  ",
"(((x)))                ",
"(x + (x * (y*(2)) + 4))",
    };

    for(auto const& expression : good_expressions) {
        auto m = make_module(expression);
        Parser p(m, false);
        Expression *e = p.parse_parenthesis_expression();

#ifdef VERBOSE_TEST
        if(e) std::cout << e->to_string() << std::endl;
#endif
        EXPECT_NE(e, nullptr);
        EXPECT_EQ(p.status(), ls_happy);

        // always print the compiler errors, because they are unexpected
        if(p.status()==ls_error)
            std::cout << colorize(expression,kCyan) << "\t"
                      << colorize("error", kRed) << p.error_message() << std::endl;
    }

    std::vector<const char*> bad_expressions =
    {
"(x             ",
"((x+3)         ",
"(x+ +)         ",
"(x=3)          ",
"(a + (b*2^(x)) ",
    };

    for(auto const& expression : bad_expressions) {
        auto m = make_module(expression);
        Parser p(m, false);
        Expression *e = p.parse_parenthesis_expression();

        EXPECT_EQ(e, nullptr);
        EXPECT_EQ(p.status(), ls_error);

#ifdef VERBOSE_TEST
        if(e) std::cout << e->to_string() << std::endl;
        if(p.status()==ls_error)
            std::cout << "in " << colorize(expression, kCyan) << "\t" << p.error_message() << std::endl;
#endif
    }
}

// test parsing of line expressions
TEST(Parser, parse_line_expression) {
    std::vector<const char*> good_expressions =
    {
"qt=q10^((celsius-22)/10)"
"x=2        ",
"x=2        ",
"x = -y\n   "
"x=2*y      ",
"x=y + 2 * z",
"x=(y + 2) * z      ",
"x=(y + 2) * z ^ 3  ",
"x=(y + 2 * z ^ 3)  ",
"foo(x+3, y, bar(21.4))",
"y=exp(x+3) + log(exp(x/y))",
    };

    for(auto const& expression : good_expressions) {
        auto m = make_module(expression);
        Parser p(m, false);
        Expression *e = p.parse_line_expression();

#ifdef VERBOSE_TEST
        if(e) std::cout << e->to_string() << std::endl;
#endif
        EXPECT_NE(e, nullptr);
        EXPECT_EQ(p.status(), ls_happy);

        // always print the compiler errors, because they are unexpected
        if(p.status()==ls_error)
            std::cout << colorize("error", kRed) << p.error_message() << std::endl;
    }

    std::vector<const char*> bad_expressions =
    {
"x=2+       ",
"x=         ",
"x=)y + 2 * z",
"x=(y + 2   ",
"x=(y ++ z  ",
"x/=3       ",
"foo+8      ",
"foo()=8    ",
    };

    for(auto const& expression : bad_expressions) {
        auto m = make_module(expression);
        Parser p(m, false);
        Expression *e = p.parse_line_expression();

        EXPECT_EQ(e, nullptr);
        EXPECT_EQ(p.status(), ls_error);

#ifdef VERBOSE_TEST
        if(e) std::cout << e->to_string() << std::endl;
        if(p.status()==ls_error)
            std::cout << "in " << colorize(expression, kCyan) << "\t" << p.error_message() << std::endl;
#endif
    }
}

/**************************************************************
 * expression tests
 **************************************************************/
TEST(Expression, variable_constructors) {
    // check that default values for constructor work
    {
        Variable v("v");
        v.set_range(k_range);
        EXPECT_TRUE(v.is_range());
        EXPECT_FALSE(v.is_scalar());

        v.set_state(true);
        EXPECT_TRUE(v.is_state());
        v.set_state(false);
        EXPECT_FALSE(v.is_state());

        v.set_ion_channel(k_ion_Na);
        EXPECT_TRUE(v.is_ion());
        v.set_ion_channel(k_ion_none);
        EXPECT_FALSE(v.is_ion());
    }
}

/**************************************************************
 * main
 **************************************************************/
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

