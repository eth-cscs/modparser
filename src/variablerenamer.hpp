#pragma once

#include "visitor.h"

class VariableRenamer : public Visitor {

public:
    VariableRenamer(std::string from, std::string to)
    :   from_(from),
        to_(to)
    {}

    void visit(Expression *e)           override;
    void visit(UnaryExpression *e)      override;
    void visit(BinaryExpression *e)     override;
    void visit(NumberExpression *e)     override;

    void visit(IdentifierExpression *e) override;
    void visit(CallExpression *e)       override;
    void visit(BlockExpression *e)      override;
    void visit(IfExpression *e)         override;

    void reset(std::string const& from, std::string const& to);
private:

    std::string from_;
    std::string to_;
};

