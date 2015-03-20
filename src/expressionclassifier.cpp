#include <iostream>
#include <cmath>

#include "expressionclassifier.hpp"

// this turns out to be quite easy, however quite fiddly to do right.

// default is to do nothing and return
void ExpressionClassifierVisitor::visit(Expression *e) {
    std::cout
        << red("compiler error: ") << white(pprintf("%", e->location()))
        << " attempting to apply linear analysis on "
        << e->to_string()
        << std::endl;
    assert(false);
}

// number expresssion
void ExpressionClassifierVisitor::visit(NumberExpression *e) {
    // save the coefficient as the number
    coefficient_ = e->clone();
    //std::cout << "leaf      " << e->to_string() << std::endl;
}

// identifier expresssion
void ExpressionClassifierVisitor::visit(IdentifierExpression *e) {
    // check if symbol of identifier matches the identifier
    if(symbol_ == e->symbol()) {
        found_symbol_ = true;
        coefficient_.reset(new NumberExpression(Location(), "1"));
    }
    else {
        coefficient_ = e->clone();
    }
    //std::cout << "leaf      " << e->to_string() << std::endl;
}

/// unary expresssion
void ExpressionClassifierVisitor::visit(UnaryExpression *e) {
    //std::cout << "unary     " << e->to_string() << std::endl;
    e->expression()->accept(this);
    if(found_symbol_) {
        switch(e->op()) {
            // plus or minus don't change linearity
            case tok_minus :
                coefficient_ = unary_expression(Location(), e->op(), std::move(coefficient_));
                return;
            case tok_plus :
                return;
            // one of these applied to the symbol certainly isn't linear
            case tok_exp :
            case tok_cos :
            case tok_sin :
            case tok_log :
                is_linear_ = false;
                return;
            default :
                std::cout
                    << red("compiler error: ")
                    << white(pprintf("%", e->location()))
                    << " attempted to find linear expression for an unsupported UnaryExpression "
                    << yellow(token_string(e->op())) << std::endl;
                assert(false);
        }
    }
    else {
        coefficient_ = e->clone();
    }
}

