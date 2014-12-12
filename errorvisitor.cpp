#include "errorvisitor.h"


void ErrorVisitor::visit(Expression *e) {
    print_error(e);
}

// traverse the statements in a procedure
void ErrorVisitor::visit(ProcedureExpression *e) {
    print_error(e);
    for(auto expression : e->args()) {
        expression->accept(this);
    }

    for(auto expression : e->body()) {
        expression->accept(this);
    }
}

// traverse the statements in a function
void ErrorVisitor::visit(FunctionExpression *e) {
    print_error(e);
    for(auto expression : e->args()) {
        expression->accept(this);
    }

    for(auto expression : e->body()) {
        expression->accept(this);
    }
}

// unary expresssion
void ErrorVisitor::visit(UnaryExpression *e) {
    print_error(e);
    e->expression()->accept(this);
}

// binary expresssion
void ErrorVisitor::visit(BinaryExpression *e) {
    print_error(e);
    e->lhs()->accept(this);
    e->rhs()->accept(this);
}

// binary expresssion
void ErrorVisitor::visit(CallExpression *e) {
    print_error(e);
    for(auto expression: e->args()) {
        expression->accept(this);
    }
}

/*
void ErrorVisitor::print_message(Expression *e) {
    if(e->has_error()) {
        std::cout << colorize("error", kRed) << ":"
                    << e->location() << " "
                    << e->error_message()
                    << std::endl;
        num_errors_++;
    }
}
*/
