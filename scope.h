#pragma once

#include <cassert>
#include <string>
#include <unordered_map>

// forward declarations
class Expression;
class LocalExpression;

enum symbolKind {
    k_function,     ///< function call
    k_procedure,    ///< procedure call
    k_variable,     ///< variable at class scope
    k_local,        ///< variable at local scope
    k_no_symbol,    ///< no symbol kind (placeholder)
};

struct Symbol {
    symbolKind kind;
    Expression* expression;

    Symbol()
        : kind(k_no_symbol), expression(nullptr)
    {}

    Symbol(symbolKind k, Expression* e)
        : kind(k), expression(e)
    {
        // we never want a non-null pointer and invalide symbol
        assert(!(k==k_no_symbol && e));
    }
};

class Scope {
public:
    using symbol_map = std::unordered_map<std::string, Symbol>;

    Scope(symbol_map& s);
    Symbol add_local_symbol(std::string const& name, Expression* e);
    Symbol find(std::string const& name);

private:
    symbol_map* global_symbols_=nullptr;
    symbol_map  local_symbols_;
};