// binary expresssion
// handle all binary expressions with one routine, because the
// pre-order and in-order code is the same for all cases
void ExpressionClassifierVisitor::visit(BinaryExpression *e) {
    //std::cout << "binary    " << e->to_string() << std::endl;
    bool lhs_contains_symbol = false;
    bool rhs_contains_symbol = false;
    expression_ptr lhs_coefficient;
    expression_ptr rhs_coefficient;
    expression_ptr lhs_constant;
    expression_ptr rhs_constant;

    // check the lhs
    reset();
    e->lhs()->accept(this);
    lhs_contains_symbol = found_symbol_;
    lhs_coefficient     = std::move(coefficient_);
    lhs_constant        = std::move(constant_);
    if(!is_linear_) return; // early return if nonlinear
    //if(lhs_coefficient) std::cout << "  lhs coeff : " << lhs_coefficient->to_string() << std::endl;
    //if(lhs_constant)    std::cout << "  lhs const : " << lhs_constant->to_string() << std::endl;

    // check the rhs
    reset();
    e->rhs()->accept(this);
    rhs_contains_symbol = found_symbol_;
    rhs_coefficient     = std::move(coefficient_);
    rhs_constant        = std::move(constant_);
    if(!is_linear_) return; // early return if nonlinear
    //if(rhs_coefficient) std::cout << "  rhs coeff : " << rhs_coefficient->to_string() << std::endl;
    //if(rhs_constant)    std::cout << "  rhs const : " << rhs_constant->to_string() << std::endl;

    // mark symbol as found if in either lhs or rhs
    found_symbol_ = rhs_contains_symbol || lhs_contains_symbol;

    //std::cout << "  " << (found_symbol_ ? "found x" : "not found x") << std::endl;

    if( found_symbol_ ) {
        // if both lhs and rhs contain symbol check that the binary operator
        // preserves linearity
        // note that we don't have to test for linearity, because we abort early
        // if either lhs or rhs are nonlinear
        if( rhs_contains_symbol && lhs_contains_symbol ) {
            //std::cout << "  on both sides " << e->to_string() << std::endl;
            // be careful to get the order of operation right for
            // non-computative operators
            switch(e->op()) {
                // addition and subtraction are valid, nothing else is
                case tok_plus :
                case tok_minus :
                    coefficient_ =
                        binary_expression(Location(),
                                          e->op(),
                                          std::move(lhs_coefficient),
                                          std::move(rhs_coefficient));
                    return;
                // multiplying two expressions that depend on symbol is nonlinear
                case tok_times :
                case tok_pow   :
                case tok_divide :
                default         :
                    is_linear_ = false;
                    return;
            }
        }
        // special cases :
        //      operator    | invalid symbol location
        //      -------------------------------------
        //      pow         | lhs OR rhs
        //      comparisons | lhs OR rhs
        //      division    | rhs
        ////////////////////////////////////////////////////////////////////////
        // only RHS contains the symbol
        ////////////////////////////////////////////////////////////////////////
        else if(rhs_contains_symbol) {
            //std::cout << "  on rhs " << e->to_string() << std::endl;
            switch(e->op()) {
                case tok_times  :
                    //std::cout << "    times" << std::endl;
                    // determine the linear coefficient
                    if( rhs_coefficient->is_number() &&
                        rhs_coefficient->is_number()->value()==1) {
                        //std::cout << "    uh oh..." << std::endl;
                        coefficient_ = lhs_coefficient->clone();
                    }
                    else {
                        coefficient_ =
                            binary_expression(Location(),
                                              tok_times,
                                              lhs_coefficient->clone(),
                                              rhs_coefficient->clone());
                    }
                    // determine the constant
                    if(rhs_constant) {
                        constant_ =
                            binary_expression(Location(),
                                              tok_times,
                                              std::move(lhs_coefficient),
                                              std::move(rhs_constant));
                    } else {
                        constant_ = nullptr;
                    }
                    return;
                case tok_plus :
                    // constant term
                    if(lhs_constant && rhs_constant) {
                        constant_ =
                            binary_expression(Location(),
                                              tok_plus,
                                              std::move(lhs_constant),
                                              std::move(rhs_constant));
                    }
                    else if(rhs_constant) {
                        constant_ = binary_expression(Location(),
                                                      tok_plus,
                                                      std::move(lhs_coefficient),
                                                      std::move(rhs_constant));
                    }
                    else {
                        constant_ = std::move(lhs_coefficient);
                    }
                    // coefficient
                    coefficient_ = std::move(rhs_coefficient);
                    return;
                case tok_minus :
                    // constant term
                    if(lhs_constant && rhs_constant) {
                        constant_ = binary_expression(Location(),
                                                      tok_minus,
                                                      std::move(lhs_constant),
                                                      std::move(rhs_constant));
                    }
                    else if(rhs_constant) {
                        constant_ = binary_expression(Location(),
                                                      tok_minus,
                                                      std::move(lhs_coefficient),
                                                      std::move(rhs_constant));
                    }
                    else {
                        constant_ = std::move(lhs_coefficient);
                    }
                    // coefficient
                    coefficient_ = unary_expression(Location(),
                                                    e->op(),
                                                    std::move(rhs_coefficient));
                    return;
                case tok_pow    :
                case tok_divide :
                case tok_lt     :
                case tok_lte    :
                case tok_gt     :
                case tok_gte    :
                case tok_EQ     :
                    is_linear_ = false;
                    return;
                default:
                    return;
            }
        }
        ////////////////////////////////////////////////////////////////////////
        // only LHS contains the symbol
        ////////////////////////////////////////////////////////////////////////
        else if(lhs_contains_symbol) {
            //std::cout << "  on lhs " << e->to_string() << std::endl;
            switch(e->op()) {
                case tok_times  :
                    // check if the lhs is == 1
                    if( lhs_coefficient->is_number() &&
                        lhs_coefficient->is_number()->value()==1) {
                        coefficient_ = rhs_coefficient->clone();
                    }
                    else {
                        coefficient_ =
                            binary_expression(Location(),
                                              tok_times,
                                              std::move(lhs_coefficient),
                                              std::move(rhs_coefficient));
                    }
                    // constant term
                    if(lhs_constant) {
                        constant_ = binary_expression(Location(),
                                                      tok_times,
                                                      std::move(lhs_constant),
                                                      std::move(rhs_coefficient));
                    } else {
                        constant_ = nullptr;
                    }
                    return;
                case tok_plus  :
                    coefficient_ = std::move(lhs_coefficient);
                    // constant term
                    if(lhs_constant && rhs_constant) {
                        constant_ = binary_expression(Location(),
                                                      tok_plus,
                                                      std::move(lhs_constant),
                                                      std::move(rhs_constant));
                    }
                    else if(lhs_constant) {
                        constant_ = binary_expression(Location(),
                                                      tok_plus,
                                                      std::move(lhs_constant),
                                                      std::move(rhs_coefficient));
                    }
                    else {
                        constant_ = std::move(rhs_coefficient);
                    }
                    return;
                case tok_minus :
                    coefficient_ = std::move(lhs_coefficient);
                    // constant term
                    if(lhs_constant && rhs_constant) {
                        constant_ = binary_expression(Location(),
                                                      tok_minus,
                                                      std::move(lhs_constant),
                                                      std::move(rhs_constant));
                    }
                    else if(lhs_constant) {
                        constant_ = binary_expression(Location(),
                                                      tok_minus,
                                                      std::move(lhs_constant),
                                                      std::move(rhs_coefficient));
                    }
                    else {
                        constant_ = unary_expression(Location(),
                                                     tok_minus,
                                                     std::move(rhs_coefficient));
                    }
                    return;
                case tok_divide:
                    coefficient_ = binary_expression(Location(),
                                                     tok_divide,
                                                     std::move(lhs_coefficient),
                                                     rhs_coefficient->clone());
                    if(lhs_constant) {
                        constant_ = binary_expression(Location(),
                                                      tok_divide,
                                                      std::move(lhs_constant),
                                                      std::move(rhs_coefficient));
                    }
                    return;
                case tok_pow    :
                case tok_lt     :
                case tok_lte    :
                case tok_gt     :
                case tok_gte    :
                case tok_EQ     :
                    is_linear_ = false;
                    return;
                default:
                    return;
            }
        }
    }
    // neither lhs or rhs contains symbol
    // continue building the coefficient
    else {
        coefficient_ = e->clone();
    }
}

void ExpressionClassifierVisitor::visit(CallExpression *e) {
    for(auto& a : e->args()) {
        a->accept(this);
        // we assume that the parameter passed into a function
        // won't be linear
        if(found_symbol_) {
            is_linear_ = false;
            return;
        }
    }
}

