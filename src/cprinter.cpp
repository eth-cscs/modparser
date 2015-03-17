#include "cprinter.hpp"
#include "lexer.h"

/******************************************************************************
                              CPrinter driver
******************************************************************************/

CPrinter::CPrinter(Module &m, bool o)
:   module_(&m),
    optimize_(o)
{
    // make a list of vector types, both parameters and assigned
    // and a list of all scalar types
    std::vector<VariableExpression*> scalar_variables;
    std::vector<VariableExpression*> array_variables;
    for(auto& sym: m.symbols()) {
        if(auto var = sym.second->is_variable()) {
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
    text_ << "#include <cmath>\n";
    text_ << "#include <limits>\n\n";
    text_ << "#include <indexedview.hpp>\n";
    text_ << "#include <mechanism.hpp>\n";
    text_ << "#include <target.hpp>\n\n";

    //////////////////////////////////////////////
    //////////////////////////////////////////////
    std::string class_name = "Mechanism_" + m.name();

    text_ << "template<typename T, typename I>\n";
    text_ << "class " + class_name + " : public Mechanism<T, I, targetKind::cpu> {\n";
    text_ << "public:\n\n";
    text_ << "    using base = Mechanism<T, I, targetKind::cpu>;\n";
    text_ << "    using value_type  = typename base::value_type;\n";
    text_ << "    using size_type   = typename base::size_type;\n";
    text_ << "    using vector_type = typename base::vector_type;\n";
    text_ << "    using view_type   = typename base::view_type;\n";
    text_ << "    using index_type  = typename base::index_type;\n";
    text_ << "    using index_view  = typename index_type::view_type;\n";
    text_ << "    using indexed_view= typename base::indexed_view;\n\n";
    text_ << "    using matrix_type = typename base::matrix_type;\n\n";

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
        text_ << "        std::size_t memory() const { return sizeof(size_type)*index.size(); }\n";
        text_ << "        std::size_t size() const { return index.size(); }\n";
        text_ << "    };\n";
        text_ << "    " + tname + " ion_" + ion.name + ";\n\n";
    }

    //////////////////////////////////////////////
    //////////////////////////////////////////////
    int num_vars = array_variables.size();
    text_ << "    " + class_name + "(\n";
    text_ << "        matrix_type &matrix,\n";
    text_ << "        index_view node_indices)\n";
    text_ << "    :   base(matrix, node_indices)\n";
    text_ << "    {\n";
    text_ << "        size_type num_fields = " << num_vars << ";\n";
    text_ << "        size_type n = size();\n";
    text_ << "        data_ = vector_type(n * num_fields);\n";
    text_ << "        data_(memory::all) = std::numeric_limits<value_type>::quiet_NaN();\n";
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
            text_ << "        " << var->name() << "(memory::all) = " << val << ";\n";
        }
    }

    text_ << "        INIT_PROFILE\n";
    text_ << "    }\n\n";

    //////////////////////////////////////////////
    //////////////////////////////////////////////

    text_ << "    using base::size;\n\n";

    text_ << "    std::size_t memory() const override {\n";
    text_ << "        auto s = std::size_t{0};\n";
    text_ << "        s += data_.size()*sizeof(value_type);\n";
    for(auto& ion: m.neuron_block().ions) {
        text_ << "        s += ion_" + ion.name + ".memory();\n";
    }
    text_ << "        return s;\n";
    text_ << "    }\n\n";

    text_ << "    void set_params(value_type t_, value_type dt_) override {\n";
    text_ << "        t = t_;\n";
    text_ << "        dt = dt_;\n";
    text_ << "    }\n\n";

    text_ << "    std::string name() const override {\n";
    text_ << "        return \"" << m.name() << "\";\n";
    text_ << "    }\n\n";

    std::string kind_str = m.kind() == k_module_density
                            ? "mechanismKind::density"
                            : "mechanismKind::point_process";
    text_ << "    mechanismKind kind() const override {\n";
    text_ << "        return " << kind_str << ";\n";
    text_ << "    }\n\n";


    //////////////////////////////////////////////
    //////////////////////////////////////////////

    increase_indentation();
    auto proctest = [] (procedureKind k) {return k == k_proc_normal || k == k_proc_api;};
    for(auto &var : m.symbols()) {
        if(   var.second->kind()==k_symbol_procedure
           && proctest(var.second->is_procedure()->kind()))
        {
            var.second->accept(this);
        }
    }

    //////////////////////////////////////////////
    //////////////////////////////////////////////

    text_ << "    vector_type data_;\n\n";
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

    text_ << "    using base::matrix_;\n";
    text_ << "    using base::node_indices_;\n\n";

    text_ << "\n    DATA_PROFILE\n";
    text_ << "};\n\n";
}


