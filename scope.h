#pragma once

#include <cassert>
#include <string>
#include <unordered_map>

// forward declarations
class Expression;

enum symbolKind {
    k_symbol_function,     ///< function call
    k_symbol_procedure,    ///< procedure call
    k_symbol_variable,     ///< variable at module scope
    k_symbol_local,        ///< variable at local scope
    k_symbol_argument,     ///< argument variable
    k_symbol_none,    ///< no symbol kind (placeholder)
};

struct Symbol {
    symbolKind kind;
    Expression* expression;

    Symbol();
    Symbol(symbolKind k, Expression* e);

    std::string to_string() const;
};

static bool operator == (const Symbol& lhs, const Symbol& rhs) {
    return (lhs.kind == rhs.kind) && (lhs.expression == rhs.expression);
}

class Scope {
public:
    using symbol_map = std::unordered_map<std::string, Symbol>;

    Scope(symbol_map& s);
    ~Scope() {};
    Symbol add_local_symbol(
        std::string const& name,
        Expression* e,
        symbolKind kind=k_symbol_local);
    Symbol find(std::string const& name) const;
    std::string to_string() const;

    symbol_map& locals();
    symbol_map* globals();

private:
    symbol_map* global_symbols_=nullptr;
    symbol_map  local_symbols_;
};

std::string to_string(symbolKind k);
