#include "test.h"

#include "../src/constantfolder.h"
#include "../src/expressionclassifier.h"
//#include "../src/variablerenamer.h"
#include "../src/perfvisitor.h"

#include "../src/parser.h"
#include "../src/util.h"

/**************************************************************
 * visitors
 **************************************************************/
// just visually inspect for the time being
/*
TEST(VariableRenamer, line_expressions) {
    auto visitor = make_unique<VariableRenamer>("_", "z");

    {
    auto e = parse_line_expression("y = _ + y");
    e->accept(visitor.get());
    }

    {
    auto e = parse_line_expression("y = foo(_+2, exp(_))");
    e->accept(visitor.get());
    }
}
*/

TEST(FlopVisitor, basic) {
    {
    auto visitor = make_unique<FlopVisitor>();
    auto e = parse_expression("x+y");
    e->accept(visitor.get());
    EXPECT_EQ(visitor->flops.add, 1);
    }

    {
    auto visitor = make_unique<FlopVisitor>();
    auto e = parse_expression("x-y");
    e->accept(visitor.get());
    EXPECT_EQ(visitor->flops.sub, 1);
    }

    {
    auto visitor = make_unique<FlopVisitor>();
    auto e = parse_expression("x*y");
    e->accept(visitor.get());
    EXPECT_EQ(visitor->flops.mul, 1);
    }

    {
    auto visitor = make_unique<FlopVisitor>();
    auto e = parse_expression("x/y");
    e->accept(visitor.get());
    EXPECT_EQ(visitor->flops.div, 1);
    }

    {
    auto visitor = make_unique<FlopVisitor>();
    auto e = parse_expression("exp(x)");
    e->accept(visitor.get());
    EXPECT_EQ(visitor->flops.exp, 1);
    }

    {
    auto visitor = make_unique<FlopVisitor>();
    auto e = parse_expression("log(x)");
    e->accept(visitor.get());
    EXPECT_EQ(visitor->flops.log, 1);
    }

    {
    auto visitor = make_unique<FlopVisitor>();
    auto e = parse_expression("cos(x)");
    e->accept(visitor.get());
    EXPECT_EQ(visitor->flops.cos, 1);
    }

    {
    auto visitor = make_unique<FlopVisitor>();
    auto e = parse_expression("sin(x)");
    e->accept(visitor.get());
    EXPECT_EQ(visitor->flops.sin, 1);
    }
}

TEST(FlopVisitor, compound) {
    {
        auto visitor = make_unique<FlopVisitor>();
    auto e = parse_expression("x+y*z/a-b");
    e->accept(visitor.get());
    EXPECT_EQ(visitor->flops.add, 1);
    EXPECT_EQ(visitor->flops.sub, 1);
    EXPECT_EQ(visitor->flops.mul, 1);
    EXPECT_EQ(visitor->flops.div, 1);
    }

    {
        auto visitor = make_unique<FlopVisitor>();
    auto e = parse_expression("exp(x+y+z)");
    e->accept(visitor.get());
    EXPECT_EQ(visitor->flops.add, 2);
    EXPECT_EQ(visitor->flops.exp, 1);
    }

    {
        auto visitor = make_unique<FlopVisitor>();
    auto e = parse_expression("exp(x+y) + 3/(12 + z)");
    e->accept(visitor.get());
    EXPECT_EQ(visitor->flops.add, 3);
    EXPECT_EQ(visitor->flops.div, 1);
    EXPECT_EQ(visitor->flops.exp, 1);
    }

    // test asssignment expression
    {
        auto visitor = make_unique<FlopVisitor>();
    auto e = parse_line_expression("x = exp(x+y) + 3/(12 + z)");
    e->accept(visitor.get());
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
    auto visitor = make_unique<FlopVisitor>();
    auto e = parse_procedure(expression);
    e->accept(visitor.get());
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
    auto visitor = make_unique<FlopVisitor>();
    auto e = parse_function(expression);
    e->accept(visitor.get());
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
    Scope<Symbol>::symbol_map globals;
    globals["x"] = make_symbol<Symbol>(Location(), "x", k_symbol_local);
    globals["y"] = make_symbol<Symbol>(Location(), "y", k_symbol_local);
    globals["z"] = make_symbol<Symbol>(Location(), "z", k_symbol_local);
    auto x = globals["x"].get();

    auto scope = std::make_shared<Scope<Symbol>>(globals);

    for(auto const& expression : expressions) {
        auto e = parse_expression(expression);

        // sanity check the compiler
        EXPECT_NE(e, nullptr);
        if( e==nullptr ) continue;

        e->semantic(scope);
        auto v = new ExpressionClassifierVisitor(x);
        e->accept(v);
        //std::cout << "expression " << e->to_string() << std::endl;
        //std::cout << "linear     " << v->linear_coefficient()->to_string() << std::endl;
        //std::cout << "constant   " << v->constant_term()->to_string() << std::endl;
        EXPECT_EQ(v->classify(), k_expression_lin);

#ifdef VERBOSE_TEST
        std::cout << "eq    "   << e->to_string()
                  << "\ncoeff " << v->linear_coefficient()->to_string()
                  << "\nconst " << v-> constant_term()->to_string()
                  << "\n----"   << std::endl;
#endif
        delete v;
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
    Scope<Symbol>::symbol_map globals;
    globals["x"] = make_symbol<Symbol>(Location(), "x", k_symbol_local);
    globals["y"] = make_symbol<Symbol>(Location(), "y", k_symbol_local);
    globals["z"] = make_symbol<Symbol>(Location(), "z", k_symbol_local);
    auto scope = std::make_shared<Scope<Symbol>>(globals);
    auto x = globals["x"].get();

    for(auto const& expression : expressions) {
        auto e = parse_expression(expression);

        // sanity check the compiler
        EXPECT_NE(e, nullptr);
        if( e==nullptr ) continue;

        e->semantic(scope);
        auto v = new ExpressionClassifierVisitor(x);
        e->accept(v);
        EXPECT_EQ(v->classify(), k_expression_const);

#ifdef VERBOSE_TEST
        if(e) std::cout << e->to_string() << std::endl;
        if(p.status()==k_compiler_error)
            std::cout << "in " << colorize(expression, kCyan) << "\t" << p.error_message() << std::endl;
#endif
        delete v;
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
    Scope<Symbol>::symbol_map globals;
    globals["x"] = make_symbol<Symbol>(Location(), "x", k_symbol_local);
    globals["y"] = make_symbol<Symbol>(Location(), "y", k_symbol_local);
    globals["z"] = make_symbol<Symbol>(Location(), "z", k_symbol_local);
    auto scope = std::make_shared<Scope<Symbol>>(globals);
    auto x = globals["x"].get();

    auto v = new ExpressionClassifierVisitor(x);
    for(auto const& expression : expressions) {
        auto e = parse_expression(expression);

        // sanity check the compiler
        EXPECT_NE(e, nullptr);
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
    delete v;
}