/******************************************************************************
                              CPrinter
******************************************************************************/

void CPrinter::visit(Expression *e) {
    std::cout << red("error ") << " CPrinter "
              << "doesn't know how to print the expression : "
              << std::endl << "  " << e->to_string() << std::endl;
    assert(false);
}

void CPrinter::visit(LocalDeclaration *e) {
}

void CPrinter::visit(Symbol *e) {
    std::string const& name = e->name();
    text_ << name;
    auto k = e->kind();
    if(k==k_symbol_ghost) {
        text_ << "[j_]";
    }
}

void CPrinter::visit(NumberExpression *e) {
    text_ << " " << e->value();
}

void CPrinter::visit(IdentifierExpression *e) {
    e->symbol()->accept(this);
}

void CPrinter::visit(VariableExpression *e) {
    text_ << e->name();
    if(e->is_range()) {
        text_ << "[i_]";
    }
}

void CPrinter::visit(IndexedVariable *e) {
    text_ << e->name() << "[i_]";
}

void CPrinter::visit(UnaryExpression *e) {
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

void CPrinter::visit(BlockExpression *e) {
    // ------------- declare local variables ------------- //
    // only if this is the outer block
    if(!e->is_nested()) {
        std::vector<std::string> names;
        for(auto& var : e->scope()->locals()) {
            if(var.second->kind() == k_symbol_local)
                names.push_back(var.first);
        }
        if(names.size()>0) {
            text_.add_gutter() << "value_type " << names[0];
            for(auto it=names.begin()+1; it!=names.end(); ++it) {
                    text_ <<  "(0), " << *it;
            }
            text_.end_line("(0);");
        }
    }

    // ------------- statements ------------- //
    for(auto stmt : e->body()) {
        if(stmt->is_local_declaration()) continue;
        // these all must be handled
        text_.add_gutter();
        stmt->accept(this);
        text_.end_line(";");
    }
}

void CPrinter::visit(IfExpression *e) {
    // for now we remove the brackets around the condition because
    // the binary expression printer adds them, and we want to work
    // around the -Wparentheses-equality warning
    text_ << "if(";
    e->condition()->accept(this);
    text_ << ") {\n";
    increase_indentation();
    e->true_branch()->accept(this);
    decrease_indentation();
    text_.add_gutter();
    text_ << "}";
}

void CPrinter::visit(ProcedureExpression *e) {
    // ------------- print prototype ------------- //
    //set_gutter(0);
    text_.add_gutter() << "void " << e->name() << "(const int i_";
    for(auto arg : e->args()) {
        text_ << ", value_type " << arg->is_argument()->name();
    }
    text_.end_line(") {");

    assert(e->scope()!=nullptr); // error: semantic analysis has not been performed

    increase_indentation();

    e->body()->accept(this);

    // ------------- close up ------------- //
    decrease_indentation();
    text_.add_line("}");
    text_.add_line();
    return;
}

void CPrinter::visit(APIMethod *e) {
    // ------------- print prototype ------------- //
    text_.add_gutter() << "void " << e->name() << "() {";
    text_.end_line();

    assert(e->scope()!=nullptr); // error: semantic analysis has not been performed

    increase_indentation();

    // create local indexed views
    for(auto &in : e->inputs()) {
        auto const& name =
            in.external->is_indexed_variable()->name();
        text_.add_gutter();
        text_ << "const indexed_view " + name;
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
            in.external->is_indexed_variable()->name();
        text_.add_gutter() << "indexed_view " + name;
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

    // ------------- get loop dimensions ------------- //
    text_.add_line("int n_ = node_indices_.size();");

    // hand off printing of loops to optimized or unoptimized backend
    if(optimize_) {
        print_APIMethod_optimized(e);
    }
    else {
        print_APIMethod_unoptimized(e);
    }
}

void CPrinter::print_APIMethod_unoptimized(APIMethod* e) {
    // ------------- get mechanism properties ------------- //
    //bool is_density = module_->kind() == k_module_density;

    text_.add_line("START_PROFILE");
    // there can not be more than 1 instance of a density chanel per grid point,
    // so we can assert that aliasing will not occur for 
    // if this is a point process, then we were redirected here as there
    // is no aliased output
    //if(is_density || optimize_) text_.add_line("#pragma ivdep");
    if(optimize_) text_.add_line("#pragma ivdep");

    text_.add_line("for(int i_=0; i_<n_; ++i_) {");
    text_.increase_indentation();

    // insert loads from external state here
    for(auto &in : e->inputs()) {
        text_.add_gutter();
        in.local->accept(this);
        text_ << " = ";
        in.external->accept(this);
        text_.end_line(";");
    }

    e->body()->accept(this);

    // insert stores here
    for(auto &out : e->outputs()) {
        text_.add_gutter();
        out.external->accept(this);
        text_ << (out.op==tok_plus ? " += " : " -= ");
        out.local->accept(this);
        text_.end_line(";");
    }

    text_.decrease_indentation();
    text_.add_line("}");

    text_.add_line("STOP_PROFILE");

    // ------------- close up ------------- //
    decrease_indentation();
    text_.add_line("}");
    text_.add_line();
    return;
}

void CPrinter::print_APIMethod_optimized(APIMethod* e) {
    // ------------- get mechanism properties ------------- //
    bool is_point_process = module_->kind() == k_module_point;

    // :: analyse outputs, to determine if they depend on any
    //    ghost fields.
    std::vector<APIMethod::memop_type*> aliased_variables;
    if(is_point_process) {
        for(auto &out : e->outputs()) {
            if(out.local->kind() == k_symbol_ghost) {
                aliased_variables.push_back(&out);
            }
        }
    }
    bool aliased_output = aliased_variables.size()>0;

    // only proceed with optimized output if required
    if(!aliased_output) {
        print_APIMethod_unoptimized(e);
        return;
    }

    // ------------- block loop ------------- //

    text_.add_line("constexpr int BSIZE = 64;");
    text_.add_line("int NB = n_/BSIZE;");
    for(auto out: aliased_variables) {
        text_.add_line("value_type " + out->local->name() + "[BSIZE];");
    }
    text_.add_line("START_PROFILE");

    text_.add_line("for(int b_=0; b_<NB; ++b_) {");
    text_.increase_indentation();
    text_.add_line("int BSTART = BSIZE*b_;");
    text_.add_line("int i_ = BSTART;");


    // assert that memory accesses are not aliased because we will
    // use ghost arrays to ensure that write-back of point processes does
    // not lead to race conditions
    text_.add_line("#pragma ivdep");
    text_.add_line("for(int j_=0; j_<BSIZE; ++j_, ++i_) {");
    text_.increase_indentation();

    // initialize ghost fields to zero
    for(auto out: aliased_variables) {
        text_.add_line(out->local->name() + "[j_] = value_type{0.};");
    }

    // insert loads from external state here
    for(auto &in : e->inputs()) {
        text_.add_gutter();
        in.local->accept(this);
        text_ << " = ";
        in.external->accept(this);
        text_.end_line(";");
    }

    e->body()->accept(this);

    text_.decrease_indentation();
    text_.add_line("}"); // end inner compute loop

    text_.add_line("i_ = BSTART;");
    text_.add_line("for(int j_=0; j_<BSIZE; ++j_, ++i_) {");
    text_.increase_indentation();

    // insert stores here
    for(auto &out : e->outputs()) {
        text_.add_gutter();
        out.external->accept(this);
        text_ << (out.op==tok_plus ? " += " : " -= ");
        out.local->accept(this);
        text_.end_line(";");
    }

    text_.decrease_indentation();
    text_.add_line("}"); // end inner write loop
    text_.decrease_indentation();
    text_.add_line("}"); // end outer block loop

    // ------------- block tail loop ------------- //

    text_.add_line("int j_ = 0;");
    text_.add_line("#pragma ivdep");
    text_.add_line("for(int i_=NB*BSIZE; i_<n_; ++j_, ++i_) {");
    text_.increase_indentation();

    // initialize ghost fields to zero
    for(auto out: aliased_variables) {
        text_.add_line(out->local->name() + "[j_] = value_type{0.};");
    }


    // insert loads from external state here
    for(auto &in : e->inputs()) {
        text_.add_gutter();
        in.local->accept(this);
        text_ << " = ";
        in.external->accept(this);
        text_.end_line(";");
    }

    e->body()->accept(this);

    text_.decrease_indentation();
    text_.add_line("}"); // end inner compute loop
    text_.add_line("j_ = 0;");
    text_.add_line("for(int i_=NB*BSIZE; i_<n_; ++j_, ++i_) {");
    text_.increase_indentation();

    // insert stores here
    for(auto &out : e->outputs()) {
        text_.add_gutter();
        out.external->accept(this);
        text_ << (out.op==tok_plus ? " += " : " -= ");
        out.local->accept(this);
        text_.end_line(";");
    }

    text_.decrease_indentation();
    text_.add_line("}"); // end block tail loop


    text_.add_line("STOP_PROFILE");

    // ------------- close up ------------- //
    decrease_indentation();
    text_.add_line("}");
    text_.add_line();
    return;
}

void CPrinter::visit(CallExpression *e) {
    text_ << e->name() << "(i_";
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

