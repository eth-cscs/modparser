#pragma once

#include <mutex>

#include "constantfolder.hpp"
#include "scope.hpp"
#include "visitor.hpp"

enum expressionClassification {
    k_expression_const,
    k_expression_lin,
    k_expression_nonlin
};

class ExpressionClassifierVisitor : public Visitor {
public:
    ExpressionClassifierVisitor(Symbol *s)
    : symbol_(s)
    {
        const_folder_ = new ConstantFolderVisitor();
    }

    void reset(Symbol* s) {
        reset();
        symbol_ = s;
    }

    void reset() {
        is_linear_    = true;
        found_symbol_ = false;
        configured_   = false;
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
        set();
        return coefficient_.get();
    }

    Expression *constant_term() {
        set();
        return constant_.get();
    }

    ~ExpressionClassifierVisitor() {
        delete const_folder_;
    }

private:

    void set() const {
        // a mutex is required because two threads might attempt to update
        // the cached constant_/coefficient_ values, which would violate the
        // assertion that set() is const
        std::lock_guard<std::mutex> g(mutex_);

        // update the constant_ and coefficient_ terms if they have not already
        // been set
        if(!configured_) {
            if(classify() == k_expression_lin) {
                // if constat_ was never set, it must be zero
                if(!constant_) {
                    constant_ =
                        make_expression<NumberExpression>(Location(), 0.);
                }
                // perform constant folding on the coefficient term
                coefficient_->accept(const_folder_);
                if(const_folder_->is_number) {
                    // if the folding resulted in a constant, reset coefficient
                    // to be a NumberExpression
                    coefficient_.reset(new NumberExpression(
                                            Location(),
                                            const_folder_->value)
                                      );
                }
            }
            else if(classify() == k_expression_const) {
                coefficient_.reset(new NumberExpression(
                                        Location(),
                                        0.)
                                  );
            }
            else { // nonlinear expression
                coefficient_ = nullptr;
                constant_    = nullptr;
            }
            configured_ = true;
        }
    }

    // assume linear until otherwise proven
    bool is_linear_     = true;
    bool found_symbol_  = false;
    mutable bool configured_    = false;
    mutable expression_ptr coefficient_;
    mutable expression_ptr constant_;
    Symbol* symbol_;
    ConstantFolderVisitor* const_folder_;

    mutable std::mutex mutex_;

};

