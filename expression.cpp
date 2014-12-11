#include "expression.h"

void IdentifierExpression::semantic(Scope* scp) {
    scope_ = scp;

    Symbol s = scope_->find(name_);

    if(s.expression==0) {
        error_ = true;
        error_string_ =
            pprintf("the variable '%' is undefined",
                    colorize(name_, kYellow), location_);
        return;
    }
    if(s.kind == k_procedure || s.kind == k_function) {
        error_ = true;
        error_string_ =
            pprintf("the symbol '%' is a function/procedure, not a variable",
                    colorize(name_, kYellow));
        return;
    }

    // save the symbol
    symbol_ = s;
}

void LocalExpression::semantic(Scope* scp) {
    scope_ = scp;

    Symbol s = scope_->find(name_);

    // First check that the variable is undefined
    // Note that we allow for local variables with the same name as
    // class scope variables (globals), in which case the local variable
    // name will be used for lookup
    if(s.expression==0 || (s.expression && s.kind==k_variable)) {
        symbol_ = scope_->add_local_symbol(name_, this);
    }
    else {
        error_ = true;
        error_string_ =
            pprintf("the symbol '%' is already defined",
                    colorize(name_, kYellow));
        return;
    }
}

std::string VariableExpression::to_string() const {
    char name[17];
    snprintf(name, 17, "%-10s", name_.c_str());
    std::string
        s = colorize("variable",kBlue) + " " + colorize(name, kYellow) + "("
          + colorize("write", is_writeable() ? kGreen : kRed) + ", "
          + colorize("read", is_readable() ? kGreen : kRed)   + ", "
          + (is_range() ? "range" : "scalar")                 + ", "
          + "ion" + colorize(::to_string(ion_channel()), ion_channel()==k_ion_none ? kRed : kGreen)+ ", "
          + "vis "  + ::to_string(visibility()) + ", "
          + "link " + ::to_string(linkage())    + ", "
          + colorize("state", is_state() ? kGreen : kRed) + ")";
    return s;
}

std::string CallExpression::to_string() const {
    std::string str = colorize("call", kBlue) + " " + colorize(name_, kYellow) + " (";
    for(auto arg : args_)
        str += arg->to_string() + ", ";
    str += ")";

    return str;
}

void CallExpression::semantic(Scope* scp) {
    scope_ = scp;

    // look up to see if symbol is defined
    Symbol s = scope_->find(name_);

    // either undefined or refers to a variable
    if(s.expression==0) {
        error_ = true;
        error_string_ = 
            pprintf("there is no function or procedure named '%' ",
                    colorize(name_, kYellow));
    }
    if(s.kind==k_local || s.kind==k_variable) {
        error_ = true;
        error_string_ = 
            pprintf("the symbol '%' refers to a variable, but it is being called like a function",
                    colorize(name_, kYellow));
    }

    // save the symbol
    symbol_ = s;

    // TODO: check that the number of arguments matches
    if( !error_ ) {
        // only analyze if the call was found
    }

    // perform semantic analysis on the arguments
    for(auto a : args_) {
        a->semantic(scp);
    }
}

std::string ProcedureExpression::to_string() const {
    std::string str = colorize("procedure", kBlue) + " " + colorize(name_, kYellow) + "\n";
    str += colorize("  args",kBlue) + " : ";
    for(auto arg : args_)
        str += arg->to_string() + " ";
    str += "\n  "+colorize("body", kBlue)+" :";
    for(auto ex : body_)
        str += "\n    " + ex->to_string();

    return str;
}

std::string FunctionExpression::to_string() const {
    std::string str = colorize("function", kBlue) + " " + colorize(name_, kYellow) + "\n";
    str += colorize("  args",kBlue) + " : ";
    for(auto arg : args_)
        str += arg->to_string() + " ";
    str += "\n  "+colorize("body", kBlue)+" :";
    for(auto ex : body_)
        str += "\n    " + ex->to_string();

    return str;
}

void ProcedureExpression::semantic(Scope::symbol_map &global_symbols) {
    // assert that the symbol is already visible in the global_symbols
    assert(global_symbols.find(name_) != global_symbols.end());

    // create the scope for this procedure
    scope_ = new Scope(global_symbols);

    // add the argumemts tp the list of local variables
    for(auto a : args_) {
        a->semantic(scope_);
    }

    // perform semantic analysis for each expression in the body
    for(auto e : body_) {
        e->semantic(scope_);
    }

    // the symbol for this expression is itself
    // this could lead to nasty self-referencing loops
    symbol_ = global_symbols.find(name_)->second;
}

void FunctionExpression::semantic(Scope::symbol_map &global_symbols) {
    // assert that the symbol is already visible in the global_symbols
    assert(global_symbols.find(name_) != global_symbols.end());

    // create the scope for this procedure
    scope_ = new Scope(global_symbols);

    // add the argumemts tp the list of local variables
    for(auto a : args_) {
        a->semantic(scope_);
    }

    // perform semantic analysis for each expression in the body
    for(auto e : body_) {
        e->semantic(scope_);
    }

    // the symbol for this expression is itself
    // this could lead to nasty self-referencing loops
    symbol_ = global_symbols.find(name_)->second;
}

void UnaryExpression::semantic(Scope* scp) {
    e_->semantic(scp);
}

void BinaryExpression::semantic(Scope* scp) {
    lhs_->semantic(scp);
    rhs_->semantic(scp);
}

void AssignmentExpression::semantic(Scope* scp) {
    lhs_->semantic(scp);
    rhs_->semantic(scp);

    if(lhs_->is_function_call()) {
        error_ = true;
        error_string_ =
            "the left hand side of an assignment can't be a function or procedure call";
    }
    if(rhs_->is_procedure_call() || lhs_->is_procedure_call()) {
        error_ = true;
        error_string_ = "a procedure call can't be part of an expression";
    }
}

