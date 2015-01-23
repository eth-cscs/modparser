/**************************************************************
 * unit test driver
 **************************************************************/
#include <cmath>

#include "gtest.h"

#include "constantfolder.h"
#include "variablerenamer.h"
#include "cprinter.h"
#include "expressionclassifier.h"
#include "lexer.h"
#include "module.h"
#include "parser.h"
#include "perfvisitor.h"
#include "util.h"

//#define VERBOSE_TEST
#ifdef VERBOSE_TEST
#define VERBOSE_PRINT(x) std::cout << (x) << std::endl;
#else
#define VERBOSE_PRINT(x)
#endif

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
    EXPECT_EQ(p.status(), k_compiler_happy);

    return e;
}

Expression* parse_line_expression_helper(const char* expression_string) {
    auto m = make_module(expression_string);
    Parser p(m, false);
    Expression *e = p.parse_line_expression();
    EXPECT_NE(e, nullptr);
    EXPECT_EQ(p.status(), k_compiler_happy);

    return e;
}

Expression* parse_procedure_helper(const char* expression_string) {
    auto m = make_module(expression_string);
    Parser p(m, false);
    Expression *e = p.parse_procedure();
    EXPECT_NE(e, nullptr);
    EXPECT_EQ(p.status(), k_compiler_happy);

    return e;
}

