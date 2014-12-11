#include "scope.h"

Scope::Scope(symbol_map &s) : global_symbols_(&s) {};

Symbol Scope::add_local_symbol(std::string const& name, Expression* e) {
    // check to see if the symbol already exists
    if( local_symbols_.find(name) == local_symbols_.end() ) {
        return Symbol();
    }

    // add symbol to list
    Symbol s = Symbol(k_local, e);
    local_symbols_[name] = s;

    return s;
}

Symbol Scope::find(std::string const& name) {
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

