#include "scope.h"
#include "util.h"

std::string to_string(symbolKind k) {
    switch (k) {
        case k_symbol_variable:
            return std::string("variable");
        case k_symbol_indexed_variable:
            return std::string("indexed variable");
        case k_symbol_local:
            return std::string("local");
        case k_symbol_argument:
            return std::string("argument");
        case k_symbol_procedure:
            return std::string("procedure");
        case k_symbol_function:
            return std::string("function");
        case k_symbol_none:
            return std::string("none");
    }
}

Symbol::Symbol()
    : kind(k_symbol_none), expression(nullptr)
{}

Symbol::Symbol(symbolKind k, Expression* e)
    : kind(k), expression(e)
{
    // should never have a non-null pointer and invalid symbol
    assert(!(k==k_symbol_none && e));
}

std::string Symbol::to_string() const {
    std::string s = blue("symbol") + " ";
    s += "(" + green(::to_string(kind)) + ")";
    return s;
}

Scope::Scope(symbol_map &s)
    : global_symbols_(&s)
{}

Symbol Scope::add_local_symbol( std::string const& name,
                                Expression* e,
                                symbolKind kind)
{
    // check to see if the symbol already exists
    if( local_symbols_.find(name) != local_symbols_.end() ) {
        return Symbol();
    }

    // add symbol to list
    Symbol s = Symbol(kind, e);
    local_symbols_[name] = s;

    return s;
}

Symbol Scope::find(std::string const& name) const {
    // search in local symbols
    auto local = local_symbols_.find(name);

    if(local != local_symbols_.end()) {
        return local->second;
    }

    // search in global symbols
    if( global_symbols_ ) {
        auto global = global_symbols_->find(name);

        if(global != global_symbols_->end()) {
            return global->second;
        }
    }

    // the symbol was not found
    // return a symbol with nullptr for expression
    return Symbol();
}

std::string Scope::to_string() const {
    std::string s;
    char buffer[16];

    s += blue("Scope") + "\n";
    s += blue("  global :\n");
    for(auto sym : *global_symbols_) {
        snprintf(buffer, 16, "%-15s", sym.first.c_str());
        s += "    " + yellow(buffer) + " " + sym.second.to_string() + "\n";
    }
    s += blue("  local  :\n");
    for(auto sym : local_symbols_) {
        snprintf(buffer, 16, "%-15s", sym.first.c_str());
        s += "    " + yellow(buffer) + " " + sym.second.to_string() + "\n";
    }

    return s;
}

Scope::symbol_map& Scope::locals() {
    return local_symbols_;
}

Scope::symbol_map* Scope::globals() {
    return global_symbols_;
}

