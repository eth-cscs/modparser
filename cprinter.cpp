#include "cprinter.h"

void CPrinter::set_gutter(int width) {
    indent_ = width;
    gutter_ = std::string(indent_, ' ');
}

void CPrinter::increase_indentation() {
    indent_ += indentation_width_;
    if(indent_<0) {
        indent_=0;
    }
    gutter_ = std::string(indent_, ' ');
}
void CPrinter::decrease_indentation() {
    indent_ -= indentation_width_;
    if(indent_<0) {
        indent_=0;
    }
    gutter_ = std::string(indent_, ' ');
}

void CPrinter::visit(Expression *e) {
    assert(false);
}

void CPrinter::visit(LocalExpression *e) {
}

void CPrinter::visit(NumberExpression *e) {
    text_ << e->value();
}

void CPrinter::visit(IdentifierExpression *e) {
    text_ << e->name();
    auto var = e->variable();
    if(var!=nullptr && var->is_range()) {
        text_ << "[i]";
    }
}

void CPrinter::visit(UnaryExpression *e) {
    //auto is_leaf = [] (Expression* e) {
        //return e->is_number() || e->is_identifier() || e->is_function_call();
    //};
    switch(e->op()) {
        case tok_minus :
            text_ << "-";
            e->expression()->accept(this);
            return;
        case tok_exp :
            text_ << "exp(";
            e->expression()->accept(this);
            text_ << ")";
            return;
        case tok_cos :
            text_ << "cos(";
            e->expression()->accept(this);
            text_ << ")";
            return;
        case tok_sin :
            text_ << "sin(";
            e->expression()->accept(this);
            text_ << ")";
            return;
        case tok_log :
            text_ << "log(";
            e->expression()->accept(this);
            text_ << ")";
            return;
        default :
            std::cout
                << red("compiler error: ") << white(pprintf("%", e->location()))
                << " cprinter : unsupported unary operator "
                << yellow(token_string(e->op())) << std::endl;
            assert(false);
    }
}

void CPrinter::visit(ProcedureExpression *e) {
    // ------------- print prototype ------------- //
    set_gutter(0);
    text_ << gutter_ << "void " << e->name() << "(const int i";
    for(auto arg : e->args()) {
        text_ << ", double " << arg->is_argument()->name();
    }
    text_ << ") {\n";

    assert(e->scope()!=nullptr); // error: semantic analysis has not been performed

    // ------------- declare local variables ------------- //
    for(auto var : e->scope()->locals()) {
        if(var.second.kind == k_symbol_local)
            text_ << gutter_ << "  double " << var.first << ";\n";
    }
    increase_indentation();

    // ------------- add loop if API call ------------- //
    if(e->kind() == k_proc_api) {
        text_ << gutter_ << "auto n = node_indices_.size();\n";
        text_ << gutter_ << "for(int i=0; i<n; ++i) {\n";
        increase_indentation();
    }

    // ------------- statements ------------- //
    for(auto stmt : *(e->body())) {
        if(stmt->is_local_declaration()) continue;
        // these all must be handled
        //if(stmt->is_procedure_call()) continue;
        text_ << gutter_;
        stmt->accept(this);
        text_ << ";\n";
    }

    if(e->kind() == k_proc_api) {
        decrease_indentation();
        text_ << gutter_ << "}\n";
    }

    // ------------- close up ------------- //
    decrease_indentation();
    text_ << gutter_ << "}\n";
    return;
}

void CPrinter::visit(CallExpression *e) {
    text_ << e->name() << "(i";
    for(auto& arg: e->args()) {
        text_ << ", ";
        arg->accept(this);
    }
    text_ << ")";
}

void CPrinter::visit(AssignmentExpression *e) {
    e->lhs()->accept(this);
    text_ << " = ";
    e->rhs()->accept(this);
}

void CPrinter::visit(PowBinaryExpression *e) {
    text_ << "std::pow(";
    e->lhs()->accept(this);
    text_ << ", ";
    e->rhs()->accept(this);
    text_ << ")";
}

void CPrinter::visit(BinaryExpression *e) {
    auto lhs = e->lhs();
    auto rhs = e->rhs();
    text_ << "(";
    lhs->accept(this);
    switch(e->op()) {
        case tok_minus :
            text_ << "-";
            break;
        case tok_plus :
            text_ << "+";
            break;
        case tok_times :
            text_ << "*";
            break;
        case tok_divide :
            text_ << "/";
            break;
        default :
            std::cout
                << red("compiler error: ") << white(pprintf("%", e->location()))
                << " cprinter : unsupported binary operator "
                << yellow(token_string(e->op())) << std::endl;
            assert(false);
    }
    rhs->accept(this);
    text_ << ")";
}

