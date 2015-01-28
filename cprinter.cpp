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
    std::cout << "CPrinter :: error printing : " << e->to_string() << std::endl;
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

void CPrinter::visit(VariableExpression *e) {
    text_ << e->name();
    if(e->is_range()) {
        text_ << "[i]";
    }
}

void CPrinter::visit(IndexedVariable *e) {
    text_ << e->name() << "[i]";
}

void CPrinter::visit(UnaryExpression *e) {
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

void CPrinter::visit(BlockExpression *e) {
    // ------------- declare local variables ------------- //
    // only if this is the outer block
    if(!e->is_nested()) {
        for(auto var : e->scope()->locals()) {
            if(var.second.kind == k_symbol_local)
                text_ << gutter_ << "double " << var.first << " = 0.;\n";
        }
    }

    // ------------- statements ------------- //
    for(auto stmt : e->body()) {
        if(stmt->is_local_declaration()) continue;
        // these all must be handled
        text_ << gutter_;
        stmt->accept(this);
        text_ << ";\n";
    }
}

void CPrinter::visit(IfExpression *e) {
    text_ << "if(";
    e->condition()->accept(this);
    text_ << ") {\n";
    increase_indentation();
    e->true_branch()->accept(this);
    decrease_indentation();
    text_ << gutter_ << "}";
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

    increase_indentation();

    e->body()->accept(this);

    // ------------- close up ------------- //
    decrease_indentation();
    text_ << gutter_ << "}\n";
    return;
}

void CPrinter::visit(APIMethod *e) {
    // ------------- print prototype ------------- //
    set_gutter(0);
    text_ << gutter_ << "void " << e->name() << "(const int i";
    for(auto arg : e->args()) {
        text_ << ", double " << arg->is_argument()->name();
    }
    text_ << ") {\n";

    assert(e->scope()!=nullptr); // error: semantic analysis has not been performed

    increase_indentation();

    // ------------- add loop for API call ------------- //
    text_ << gutter_ << "int n = node_indices_.size();\n";

    text_ << gutter_ << "for(int i=0; i<n; ++i) {\n";
    increase_indentation();

    // insert loads from external state here
    for(auto &in : e->inputs()) {
        text_ << gutter_;
        in.local.expression->accept(this);
        text_ << " = ";
        in.external.expression->accept(this);
        text_ << ";\n";
    }

    e->body()->accept(this);

    // insert stores here
    for(auto &out : e->outputs()) {
        text_ << gutter_;
        out.external.expression->accept(this);
        text_ << (out.op==tok_plus ? " += " : " -= ");
        out.local.expression->accept(this);
        text_ << ";\n";
    }

    decrease_indentation();
    text_ << gutter_ << "}\n";

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
        case tok_lt     :
            text_ << "<";
            break;
        case tok_lte    :
            text_ << "<=";
            break;
        case tok_gt     :
            text_ << ">";
            break;
        case tok_gte    :
            text_ << ">=";
            break;
        case tok_EQ     :
            text_ << "==";
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

