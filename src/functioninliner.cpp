#include <iostream>

#include "error.hpp"
#include "functioninliner.hpp"
#include "util.hpp"
#include "errorvisitor.hpp"

void VariableReplacer::visit(Expression *e) {
    throw compiler_exception(
            "I don't know how to perform function inlining for "
            + e->to_string(), e->location());
}

void VariableReplacer::visit(UnaryExpression *e) {
    auto exp = e->expression()->is_identifier();
    if(exp && exp->spelling()==source_) {
        e->replace_expression(make_expression<IdentifierExpression>(exp->location(), target_));
    }
    else {
        e->expression()->accept(this);
    }
}

void VariableReplacer::visit(BinaryExpression *e) {
    auto lhs = e->lhs()->is_identifier();
    if(lhs && lhs->spelling()==source_) {
        e->replace_lhs(make_expression<IdentifierExpression>(lhs->location(), target_));
    }
    else {
        e->lhs()->accept(this);
    }

    auto rhs = e->rhs()->is_identifier();
    if(rhs && rhs->spelling()==source_) {
        e->replace_rhs(make_expression<IdentifierExpression>(rhs->location(), target_));
    }
    else {
        e->rhs()->accept(this);
    }
}

expression_ptr inline_function_call(Expression* e)
{
    if(auto f=e->is_function_call()) {
        auto func = f->function();
        auto& body = func->body()->body();
        if(body.size() != 1) {
            throw compiler_exception("I only know how to inline functions with "
                                     "1 statement", func->location());
        }
        // assume that the function body is correctly formed, with the last
        // statement being an assignment expression
        auto last = body.front()->is_assignment();
        auto new_e = last->rhs()->clone();

        auto& fargs = func->args(); // argument names for the function
        auto& cargs = f->args();    // arguments at the call site
        for(int i=0; i<fargs.size(); ++i) {
            if(auto id = cargs[i]->is_identifier()) {
                auto v = make_unique<VariableReplacer>
                            (fargs[i]->is_argument()->spelling(),
                             id->spelling());
                new_e->accept(v.get());
            }
            else {
                throw compiler_exception("can't inline functions which don't "
                                         "take identifiers as arguments",
                                         e->location());
            }
        }
        new_e->semantic(e->scope());

        auto v = make_unique<ErrorVisitor>("");
        new_e->accept(v.get());
        if(v->num_errors()) {
            throw compiler_exception("something went wrong with inlined function call ",
                                     e->location());
        }

        return std::move(new_e);
    }

    return {};
}