Expression* parse_function_helper(const char* expression_string) {
    auto m = make_module(expression_string);
    Parser p(m, false);
    Expression *e = p.parse_function();
    EXPECT_NE(e, nullptr);
    EXPECT_EQ(p.status(), k_compiler_happy);

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

/**************************************************************
 * visitors
 **************************************************************/
// just visually inspect for the time being
TEST(VariableRenamer, line_expressions) {
    VariableRenamer *visitor = new VariableRenamer("_", "z");

    {
    Expression *e = parse_line_expression_helper("y = _ + y");
    std::cout << e->to_string() << " --- ";
    e->accept(visitor);
    std::cout << e->to_string() << std::endl;
    }

    {
    Expression *e = parse_line_expression_helper("y = foo(_+2, exp(_))");
    std::cout << e->to_string() << " --- ";
    e->accept(visitor);
    std::cout << e->to_string() << std::endl;
    }
}

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

TEST(ClassificationVisitor, linear) {
    std::vector<const char*> expressions =
    {
"x + y + z",
"y + x + z",
"y + z + x",
"x - y - z",
"y - x - z",
"y - z - x",
"z*(x + y + 2)",
"(x + y)*z",
"(x + y)/z",
"x+3",
"-x",
"x+x+x+x",
"2*x     ",
"y*x     ",
"x + y   ",
"y + x   ",
"y + z*x ",
"2*(x*z + y)",
"z*x - y",
"(2+z)*(x*z + y)",
"x/y",
"(y - x)/z",
"(x - y)/z",
"y*(x - y)/z",
    };

    // create a scope that contains the symbols used in the tests
    auto x = new IdentifierExpression(Location(), "x");
    auto y = new IdentifierExpression(Location(), "y");
    auto z = new IdentifierExpression(Location(), "z");
    Scope::symbol_map globals = {
        {"x", {k_symbol_variable, x}},
        {"y", {k_symbol_variable, y}},
        {"z", {k_symbol_variable, z}}
    };
    auto scope = std::make_shared<Scope>(globals);

    for(auto const& expression : expressions) {
        auto m = make_module(expression);
        Parser p(m, false);
        Expression *e = p.parse_expression();

        // sanity check the compiler
        EXPECT_NE(e, nullptr);
        EXPECT_NE(p.status(), k_compiler_error);

        if( e==nullptr ) continue;

        e->semantic(scope);
        auto v = new ExpressionClassifierVisitor({k_symbol_variable, x});
        e->accept(v);
        EXPECT_EQ(v->classify(), k_expression_lin);

//#ifdef VERBOSE_TEST
        std::cout << "eq    "   << e->to_string()
                  << "\ncoeff " << v->linear_coefficient()->to_string()
                  << "\nconst " << v-> constant_term()->to_string()
                  << "\n----"   << std::endl;
//#endif
    }
}

TEST(ClassificationVisitor, constant) {
    std::vector<const char*> expressions =
    {
"y+3",
"-y",
"exp(y+z)",
"1",
"y^z",
    };

    // create a scope that contains the symbols used in the tests
    auto x = new IdentifierExpression(Location(), "x");
    auto y = new IdentifierExpression(Location(), "y");
    auto z = new IdentifierExpression(Location(), "z");
    Scope::symbol_map globals = {
        {"x", {k_symbol_variable, x}},
        {"y", {k_symbol_variable, y}},
        {"z", {k_symbol_variable, z}}
    };
    auto scope = std::make_shared<Scope>(globals);

    for(auto const& expression : expressions) {
        auto m = make_module(expression);
        Parser p(m, false);
        Expression *e = p.parse_expression();

        // sanity check the compiler
        EXPECT_NE(e, nullptr);
        EXPECT_NE(p.status(), k_compiler_error);

        if( e==nullptr ) continue;

        e->semantic(scope);
        auto v = new ExpressionClassifierVisitor({k_symbol_variable, x});
        e->accept(v);
        EXPECT_EQ(v->classify(), k_expression_const);

#ifdef VERBOSE_TEST
        if(e) std::cout << e->to_string() << std::endl;
        if(p.status()==k_compiler_error)
            std::cout << "in " << colorize(expression, kCyan) << "\t" << p.error_message() << std::endl;
#endif
    }
}

TEST(ClassificationVisitor, nonlinear) {
    std::vector<const char*> expressions =
    {
"x*x",
"x*2*x",
"x*(2+x)",
"y/x",
"x*(y + z*(x/y))",
"exp(x)",
"exp(x+y)",
"exp(z*(x+y))",
"log(x)",
"cos(x)",
"sin(x)",
"x^y",
"y^x",
    };

    // create a scope that contains the symbols used in the tests
    auto x = new IdentifierExpression(Location(), "x");
    auto y = new IdentifierExpression(Location(), "y");
    auto z = new IdentifierExpression(Location(), "z");
    Scope::symbol_map globals = {
        {"x", {k_symbol_variable, x}},
        {"y", {k_symbol_variable, y}},
        {"z", {k_symbol_variable, z}}
    };
    auto scope = std::make_shared<Scope>(globals);

    auto v = new ExpressionClassifierVisitor({k_symbol_variable, x});
    for(auto const& expression : expressions) {
        auto m = make_module(expression);
        Parser p(m, false);
        Expression *e = p.parse_expression();

        // sanity check the compiler
        EXPECT_NE(e, nullptr);
        EXPECT_NE(p.status(), k_compiler_error);

        if( e==nullptr ) continue;

        e->semantic(scope);
        v->reset();
        e->accept(v);
        EXPECT_EQ(v->classify(), k_expression_nonlin);

#ifdef VERBOSE_TEST
        if(e) std::cout << e->to_string() << std::endl;
        if(p.status()==k_compiler_error)
            std::cout << "in " << colorize(expression, kCyan) << "\t" << p.error_message() << std::endl;
#endif
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
    EXPECT_EQ(p.status(), k_compiler_happy);
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
        EXPECT_EQ(p.status(), k_compiler_happy);
        if(p.status()==k_compiler_error) {
            std::cout << str << std::endl;
            std::cout << colorize("error ", kRed) << p.error_message() << std::endl;
        }
    }
}

TEST(Parser, net_receive) {
    char str[] =
    "NET_RECEIVE (x, y) {   \n"
    "  LOCAL a              \n"
    "  a = 3                \n"
    "  x = a+3              \n"
    "  y = x+a              \n"
    "}";
    std::vector<char> input(str, str+strlen(str));
    Module m(input);
    Parser p(m, false);
    Expression *e = p.parse_procedure();
    #ifdef VERBOSE_TEST
    if(e) std::cout << e->to_string() << std::endl;
    #endif

    EXPECT_NE(e, nullptr);
    EXPECT_EQ(p.status(), k_compiler_happy);

    auto nr = e->is_net_receive();
    EXPECT_NE(nr, nullptr);
    if(nr) {
        EXPECT_EQ(nr->args().size(), 2);
    }
    if(p.status()==k_compiler_error) {
        std::cout << str << std::endl;
        std::cout << colorize("error ", kRed) << p.error_message() << std::endl;
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
        EXPECT_EQ(p.status(), k_compiler_happy);
        if(p.status()==k_compiler_error) {
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
    EXPECT_EQ(p.status(), k_compiler_happy);

    if(e) {
        SolveExpression* s = dynamic_cast<SolveExpression*>(e);
        EXPECT_EQ(s->method(), k_cnexp);
        EXPECT_EQ(s->name(), "states");
    }

    // always print the compiler errors, because they are unexpected
    if(p.status()==k_compiler_error) {
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
    ////////////////////// test for valid expressions //////////////////////
    {
        char expression[] = "LOCAL xyz";
        auto m = make_module(expression);
        Parser p(m, false);
        Expression *e = p.parse_local();

        #ifdef VERBOSE_TEST
        if(e) std::cout << e->to_string() << std::endl;
        #endif
        EXPECT_NE(e, nullptr);
        if(e) {
            EXPECT_NE(e->is_local_declaration(), nullptr);
            EXPECT_EQ(p.status(), k_compiler_happy);
        }

        // always print the compiler errors, because they are unexpected
        if(p.status()==k_compiler_error)
            std::cout << colorize("error", kRed) << p.error_message() << std::endl;
    }

    {
        char expression[] = "LOCAL x, y, z";
        auto m = make_module(expression);
        Parser p(m, false);
        Expression *e = p.parse_local();

        #ifdef VERBOSE_TEST
        if(e) std::cout << e->to_string() << std::endl;
        #endif
        EXPECT_NE(e, nullptr);
        if(e) {
            EXPECT_NE(e->is_local_declaration(), nullptr);
            EXPECT_EQ(p.status(), k_compiler_happy);
            auto vars = e->is_local_declaration()->variables();
            EXPECT_EQ(vars.size(), 3);
            EXPECT_NE(vars.find("x"), vars.end());
            EXPECT_NE(vars.find("y"), vars.end());
            EXPECT_NE(vars.find("z"), vars.end());
        }

        // always print the compiler errors, because they are unexpected
        if(p.status()==k_compiler_error)
            std::cout << colorize("error", kRed) << p.error_message() << std::endl;
    }

    ////////////////////// test for invalid expressions //////////////////////
    {
        char bad_expression[] = "LOCAL 2";
        auto m = make_module(bad_expression);
        Parser p(m, false);
        Expression *e = p.parse_local();

        EXPECT_EQ(e, nullptr);
        EXPECT_EQ(p.status(), k_compiler_error);

        #ifdef VERBOSE_TEST
        if(e) std::cout << e->to_string() << std::endl;
        if(p.status()==k_compiler_error)
            std::cout << "in " << cyan(bad_expression) << "\t" << p.error_message() << std::endl;
        #endif
    }

    {
        char bad_expression[] = "LOCAL x, ";
        auto m = make_module(bad_expression);
        Parser p(m, false);
        Expression *e = p.parse_local();

        EXPECT_EQ(e, nullptr);
        EXPECT_EQ(p.status(), k_compiler_error);

        #ifdef VERBOSE_TEST
        if(e) std::cout << e->to_string() << std::endl;
        if(p.status()==k_compiler_error)
            std::cout << "in " << cyan(bad_expression) << "\t" << p.error_message() << std::endl;
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
        EXPECT_EQ(p.status(), k_compiler_happy);

        // always print the compiler errors, because they are unexpected
        if(p.status()==k_compiler_error)
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
        EXPECT_EQ(p.status(), k_compiler_happy);

        // always print the compiler errors, because they are unexpected
        if(p.status()==k_compiler_error)
            std::cout << colorize(expression,kCyan) << "\t"
                      << colorize("error", kRed) << p.error_message() << std::endl;
    }

    std::vector<const char*> bad_expressions =
    {
"(x             ",
"((x+3)         ",
"(x+ +)         ",
"(x=3)          ",  // assignment inside parenthesis isn't allowed
"(a + (b*2^(x)) ",  // missing closing parenthesis
    };

    for(auto const& expression : bad_expressions) {
        auto m = make_module(expression);
        Parser p(m, false);
        Expression *e = p.parse_parenthesis_expression();

        EXPECT_EQ(e, nullptr);
        EXPECT_EQ(p.status(), k_compiler_error);

#ifdef VERBOSE_TEST
        if(e) std::cout << e->to_string() << std::endl;
        if(p.status()==k_compiler_error)
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
        EXPECT_EQ(p.status(), k_compiler_happy);

        // always print the compiler errors, because they are unexpected
        if(p.status()==k_compiler_error)
            std::cout << colorize("error", kRed) << p.error_message() << std::endl;
    }

    std::vector<const char*> bad_expressions =
    {
"x=2+       ",      // incomplete binary expression on rhs
"x=         ",      // missing rhs of assignment
"x=)y + 2 * z",
"x=(y + 2   ",
"x=(y ++ z  ",
"x/=3       ",      // compound binary expressions not supported
"foo+8      ",      // missing assignment
"foo()=8    ",      // lhs of assingment must be an lvalue
    };

    for(auto const& expression : bad_expressions) {
        auto m = make_module(expression);
        Parser p(m, false);
        Expression *e = p.parse_line_expression();

        EXPECT_EQ(e, nullptr);
        EXPECT_EQ(p.status(), k_compiler_error);

#ifdef VERBOSE_TEST
        if(e) std::cout << e->to_string() << std::endl;
        if(p.status()==k_compiler_error)
            std::cout << "in " << colorize(expression, kCyan) << "\t" << p.error_message() << std::endl;
#endif
    }
}

TEST(Optimizer, constant_folding) {
    ConstantFolderVisitor* v = new ConstantFolderVisitor();
    {
        char str[] = "x = 2*3";
        Expression* e = parse_line_expression_helper(str);
        VERBOSE_PRINT( e->to_string() )
        e->accept(v);
        EXPECT_EQ(e->is_assignment()->rhs()->is_number()->value(), 6);
        VERBOSE_PRINT( e->to_string() )
        VERBOSE_PRINT( "" )
    }
    {
        char str[] = "x = 1 + 2 + 3";
        Expression* e = parse_line_expression_helper(str);
        VERBOSE_PRINT( e->to_string() )
        e->accept(v);
        EXPECT_EQ(e->is_assignment()->rhs()->is_number()->value(), 6);
        VERBOSE_PRINT( e->to_string() )
        VERBOSE_PRINT( "" )
    }
    {
        char str[] = "x = exp(2)";
        Expression* e = parse_line_expression_helper(str);
        VERBOSE_PRINT( e->to_string() )
        e->accept(v);
        // The tolerance has to be loosend to 1e-15, because the optimizer performs
        // all intermediate calculations in 80 bit precision, which disagrees in
        // the 16 decimal place to the double precision value from std::exp(2.0).
        // This is a good thing: by using the constant folder we increase accuracy
        // over the unoptimized code!
        EXPECT_EQ(std::fabs(e->is_assignment()->rhs()->is_number()->value()-std::exp(2.0))<1e-15, true);
        VERBOSE_PRINT( e->to_string() )
        VERBOSE_PRINT( "" )
    }
    {
        char str[] = "x= 2*2 + 3";
        Expression* e = parse_line_expression_helper(str);
        VERBOSE_PRINT( e->to_string() )
        e->accept(v);
        EXPECT_EQ(e->is_assignment()->rhs()->is_number()->value(), 7);
        VERBOSE_PRINT( e->to_string() )
        VERBOSE_PRINT( "" )
    }
    {
        char str[] = "x= 3 + 2*2";
        Expression* e = parse_line_expression_helper(str);
        VERBOSE_PRINT( e->to_string() )
        e->accept(v);
        EXPECT_EQ(e->is_assignment()->rhs()->is_number()->value(), 7);
        VERBOSE_PRINT( e->to_string() )
        VERBOSE_PRINT( "" )
    }
    {
        // this doesn't work: the (y+2) expression is not a constant, so folding stops.
        // we need to fold the 2+3, which isn't possible with a simple walk.
        // one approach would be try sorting communtative operations so that numbers
        // are adjacent to one another in the tree
        char str[] = "x= y + 2 + 3";
        Expression* e = parse_line_expression_helper(str);
        VERBOSE_PRINT( e->to_string() )
        e->accept(v);
        VERBOSE_PRINT( e->to_string() )
        VERBOSE_PRINT( "" )
    }
    {
        char str[] = "x= 2 + 3 + y";
        Expression* e = parse_line_expression_helper(str);
        VERBOSE_PRINT( e->to_string() )
        e->accept(v);
        VERBOSE_PRINT( e->to_string() )
        VERBOSE_PRINT("");
    }
    {
        char str[] = "foo(2+3, log(32), 2*3 + x)";
        Expression* e = parse_line_expression_helper(str);
        VERBOSE_PRINT( e->to_string() )
        e->accept(v);
        VERBOSE_PRINT( e->to_string() )
        VERBOSE_PRINT("");
    }
}

TEST(CPrinter, statement) {
    std::vector<const char*> expressions =
    {
"y=x+3",
"y=y^z",
"y=exp(x/2 + 3)",
    };

    // create a scope that contains the symbols used in the tests
    auto x = new IdentifierExpression(Location(), "x");
    auto y = new IdentifierExpression(Location(), "y");
    auto z = new IdentifierExpression(Location(), "z");
    Scope::symbol_map symbols = {
        {"x", {k_symbol_variable, x}},
        {"y", {k_symbol_variable, y}},
        {"z", {k_symbol_variable, z}}
    };
    auto scope = std::make_shared<Scope>(symbols);

    for(auto const& expression : expressions) {
        auto m = make_module(expression);
        Parser p(m, false);
        Expression *e = p.parse_line_expression();

        // sanity check the compiler
        EXPECT_NE(e, nullptr);
        EXPECT_NE(p.status(), k_compiler_error);

        if(p.status()==k_compiler_error)
            std::cout << colorize("error", kRed) << p.error_message() << std::endl;
        if( e==nullptr ) continue;

        e->semantic(scope);
        auto v = new CPrinter();
        e->accept(v);

#ifdef VERBOSE_TEST
        std::cout << e->to_string() << std::endl;
                  << " :--: " << v->text() << std::endl;
#endif
    }
}

TEST(CPrinter, proc) {
    std::vector<const char*> expressions =
    {
"PROCEDURE trates(v) {\n"
"    LOCAL k\n"
"    minf=1-1/(1+exp((v-k)/k))\n"
"    hinf=1/(1+exp((v-k)/k))\n"
"    mtau = 0.6\n"
"    htau = 1500\n"
"}"
    };

    // create a scope that contains the symbols used in the tests
    auto minf = new IdentifierExpression(Location(), "minf");
    auto hinf = new IdentifierExpression(Location(), "hinf");
    auto mtau = new IdentifierExpression(Location(), "mtau");
    auto htau = new IdentifierExpression(Location(), "htau");
    auto v    = new IdentifierExpression(Location(), "v");
    for(auto const& expression : expressions) {
        Expression *e = parse_procedure_helper(expression);

        // sanity check the compiler
        EXPECT_NE(e, nullptr);

        if( e==nullptr ) continue;

        Scope::symbol_map symbols = {
            {"minf",   {k_symbol_variable,  minf}},
            {"hinf",   {k_symbol_variable,  hinf}},
            {"mtau",   {k_symbol_variable,  mtau}},
            {"htau",   {k_symbol_variable,  htau}},
            {"v",      {k_symbol_variable,  v}},
            {"trates", {k_symbol_procedure, e}},
        };

        e->semantic(symbols);
        auto v = new CPrinter();
        e->accept(v);

#ifdef VERBOSE_TEST
        std::cout << e->to_string() << std::endl;
                  << " :--: " << v->text() << std::endl;
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

