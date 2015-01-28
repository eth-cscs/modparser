#pragma once

#include <sstream>

#include "visitor.h"

class CPrinter : public Visitor {
public:
    CPrinter() {}

    void visit(Expression *e)           override;
    void visit(UnaryExpression *e)      override;
    void visit(BinaryExpression *e)     override;
    void visit(AssignmentExpression *e) override;
    void visit(PowBinaryExpression *e)  override;
    void visit(NumberExpression *e)     override;
    void visit(VariableExpression *e)   override;
    void visit(IndexedVariable *e)      override;

    void visit(IdentifierExpression *e) override;
    void visit(CallExpression *e)       override;
    void visit(ProcedureExpression *e)  override;
    void visit(APIMethod *e)            override;
    void visit(LocalExpression *e)      override;
    //void visit(FunctionExpression *e)   override;
    void visit(BlockExpression *e)      override;
    void visit(IfExpression *e)         override;

    std::string text() const {
        return text_.str();
    }
private:

    void set_gutter(int);
    void increase_indentation();
    void decrease_indentation();

    int indent_ = 0;
    const int indentation_width_=4;
    std::string gutter_ = "";
    std::stringstream text_;
};

