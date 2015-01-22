#include <iostream>
#include <cmath>

#include "expressionclassifier.h"

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
    coefficient = e->clone();
    //std::cout << "+++ number : " << e->to_string() << "       coeff " << coefficient->to_string() << std::endl;
}

// identifier expresssion
void ExpressionClassifierVisitor::visit(IdentifierExpression *e) {
    // check if symbol of identifier matches the identifier
    //std::cout << "+++ symbol : " << e->to_string();
    if(symbol == e->symbol()) {
        found_symbol = true;
        coefficient = new NumberExpression(Location(), "1");
    }
    else {
        coefficient = e->clone();
    }
    //std::cout << (found_symbol ? " FOUND" : " CONST") << " coeff " << coefficient->to_string() << std::endl;
}

/// unary expresssion
void ExpressionClassifierVisitor::visit(UnaryExpression *e) {
    e->expression()->accept(this);
    if(found_symbol) {
        switch(e->op()) {
            // plus or minus don't change linearity
            case tok_minus :
                coefficient = unary_expression(Location(), e->op(), coefficient);
                return;
            case tok_plus :
                return;
            // one of these applied to the symbol certainly isn't linear
            case tok_exp :
            case tok_cos :
            case tok_sin :
            case tok_log :
                is_linear = false;
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
        coefficient = e;
    }
}

// binary expresssion
// handle all binary expressions with one routine, because the
// pre-order and in-order code is the same for all cases
void ExpressionClassifierVisitor::visit(BinaryExpression *e) {
    bool lhs_contains_symbol = false;
    bool rhs_contains_symbol = false;
    bool lhs_is_linear = true;
    bool rhs_is_linear = true;
    Expression *lhs_coefficient;
    Expression *rhs_coefficient;

    //std::cout << "+++ binary : " << e->to_string() << std::endl;
    // check the lhs
    reset();
    e->lhs()->accept(this);
    lhs_contains_symbol = found_symbol;
    lhs_is_linear       = is_linear;
    lhs_coefficient     = coefficient;
    //std::cout << " LHS " << (found_symbol ? " FOUND" : " CONST") << (is_linear ? " LINEAR\n" : "NONLINEAR\n");
    if(!is_linear) return; // early return if nonlinear

    // check the rhs
    reset();
    e->rhs()->accept(this);
    rhs_contains_symbol = found_symbol;
    rhs_is_linear       = is_linear;
    rhs_coefficient     = coefficient;
    //std::cout << " RHS " << (found_symbol ? " FOUND" : " CONST") << (is_linear ? " LINEAR\n" : "NONLINEAR\n");
    if(!is_linear) return; // early return if nonlinear

    // mark symbol as found if in either lhs or rhs
    found_symbol = rhs_contains_symbol || lhs_contains_symbol;

    if( found_symbol ) {
        // if both lhs and rhs contain symbol check that the binary operator
        // preserves linearity
        // note that we don't have to test for linearity, because we abort early
        // if either lhs or rhs are nonlinear
        if( rhs_contains_symbol && lhs_contains_symbol ) {
            // be careful to get the order of operation right for
            // non-computative operators
            switch(e->op()) {
                // addition and subtraction are valid
                case tok_plus :
                case tok_minus :
                    coefficient = binary_expression(Location(), e->op(), lhs_coefficient, rhs_coefficient);
                    return;
                // multiplying two expressions that depend on symbol is nonlinear
                case tok_times :
                case tok_pow :
                case tok_divide :
                    is_linear = false;
                    return;
                // should this be a compiler error?
                // it doesn't make much sense to have conditional operators in
                // an expression that is being clasified
                case tok_lt     :
                case tok_lte    :
                case tok_gt     :
                case tok_gte    :
                case tok_EQ     :
                default         :
                    is_linear = false;
                    return;
            }
        }
        // special cases :
        //      operator    | invalid symbol location
        //      -------------------------------------
        //      pow         | lhs OR rhs
        //      comparisons | lhs OR rhs
        //      division    | rhs
        else if(rhs_contains_symbol) {
            switch(e->op()) {
                case tok_times  :
                    if( rhs_coefficient->is_number() &&
                        rhs_coefficient->is_number()->value()==1) {
                        coefficient = lhs_coefficient;
                    }
                    else {
                        coefficient = binary_expression(
                                Location(),
                                tok_times,
                                lhs_coefficient,
                                rhs_coefficient);
                    }
                    return;
                case tok_plus :
                    coefficient = rhs_coefficient;
                    return;
                case tok_minus :
                    coefficient = unary_expression(
                            Location(),
                            e->op(),
                            rhs_coefficient);
                    return;
                case tok_pow    :
                case tok_divide :
                case tok_lt     :
                case tok_lte    :
                case tok_gt     :
                case tok_gte    :
                case tok_EQ     :
                    is_linear = false;
                    return;
                default:
                    return;
            }
        }
        else if(lhs_contains_symbol) {
            switch(e->op()) {
                case tok_times  :
                    // check if the lhs is == 1
                    if( lhs_coefficient->is_number() &&
                        lhs_coefficient->is_number()->value()==1) {
                        coefficient = rhs_coefficient;
                    }
                    else {
                        coefficient = binary_expression(
                                Location(),
                                tok_times,
                                lhs_coefficient,
                                rhs_coefficient);
                    }
                    return;
                case tok_plus  :
                    coefficient = lhs_coefficient;
                    return;
                case tok_minus :
                    coefficient = unary_expression(
                            Location(),
                            tok_minus,
                            lhs_coefficient);
                    return;
                case tok_divide:
                    coefficient = binary_expression(
                            Location(),
                            tok_divide,
                            lhs_coefficient,
                            rhs_coefficient);
                    return;
                case tok_pow    :
                case tok_lt     :
                case tok_lte    :
                case tok_gt     :
                case tok_gte    :
                case tok_EQ     :
                    is_linear = false;
                    return;
                default:
                    return;
            }
        }
    }
    // neither lhs or rhs contains symbol
    // continue building the coefficient
    else {
        coefficient = e;
    }
}

void ExpressionClassifierVisitor::visit(CallExpression *e) {
    for(auto& a : e->args()) {
        a->accept(this);
        // we assume that the parameter passed into a function
        // won't be linear
        if(found_symbol) {
            is_linear = false;
            return;
        }
    }
}

