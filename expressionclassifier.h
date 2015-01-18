#pragma once

#include "constantfolder.h"
#include "scope.h"
#include "visitor.h"

enum expressionClassification {k_expression_const, k_expression_lin, k_expression_nonlin};

class ExpressionClassifierVisitor : public Visitor {
public:
    ExpressionClassifierVisitor(Symbol const& s)
    : symbol(s)
    {
        const_folder = new ConstantFolderVisitor();
    }

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
        coefficient->accept(const_folder);
        if(const_folder->is_number)
            return new NumberExpression(
                    Location(),
                    const_folder->value);
        return coefficient;
    }

    ~ExpressionClassifierVisitor() {
        delete const_folder;
    }

private:
    // assume linear until otherwise proven
    bool is_linear     = true;
    bool found_symbol  = false;
    Expression* coefficient   = nullptr;
    Symbol symbol;
    ConstantFolderVisitor* const_folder;
};

