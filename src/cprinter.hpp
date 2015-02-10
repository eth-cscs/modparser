#pragma once

#include <sstream>

#include "module.h"
#include "visitor.h"

class CPrinter {
public:
    CPrinter(Module &module);

    std::string text() const {
        return text_.str();
    }

private:
    std::stringstream text_;
};

class TextBuffer {
public:
    TextBuffer& add_gutter() {
        text_ << gutter_;
        return *this;
    }
    void add_line(std::string const& line) {
        text_ << gutter_ << line << std::endl;
    }
    void add_line() {
        text_ << std::endl;
    }
    void end_line(std::string const& line) {
        text_ << line << std::endl;
    }
    void end_line() {
        text_ << std::endl;
    }

    std::string str() const {
        return text_.str();
    }

    void set_gutter(int width) {
        indent_ = width;
        gutter_ = std::string(indent_, ' ');
    }

    void increase_indentation() {
        indent_ += indentation_width_;
        if(indent_<0) {
            indent_=0;
        }
        gutter_ = std::string(indent_, ' ');
    }
    void decrease_indentation() {
        indent_ -= indentation_width_;
        if(indent_<0) {
            indent_=0;
        }
        gutter_ = std::string(indent_, ' ');
    }
    std::stringstream &text() {
        return text_;
    }

private:

    int indent_ = 0;
    const int indentation_width_=4;
    std::string gutter_ = "";
    std::stringstream text_;
};

template <typename T>
TextBuffer& operator<< (TextBuffer& buffer, T const& v) {
    buffer.text() << v;

    return buffer;
}

class CPrinterVisitor : public Visitor {
public:
    CPrinterVisitor() {}

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

    TOK parent_op_ = tok_eq;
    TextBuffer text_;
};

