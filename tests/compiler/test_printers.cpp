#include "test.h"

#include "../src/cprinter.hpp"

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
        Expression *e = parse_line_expression(expression);

        // sanity check the compiler
        EXPECT_NE(e, nullptr);
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
        Expression *e = parse_procedure(expression);

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

