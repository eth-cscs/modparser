#pragma once

#include "expression.h"

class APIMethod : public ProcedureExpression {

    std::list<Symbol> lvalues_;
    std::list<Symbol> rvalues_;

public:

    void accept(Visitor *v):

    APIMethod(ProcedureExpression* p);

    // are these good names?
    std::list<Symbol>& lvalues();
    std::list<Symbol>& rvalues();
    std::list<Symbol> const& lvalues() const;
    std::list<Symbol> const& rvalues() const;
};

