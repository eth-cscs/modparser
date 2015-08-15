#pragma once

#include <sstream>

#include "scope.hpp"
#include "visitor.hpp"

class FunctionExpander : public Visitor {

public:
    using scope_type = Scope<Symbol>;
    using call_list = std::list<expression_ptr>;
    FunctionExpander(std::shared_ptr<scope_type> s)
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

    ~FunctionExpander() {}

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

    template< typename F>
    void expand(CallExpression* func, F replacer) {
        // use the source location of the original statement
        auto loc = func->location();

        // make an identifier for the new symbol which will store the result of
        // the function call
        auto id = make_expression<IdentifierExpression>
            (loc, make_unique_local()->name());
        id->semantic(scope_);
        // generate a LOCAL declaration for the variable
        calls_.push_front(
            make_expression<LocalDeclaration>(loc, id->is_identifier()->spelling())
        );

        // make a binary expression which assigns the function to the variable
        auto ass = binary_expression(loc, tok::eq, id->clone(), func->clone());
        ass->semantic(scope_);
        calls_.push_back(std::move(ass));

        // replace the function call in the original expression with the local
        // variable which holds the pre-computed value
        replacer(std::move(id));
    }

    call_list calls_;
    std::shared_ptr<scope_type> scope_;
};

FunctionExpander::call_list expand_function_calls(Expression* e);

