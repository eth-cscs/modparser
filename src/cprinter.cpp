#include "cprinter.h"
#include "lexer.h"

ionKind ion_kind_from_name(std::string field) {
    if(field.substr(0,4) == "ion_") {
        field = field.substr(4);
    }
    if(field=="ica" || field=="eca" || field=="cai" || field=="cao") {
        return k_ion_Ca;
    }
    if(field=="ik" || field=="ek" || field=="ki" || field=="ko") {
        return k_ion_K;
    }
    if(field=="ina" || field=="ena" || field=="nai" || field=="nao") {
        return k_ion_Na;
    }
    return k_ion_none;
}

std::string ion_store(ionKind k) {
    switch(k) {
        case k_ion_Ca:
            return "ion_ca";
        case k_ion_Na:
            return "ion_na";
        case k_ion_K:
            return "ion_k";
        default:
            return "";
    }
}

void CPrinterVisitor::set_gutter(int width) {
    indent_ = width;
    gutter_ = std::string(indent_, ' ');
}

void CPrinterVisitor::increase_indentation() {
    indent_ += indentation_width_;
    if(indent_<0) {
        indent_=0;
    }
    gutter_ = std::string(indent_, ' ');
}
void CPrinterVisitor::decrease_indentation() {
    indent_ -= indentation_width_;
    if(indent_<0) {
        indent_=0;
    }
    gutter_ = std::string(indent_, ' ');
}

void CPrinterVisitor::visit(Expression *e) {
    std::cout << "CPrinterVisitor :: error printing : " << e->to_string() << std::endl;
    assert(false);
}

void CPrinterVisitor::visit(LocalExpression *e) {
}

void CPrinterVisitor::visit(NumberExpression *e) {
    text_ << e->value();
}

void CPrinterVisitor::visit(IdentifierExpression *e) {
    if(auto var = e->variable()) {
        var->accept(this);
    }
    else {
        text_ << e->name();
    }
}

void CPrinterVisitor::visit(VariableExpression *e) {
    text_ << e->name();
    if(e->is_range()) {
        text_ << "[i]";
    }
}

void CPrinterVisitor::visit(IndexedVariable *e) {
    text_ << e->name() << "[i]";
}

