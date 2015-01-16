#pragma once

#include "visitor.h"
#include "scope.h"

enum expressionClassification {k_expression_const, k_expression_lin, k_expression_nonlin};

class ExpressionClassifierVisitor : public Visitor {
public:
    ExpressionClassifierVisitor(Symbol const& s)
    : symbol(s)
    {}

    void reset(Symbol const& s) {
        reset();
        symbol = s;
    }

    void reset() {
        is_linear    = true;
        found_symbol = false;
        coefficient  = nullptr;
    }

    void visit(Expression *e)           override;
    void visit(UnaryExpression *e)      override;
    void visit(BinaryExpression *e)     override;
    void visit(NumberExpression *e)     override;
    void visit(IdentifierExpression *e) override;
    void visit(CallExpression *e)       override;

    expressionClassification classify() const {
        if(!found_symbol) {
            return k_expression_const;
        }
        if(is_linear) {
            return k_expression_lin;
        }
        return k_expression_nonlin;
    }

    Expression *linear_coefficient() {
        return coefficient;
    }

private:


    bool        is_linear     = true; // assume linear until otherwise proven
    bool        found_symbol  = false;
    Expression* coefficient   = nullptr;
    Symbol symbol;

};

