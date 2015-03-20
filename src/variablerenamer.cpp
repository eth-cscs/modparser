#include "variablerenamer.h"

void VariableRenamer::visit(Expression *e) {
    assert(false);
}

void VariableRenamer::visit(NumberExpression *e) {}

void VariableRenamer::visit(IdentifierExpression *e) {
    if(from_ == e->spelling()) {
        e->rename(to_);
    }
}

void VariableRenamer::visit(UnaryExpression *e) {
    e->expression()->accept(this);
}

void VariableRenamer::visit(BinaryExpression *e) {
    e->lhs()->accept(this);
    e->rhs()->accept(this);
}

void VariableRenamer::visit(BlockExpression *e) {
    for(auto stmnt : e->body()) {
        stmnt->accept(this);
    }
}

void VariableRenamer::visit(CallExpression *e) {
    for(auto arg : e->args()) {
        arg->accept(this);
    }
}

void VariableRenamer::visit(IfExpression *e) {
    e->condition()->accept(this);
    e->true_branch()->accept(this);
    e->false_branch()->accept(this);
}

void VariableRenamer::reset(std::string const& from, std::string const& to) {
    from_ = from;
    to_ = to;
}

