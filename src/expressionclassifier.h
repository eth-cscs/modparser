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
        const_folder_ = new ConstantFolderVisitor();
    }

    void reset(Symbol const& s) {
        reset();
        symbol = s;
    }

    void reset() {
        is_linear_    = true;
        found_symbol_ = false;
        coefficient_  = nullptr;
        constant_     = nullptr;
    }

    void visit(Expression *e)           override;
    void visit(UnaryExpression *e)      override;
    void visit(BinaryExpression *e)     override;
    void visit(NumberExpression *e)     override;
    void visit(IdentifierExpression *e) override;
    void visit(CallExpression *e)       override;

    expressionClassification classify() const {
        if(!found_symbol_) {
            return k_expression_const;
        }
        if(is_linear_) {
            return k_expression_lin;
        }
        return k_expression_nonlin;
    }

    Expression *linear_coefficient() {
        // if is a linear expression with nonzero linear coefficient
        if(classify() == k_expression_lin) {
            coefficient_->accept(const_folder_);
            if(const_folder_->is_number)
                return new NumberExpression(
                        Location(),
                        const_folder_->value);
            return coefficient_;
        }
        // constant expression
        else if(classify() == k_expression_const) {
            return new NumberExpression(Location(), 0.);
        }
        // nonlinear expression
        else {
            return nullptr;
        }
    }

    Expression *constant_term() {
        // if is a linear expression with nonzero linear coefficient
        if(classify() == k_expression_lin) {
            //return constant_;
            return constant_ ? constant_ : new NumberExpression(Location(), 0.);
        }
        // constant expression
        else if(classify() == k_expression_const) {
            return coefficient_;
            //return coefficient_ ? coefficient_ : new NumberExpression(Location(), 0.);
        }
        // nonlinear expression
        else {
            return nullptr;
        }
    }

    ~ExpressionClassifierVisitor() {
        delete const_folder_;
    }

private:
    // assume linear until otherwise proven
    bool is_linear_     = true;
    bool found_symbol_  = false;
    Expression* coefficient_   = nullptr;
    Expression* constant_  = nullptr;
    Symbol symbol;
    ConstantFolderVisitor* const_folder_;
};