void CPrinterVisitor::visit(UnaryExpression *e) {
    switch(e->op()) {
        case tok_minus :
            // place a space in front of minus sign to avoid invalid
            // expressions of the form : (v[i]--67)
            text_ << " -";
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

void CPrinterVisitor::visit(BlockExpression *e) {
    // ------------- declare local variables ------------- //
    // only if this is the outer block
    if(!e->is_nested()) {
        for(auto var : e->scope()->locals()) {
            if(var.second.kind == k_symbol_local)
                text_ << gutter_ << "value_type " << var.first << " = 0.;\n";
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

void CPrinterVisitor::visit(IfExpression *e) {
    // for now we remove the brackets around the condition because
    // the binary expression printer adds them, and we want to work
    // around the -Wparentheses-equality warning
    text_ << "if(";
    e->condition()->accept(this);
    text_ << ") {\n";
    increase_indentation();
    e->true_branch()->accept(this);
    decrease_indentation();
    text_ << gutter_ << "}";
}

void CPrinterVisitor::visit(ProcedureExpression *e) {
    // ------------- print prototype ------------- //
    //set_gutter(0);
    text_ << gutter_ << "void " << e->name() << "(const int i";
    for(auto arg : e->args()) {
        text_ << ", value_type " << arg->is_argument()->name();
    }
    text_ << ") {\n";

    assert(e->scope()!=nullptr); // error: semantic analysis has not been performed

    increase_indentation();

    e->body()->accept(this);

    // ------------- close up ------------- //
    decrease_indentation();
    text_ << gutter_ << "}\n\n";
    return;
}

void CPrinterVisitor::visit(APIMethod *e) {
    // ------------- print prototype ------------- //
    //set_gutter(0);
    text_ << gutter_ << "void " << e->name() << "() {\n";

    assert(e->scope()!=nullptr); // error: semantic analysis has not been performed

    increase_indentation();

    // create local indexed views
    for(auto &in : e->inputs()) {
        auto const& name =
            in.external.expression->is_indexed_variable()->name();
        text_ << gutter_ + "const indexed_view " + name;
        if(name.substr(0,3) == "vec") {
            text_ << "(matrix_." + name + "(), node_indices_);\n";
        }
        else if(name.substr(0,3) == "ion") {
            auto iname = ion_store(ion_kind_from_name(name));
            text_ << "(" + iname + "." + name.substr(4) + ", " + iname + ".index);\n";
        }
        else {
            std::cout << red("compliler error:")
                      << " what to do with indexed view `" << name
                      << "`, which doesn't start with vec_ or ion_?"
                      << std::endl;;
            assert(false);
        }
    }
    for(auto &in : e->outputs()) {
        auto const& name =
            in.external.expression->is_indexed_variable()->name();
        text_ << gutter_ + "indexed_view " + name;
        if(name.substr(0,3) == "vec") {
            text_ << "(matrix_." + name + "(), node_indices_);\n";
        }
        else if(name.substr(0,3) == "ion") {
            auto iname = ion_store(ion_kind_from_name(name));
            text_ << "(" + iname + "." + name.substr(4) + ", " + iname + ".index);\n";
        }
        else {
            std::cout << red("compliler error:")
                      << " what to do with indexed view `" << name
                      << "`, which doesn't start with vec_ or ion_?"
                      << std::endl;;
            assert(false);
        }
    }

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
    text_ << gutter_ << "}\n\n";
    return;
}

void CPrinterVisitor::visit(CallExpression *e) {
    text_ << e->name() << "(i";
    for(auto& arg: e->args()) {
        text_ << ", ";
        arg->accept(this);
    }
    text_ << ")";
}

void CPrinterVisitor::visit(AssignmentExpression *e) {
    e->lhs()->accept(this);
    text_ << " = ";
    e->rhs()->accept(this);
}

void CPrinterVisitor::visit(PowBinaryExpression *e) {
    text_ << "std::pow(";
    e->lhs()->accept(this);
    text_ << ", ";
    e->rhs()->accept(this);
    text_ << ")";
}

void CPrinterVisitor::visit(BinaryExpression *e) {
    auto pop = parent_op_;
    bool use_brackets = Lexer::binop_precedence(pop) > Lexer::binop_precedence(e->op());
    parent_op_ = e->op();

    auto lhs = e->lhs();
    auto rhs = e->rhs();
    if(use_brackets) {
        text_ << "(";
    }
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
    if(use_brackets) {
        text_ << ")";
    }

    // reset parent precedence
    parent_op_ = pop;
}

CPrinter::CPrinter(Module &m) {
    // make a list of vector types, both parameters and assigned
    // and a list of all scalar types
    std::vector<VariableExpression*> scalar_variables;
    std::vector<VariableExpression*> array_variables;
    for(auto sym: m.symbols()) {
        if(sym.second.kind==k_symbol_variable) {
            auto var = sym.second.expression->is_variable();
            if(var->is_range()) {
                array_variables.push_back(var);
            }
            else {
                scalar_variables.push_back(var);
            }
        }
    }

    //////////////////////////////////////////////
    //////////////////////////////////////////////
    text_ << "#pragma once\n\n";
    text_ << "#include <cmath>\n\n";
    text_ << "#include <matrix.h>\n";
    text_ << "#include <indexedview.h>\n\n";

    //////////////////////////////////////////////
    //////////////////////////////////////////////
    std::string class_name = "Mechanism_" + m.name();

    text_ << "class " + class_name + " {\n";
    text_ << "public:\n\n";
    text_ << "    using value_type  = double;\n";
    text_ << "    using size_type   = int;\n";
    text_ << "    using vector_type = memory::HostVector<value_type>;\n";
    text_ << "    using view_type   = memory::HostView<value_type>;\n";
    text_ << "    using index_type  = memory::HostVector<size_type>;\n";
    text_ << "    using indexed_view= IndexedView<value_type, size_type>;\n\n";

    //////////////////////////////////////////////
    //////////////////////////////////////////////
    for(auto& ion: m.neuron_block().ions) {
        auto tname = "Ion" + ion.name;
        text_ << "    struct " + tname + " {\n";
        for(auto& field : ion.read) {
            text_ << "        view_type " + field + ";\n";
        }
        for(auto& field : ion.write) {
            text_ << "        view_type " + field + ";\n";
        }
        text_ << "        index_type index;\n";
        text_ << "    };\n";
        text_ << "    " + tname + " ion_" + ion.name + ";\n\n";
    }

    //////////////////////////////////////////////
    //////////////////////////////////////////////
    text_ << "    " + class_name + "( index_type const& node_indices,\n";
    text_ << "               Matrix &matrix)\n";
    text_ << "    :  matrix_(matrix), node_indices_(node_indices)\n";
    text_ << "    {\n";
    int num_vars = array_variables.size();
    text_ << "        size_type num_fields = " << num_vars << ";\n";
    text_ << "        size_type n = size();\n";
    text_ << "        data_ = vector_type(n * num_fields);\n";
    text_ << "        data_(all) = std::numeric_limits<value_type>::quiet_NaN();\n";
    for(int i=0; i<num_vars; ++i) {
        char namestr[128];
        sprintf(namestr, "%-15s", array_variables[i]->name().c_str());
        text_ << "        " << namestr << " = data_(" << i << "*n, " << i+1 << "*n);\n";
    }
    for(auto const& var : array_variables) {
        double val = var->value();
        // only non-NaN fields need to be initialized, because data_
        // is NaN by default
        if(val == val) {
            text_ << "        " << var->name() << "(all) = " << val << ";\n";
        }
    }
    text_ << "    }\n\n";

    //////////////////////////////////////////////
    //////////////////////////////////////////////

    text_ << "    size_type size() const {\n";
    text_ << "        return node_indices_.size();\n";
    text_ << "    }\n\n";

    text_ << "    void set_params(value_type t_, value_type dt_) {\n";
    text_ << "        t = t_;\n";
    text_ << "        dt = dt_;\n";
    text_ << "    }\n\n";


    //////////////////////////////////////////////
    //////////////////////////////////////////////

    auto v = new CPrinterVisitor();
    v->increase_indentation();
    auto proctest = [] (procedureKind k) {return k == k_proc_normal || k == k_proc_api;};
    for(auto const &var : m.symbols()) {
        if(   var.second.kind==k_symbol_procedure
           && proctest(var.second.expression->is_procedure()->kind()))
        {
            var.second.expression->accept(v);
        }
    }
    text_ << v->text();

    //////////////////////////////////////////////
    //////////////////////////////////////////////

    //text_ << "private:\n\n";
    text_ << "    vector_type data_;\n\n" << std::endl;
    for(auto var: array_variables) {
        text_ << "    view_type " << var->name() << ";\n";
    }
    for(auto var: scalar_variables) {
        double val = var->value();
        // test the default value for NaN
        // useful for error propogation from bad initial conditions
        if(val==val) {
            text_ << "    value_type " << var->name() << " = " << val << ";\n";
        }
        else {
            text_ << "    value_type " << var->name()
              << " = std::numeric_limits<value_type>::quiet_NaN();\n";
        }
    }

    text_ << "    Matrix &matrix_;\n";
    text_ << "    index_type const& node_indices_;\n";

    text_ << "};\n\n";
}

