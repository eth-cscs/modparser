#include "method.h"

std::list<Symbol>&
APIMethod::lvalues() {
    return lvalues_;
}
std::list<Symbol> const&
APIMethod::lvalues() const {
    return rvalues_;
}

std::list<Symbol>&
APIMethod::rvalues() {
    return lvalues_;
}
std::list<Symbol> const&
APIMethod::rvalues() const {
    return rvalues_;
}

APIMethod::APIMethod(ProcedureExpression* p)
:   body_(p->body),
    name_(p->name),
    args_(p->args),
    kind_(k_proc)
{
    std::set<Symbol> lvals;

    // remove local variable declarations
    // tests whether an expression declares a local variable
    auto local_test = [] (Expression* e) {
        return e->is_local_declaration() != nullptr;
    }
    body_->body().remove_if(local_test);

    // search for all variables that are assigned to
    for(auto e : body_) {
        auto lhs = e->is_assignment();
        if(lhs == nullptr) continue;

        // expression on LHS of an assignment has to be symbol by definition
        lvals.insert(a->is_identifier()->symbol());
    }

    for(auto s: lvals) {
        lvalues_.push_back(s);
    }

    // remove unused local variables
    auto local_unused = [&lvals] (Symbol_& s) {
        return lvals.find(s)==lvalues.end();
    };
    args_.remove_if(local_unused);

    // TODO inline procedures
    // 1. iterate over statements looking for procedures
    // 2. if procedure :
    //      - insert local variables with renaming if needed
    //      - when renaming the variable symbols must be updated
    //      - 
}


