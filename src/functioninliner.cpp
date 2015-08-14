#include <iostream>

#include "error.hpp"
#include "functioninliner.hpp"
#include "util.hpp"

void FunctionInliner::visit(Expression *e) {
    throw compiler_exception(
            "I don't know how to perform function inlining for "
            + e->to_string(), e->location());
}

void FunctionInliner::visit(UnaryExpression *e) {
    if(auto func = e->expression()->is_function_call()) {
        auto loc = e->location();
        auto id = make_expression<IdentifierExpression>
            (loc, make_unique_local()->name());
        auto ass = binary_expression(loc, tok::eq, id->clone(), func->clone());
        ass->semantic(scope_);
        e->replace_expression(std::move(id));
        e->semantic(scope_);
        calls_.push_back(std::move(ass));
    }
    else {
        e->expression()->accept(this);
    }
}

void FunctionInliner::visit(BinaryExpression *e) {
    if(auto func = e->lhs()->is_function_call()) {
        auto loc = e->lhs()->location();
        auto id = make_expression<IdentifierExpression>
            (loc, make_unique_local()->name());
        auto ass = binary_expression(loc, tok::eq, id->clone(), func->clone());
        ass->semantic(scope_);
        e->replace_lhs(std::move(id));
        e->semantic(scope_);
        calls_.push_back(std::move(ass));
    }
    else {
        e->lhs()->accept(this);
    }

    if(auto func = e->rhs()->is_function_call()) {
        auto loc = e->rhs()->location();
        auto id = make_expression<IdentifierExpression>
            (loc, make_unique_local()->name());
        auto ass = binary_expression(loc, tok::eq, id->clone(), func->clone());
        ass->semantic(scope_);
        e->replace_rhs(std::move(id));
        e->semantic(scope_);
        calls_.push_back(std::move(ass));
    }
    else {
        e->rhs()->accept(this);
    }
}

FunctionInliner::call_list expand_function_calls(Expression* e)
{
    if(auto a=e->is_assignment()) {
        if(a->rhs()->is_function_call()) {
            return {};
        }
        auto v = make_unique<FunctionInliner>(e->scope());
        a->rhs()->accept(v.get());
        return v->calls();
    }

    // todo: handle procedure calls where arguments might be function calls
    return {};
}

