#pragma once

#include <sstream>

#include "scope.hpp"
#include "visitor.hpp"

class FunctionInliner : public Visitor {

public:
    using scope_type = Scope<Symbol>;
    using call_list = std::list<expression_ptr>;
    FunctionInliner(std::shared_ptr<scope_type> s)
    :   scope_(s)
    {}

    void visit(Expression *e)           override;
    void visit(UnaryExpression *e)      override;
    void visit(BinaryExpression *e)     override;
    void visit(NumberExpression *e)     override {};
    void visit(IdentifierExpression *e) override {};

    call_list calls() {
        return std::move(calls_);
    }

    ~FunctionInliner() {}

private:
    Symbol* make_unique_local() {
        std::string name;
        auto i = 0;
        do {
            name = pprintf("l_%", i);
            ++i;
        } while(scope_->find(name));

        auto sym = scope_->add_local_symbol(
                name,
                make_symbol<LocalVariable>(Location(), name, localVariableKind::local)
              );

        return sym;
    }

    call_list calls_;
    std::shared_ptr<scope_type> scope_;
};

FunctionInliner::call_list expand_function_calls(Expression* e);

