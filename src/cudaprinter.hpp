#pragma once

#include <sstream>

#include "module.h"
#include "textbuffer.hpp"
#include "visitor.h"

class CUDAPrinter : public Visitor {
public:
    CUDAPrinter() {}
    CUDAPrinter(Module &m, bool o=false);

    void visit(Expression *e)           override;
    void visit(UnaryExpression *e)      override;
    void visit(BinaryExpression *e)     override;
    void visit(AssignmentExpression *e) override;
    void visit(PowBinaryExpression *e)  override;
    void visit(NumberExpression *e)     override;
    void visit(VariableExpression *e)   override;
    void visit(IndexedVariable *e)      override;

    void visit(Symbol *e)               override;
    void visit(IdentifierExpression *e) override;
    void visit(CallExpression *e)       override;
    void visit(ProcedureExpression *e)  override;
    void visit(APIMethod *e)            override;
    void visit(LocalDeclaration *e)      override;
    //void visit(FunctionExpression *e)   override;
    void visit(BlockExpression *e)      override;
    void visit(IfExpression *e)         override;

    std::string text() const {
        return text_.str();
    }

    void set_gutter(int w) {
        text_.set_gutter(w);
    }
    void increase_indentation(){
        text_.increase_indentation();
    }
    void decrease_indentation(){
        text_.decrease_indentation();
    }
private:

    void print_APIMethod_body(APIMethod* e);
    void print_procedure_prototype(ProcedureExpression *e);
    std::string index_string(Symbol *e);

    Module *module_ = nullptr;
    TOK parent_op_ = tok_eq;
    TextBuffer text_;
    //bool optimize_ = false;
};

