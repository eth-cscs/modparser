#include <iostream>

#include "error.hpp"
#include "functionexpander.hpp"
#include "util.hpp"

void FunctionExpander::visit(Expression *e) {
    throw compiler_exception(
            "I don't know how to perform function inlining for "
            + e->to_string(), e->location());
}

void FunctionExpander::visit(UnaryExpression *e) {
    if(auto func = e->expression()->is_function_call()) {
        expand(func, [&e](expression_ptr&& p){e->replace_expression(std::move(p));});
        e->semantic(scope_);
    }
    else {
        e->expression()->accept(this);
    }
}

void FunctionExpander::visit(BinaryExpression *e) {
    if(auto func = e->lhs()->is_function_call()) {
        expand(func, [&e](expression_ptr&& p){e->replace_lhs(std::move(p));});
        e->semantic(scope_);
    }
    else {
        e->lhs()->accept(this);
    }

    if(auto func = e->rhs()->is_function_call()) {
        expand(func, [&e](expression_ptr&& p){e->replace_rhs(std::move(p));});
        e->semantic(scope_);
    }
    else {
        e->rhs()->accept(this);
    }
}

FunctionExpander::call_list expand_function_calls(Expression* e)
{
    if(auto a=e->is_assignment()) {
        if(a->rhs()->is_function_call()) {
            return {};
        }
        auto v = make_unique<FunctionExpander>(e->scope());
        a->rhs()->accept(v.get());
        return v->calls();
    }

    // todo: handle procedure calls where arguments might be function calls
    return {};
}

