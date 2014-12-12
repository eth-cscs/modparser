#pragma once

#include <iostream>
#include "visitor.h"
#include "expression.h"

class ErrorVisitor : public Visitor {
public:
    ErrorVisitor(std::string const& m)
        : module_name_(m)
    {}

    void visit(Expression *e)           override;
    void visit(ProcedureExpression *e)  override;
    void visit(FunctionExpression *e)   override;
    void visit(UnaryExpression *e)      override;
    void visit(BinaryExpression *e)     override;
    void visit(CallExpression *e)       override;

    int num_errors() {return num_errors_;};
private:
    template <typename ExpressionType>
    void print_error(ExpressionType *e) {
        if(e->has_error()) {
            auto header = red("error: ") + white(pprintf("%:% ", module_name_, e->location()));
            std::cout << header << "\n  "
                      << e->error_message()
                      << std::endl;
            num_errors_++;
        }
    }

    std::string module_name_;
    int num_errors_ = 0;
};

