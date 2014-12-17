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

Expression* parse_expression_helper(const char* expression_string) {
    auto m = make_module(expression_string);
    Parser p(m, false);
    Expression *e = p.parse_expression();
    EXPECT_NE(e, nullptr);
    EXPECT_EQ(p.status(), ls_happy);

    return e;
}

Expression* parse_line_expression_helper(const char* expression_string) {
    auto m = make_module(expression_string);
    Parser p(m, false);
    Expression *e = p.parse_line_expression();
    EXPECT_NE(e, nullptr);
    EXPECT_EQ(p.status(), ls_happy);

    return e;
}

Expression* parse_procedure_helper(const char* expression_string) {
    auto m = make_module(expression_string);
    Parser p(m, false);
    Expression *e = p.parse_procedure();
    EXPECT_NE(e, nullptr);
    EXPECT_EQ(p.status(), ls_happy);

    return e;
}

Expression* parse_function_helper(const char* expression_string) {
    auto m = make_module(expression_string);
    Parser p(m, false);
    Expression *e = p.parse_function();
    EXPECT_NE(e, nullptr);
    EXPECT_EQ(p.status(), ls_happy);

    return e;
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

/**************************************************************
 * visitors
 **************************************************************/
TEST(FlopVisitor, basic) {
    {
    FlopVisitor *visitor = new FlopVisitor();
    Expression *e = parse_expression_helper("x+y");
    e->accept(visitor);
    EXPECT_EQ(visitor->flops.add, 1);
    }

    {
    FlopVisitor *visitor = new FlopVisitor();
    Expression *e = parse_expression_helper("x-y");
    e->accept(visitor);
    EXPECT_EQ(visitor->flops.sub, 1);
    }

    {
    FlopVisitor *visitor = new FlopVisitor();
    Expression *e = parse_expression_helper("x*y");
    e->accept(visitor);
    EXPECT_EQ(visitor->flops.mul, 1);
    }

    {
    FlopVisitor *visitor = new FlopVisitor();
    Expression *e = parse_expression_helper("x/y");
    e->accept(visitor);
    EXPECT_EQ(visitor->flops.div, 1);
    }

    {
    FlopVisitor *visitor = new FlopVisitor();
    Expression *e = parse_expression_helper("exp(x)");
    e->accept(visitor);
    EXPECT_EQ(visitor->flops.exp, 1);
    }

    {
    FlopVisitor *visitor = new FlopVisitor();
    Expression *e = parse_expression_helper("log(x)");
    e->accept(visitor);
    EXPECT_EQ(visitor->flops.log, 1);
    }

    {
    FlopVisitor *visitor = new FlopVisitor();
    Expression *e = parse_expression_helper("cos(x)");
    e->accept(visitor);
    EXPECT_EQ(visitor->flops.cos, 1);
    }

    {
    FlopVisitor *visitor = new FlopVisitor();
    Expression *e = parse_expression_helper("sin(x)");
    e->accept(visitor);
    EXPECT_EQ(visitor->flops.sin, 1);
    }
}

TEST(FlopVisitor, compound) {
    {
    FlopVisitor *visitor = new FlopVisitor();
    Expression *e = parse_expression_helper("x+y*z/a-b");
    e->accept(visitor);
    EXPECT_EQ(visitor->flops.add, 1);
    EXPECT_EQ(visitor->flops.sub, 1);
    EXPECT_EQ(visitor->flops.mul, 1);
    EXPECT_EQ(visitor->flops.div, 1);
    }

    {
    FlopVisitor *visitor = new FlopVisitor();
    Expression *e = parse_expression_helper("exp(x+y+z)");
    e->accept(visitor);
    EXPECT_EQ(visitor->flops.add, 2);
    EXPECT_EQ(visitor->flops.exp, 1);
    }

    {
    FlopVisitor *visitor = new FlopVisitor();
    Expression *e = parse_expression_helper("exp(x+y) + 3/(12 + z)");
    e->accept(visitor);
    EXPECT_EQ(visitor->flops.add, 3);
    EXPECT_EQ(visitor->flops.div, 1);
    EXPECT_EQ(visitor->flops.exp, 1);
    }

    // test asssignment expression
    {
    FlopVisitor *visitor = new FlopVisitor();
    Expression *e = parse_line_expression_helper("x = exp(x+y) + 3/(12 + z)");
    e->accept(visitor);
    EXPECT_EQ(visitor->flops.add, 3);
    EXPECT_EQ(visitor->flops.div, 1);
    EXPECT_EQ(visitor->flops.exp, 1);
    }
}

TEST(FlopVisitor, procedure) {
    {
    const char *expression =
"PROCEDURE trates(v) {\n"
"    LOCAL qt\n"
"    qt=q10^((celsius-22)/10)\n"
"    minf=1-1/(1+exp((v-vhalfm)/km))\n"
"    hinf=1/(1+exp((v-vhalfh)/kh))\n"
"    mtau = 0.6\n"
"    htau = 1500\n"
"}";
    FlopVisitor *visitor = new FlopVisitor();
    Expression *e = parse_procedure_helper(expression);
    e->accept(visitor);
    EXPECT_EQ(visitor->flops.add, 2);
    EXPECT_EQ(visitor->flops.sub, 4);
    EXPECT_EQ(visitor->flops.mul, 0);
    EXPECT_EQ(visitor->flops.div, 5);
    EXPECT_EQ(visitor->flops.exp, 2);
    EXPECT_EQ(visitor->flops.pow, 1);
    }
}

TEST(FlopVisitor, function) {
    {
    const char *expression =
"FUNCTION foo(v) {\n"
"    LOCAL qt\n"
"    qt=q10^((celsius-22)/10)\n"
"    minf=1-1/(1+exp((v-vhalfm)/km))\n"
"    hinf=1/(1+exp((v-vhalfh)/kh))\n"
"    foo = minf + hinf\n"
"}";
    FlopVisitor *visitor = new FlopVisitor();
    Expression *e = parse_function_helper(expression);
    e->accept(visitor);
    EXPECT_EQ(visitor->flops.add, 3);
    EXPECT_EQ(visitor->flops.sub, 4);
    EXPECT_EQ(visitor->flops.mul, 0);
    EXPECT_EQ(visitor->flops.div, 5);
    EXPECT_EQ(visitor->flops.exp, 2);
    EXPECT_EQ(visitor->flops.pow, 1);
    }
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

TEST(Parser, function) {
    std::vector< const char*> calls =
{
"FUNCTION foo(x, y) {"
"  LOCAL a\n"
"  a = 3\n"
"  b = x * y + 2\n"
"  y = x + y * 2\n"
"  foo = a * x + y\n"
"}"
};
    for(auto const& str : calls) {
        std::vector<char> input(str, str+strlen(str));
        Module m(input);
        Parser p(m, false);
        Expression *e = p.parse_function();
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

TEST(Parser, parse_if) {
    {
        char expression[] =
        "   if(a<b) {      \n"
        "       a = 2+b    \n"
        "       b = 4^b    \n"
        "   }              \n";
        auto m = make_module(expression);
        Parser p(m, false);
        Expression *e = p.parse_if();
        EXPECT_NE(e, nullptr);
        if(e) {
            auto ife = e->is_if();
            EXPECT_NE(e->is_if(), nullptr);
            if(ife) {
                EXPECT_NE(ife->condition()->is_binary(), nullptr);
                EXPECT_NE(ife->true_branch()->is_block(), nullptr);
                EXPECT_EQ(ife->false_branch(), nullptr);
            }
            //std::cout << e->to_string() << std::endl;
        }
        else {
            std::cout << p.error_message() << std::endl;
        }
    }
    {
        char expression[] =
        "   if(a<b) {      \n"
        "       a = 2+b    \n"
        "   } else {       \n"
        "       a = 2+b    \n"
        "   }                ";
        auto m = make_module(expression);
        Parser p(m, false);
        Expression *e = p.parse_if();
        EXPECT_NE(e, nullptr);
        if(e) {
            auto ife = e->is_if();
            EXPECT_NE(ife, nullptr);
            if(ife) {
                EXPECT_NE(ife->condition()->is_binary(), nullptr);
                EXPECT_NE(ife->true_branch()->is_block(), nullptr);
                EXPECT_NE(ife->false_branch(), nullptr);
            }
            //std::cout << std::endl << e->to_string() << std::endl;
        }
        else {
            std::cout << p.error_message() << std::endl;
        }
    }
    {
        char expression[] =
        "   if(a<b) {      \n"
        "       a = 2+b    \n"
        "   } else if(b>a){\n"
        "       a = 2+b    \n"
        "   }              ";
        auto m = make_module(expression);
        Parser p(m, false);
        Expression *e = p.parse_if();
        EXPECT_NE(e, nullptr);
        if(e) {
            auto ife = e->is_if();
            EXPECT_NE(ife, nullptr);
            if(ife) {
                EXPECT_NE(ife->condition()->is_binary(), nullptr);
                EXPECT_NE(ife->true_branch()->is_block(), nullptr);
                EXPECT_NE(ife->false_branch(), nullptr);
                EXPECT_NE(ife->false_branch()->is_if(), nullptr);
                EXPECT_EQ(ife->false_branch()->is_if()->false_branch(), nullptr);
            }
            //std::cout << std::endl << e->to_string() << std::endl;
        }
        else {
            std::cout << p.error_message() << std::endl;
        }
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
        EXPECT_NE(e->is_local_declaration(), nullptr);
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
"a=x^y^z",
"a=x/y/z"
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
 * main
 **************************************************************/
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

