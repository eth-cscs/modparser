#include "test.h"

#include "../src/cprinter.hpp"

using scope_type = Scope<Symbol>;
using symbol_map = scope_type::symbol_map;
using symbol_ptr = Scope<Symbol>::symbol_ptr;

TEST(CPrinter, statement) {
    std::vector<const char*> expressions =
    {
"y=x+3",
"y=y^z",
"y=exp(x/2 + 3)",
    };

    // create a scope that contains the symbols used in the tests
    auto x = symbol_ptr{ new VariableExpression(Location(), "x") };
    auto y = symbol_ptr{ new VariableExpression(Location(), "y") };
    auto z = symbol_ptr{ new VariableExpression(Location(), "z") };
    symbol_map symbols = {
        {"x", std::move(x)},
        {"y", std::move(y)},
        {"z", std::move(z)}
    };
    auto scope = std::make_shared<scope_type>(symbols);

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
    auto minf = symbol_ptr{ new VariableExpression(Location(), "htau") };
    auto hinf = symbol_ptr{ new VariableExpression(Location(), "hinf") };
    auto mtau = symbol_ptr{ new VariableExpression(Location(), "mtau") };
    auto htau = symbol_ptr{ new VariableExpression(Location(), "htau") };
    auto v    = symbol_ptr{ new VariableExpression(Location(), "v") };
    for(auto const& expression : expressions) {
        auto e = symbol_ptr{parse_procedure(expression)->is_symbol()};

        // sanity check the compiler
        EXPECT_NE(e, nullptr);

        if( e==nullptr ) continue;

        symbol_map symbols = {
            {"minf",   std::move(minf)},
            {"hinf",   std::move(hinf)},
            {"mtau",   std::move(mtau)},
            {"htau",   std::move(htau)},
            {"v",      std::move(v)},
            {"trates", std::move(e)}
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

