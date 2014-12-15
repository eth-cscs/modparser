#pragma once

#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>

// forward declarations
class Expression;

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

    Symbol();
    Symbol(symbolKind k, Expression* e);

    std::string to_string() const;
};

class Scope {
public:
    using symbol_map = std::unordered_map<std::string, Symbol>;

    Scope(symbol_map& s);
    Scope() {std::cout << "and here..." << std::endl;};
    ~Scope() {
        std::cout << "Scope::desctructor" << std::endl;
    };
    Symbol add_local_symbol(std::string const& name, Expression* e);
    Symbol find(std::string const& name) const;
    std::string to_string() const;

private:
    symbol_map* global_symbols_=nullptr;
    symbol_map  local_symbols_;
};

std::string to_string(symbolKind k);
