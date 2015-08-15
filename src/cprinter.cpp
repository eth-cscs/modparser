#include "cprinter.hpp"
#include "lexer.hpp"

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

    std::string kind_str = m.kind() == moduleKind::density
                            ? "mechanismKind::density"
                            : "mechanismKind::point_process";
    text_ << "    mechanismKind kind() const override {\n";
    text_ << "        return " << kind_str << ";\n";
    text_ << "    }\n\n";


    //////////////////////////////////////////////
    //////////////////////////////////////////////

    increase_indentation();
    auto proctest = [] (procedureKind k) {return k == procedureKind::normal || k == procedureKind::api;};
    for(auto &var : m.symbols()) {
        if(   var.second->kind()==symbolKind::procedure
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
    throw compiler_exception(
        "CPrinter doesn't know how to print " + e->to_string(),
        e->location());
}

void CPrinter::visit(LocalDeclaration *e) {
}

void CPrinter::visit(Symbol *e) {
    /*
    std::string const& name = e->name();
    text_ << name;
    auto k = e->kind();
    if(k==symbolKind::ghost) {
        text_ << "[j_]";
    }
    */
    throw compiler_exception("I don't know how to print raw Symbol " + e->to_string(),
                             e->location());
}

void CPrinter::visit(LocalVariable *e) {
    std::string const& name = e->name();
    text_ << name;
    // TODO : this is wrong: only use index if
    //  a) has indexed
    //  b) is write only
    //  c) is in a point process
    if(e->is_indexed() && is_point_process() && e->external_variable()->is_write()) {
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
        case tok::minus :
            // place a space in front of minus sign to avoid invalid
            // expressions of the form : (v[i]--67)
            text_ << " -";
            e->expression()->accept(this);
            return;
        case tok::exp :
            text_ << "exp(";
            e->expression()->accept(this);
            text_ << ")";
            return;
        case tok::cos :
            text_ << "cos(";
            e->expression()->accept(this);
            text_ << ")";
            return;
        case tok::sin :
            text_ << "sin(";
            e->expression()->accept(this);
            text_ << ")";
            return;
        case tok::log :
            text_ << "log(";
            e->expression()->accept(this);
            text_ << ")";
            return;
        default :
            throw compiler_exception(
                "CPrinter unsupported unary operator " + yellow(token_string(e->op())),
                e->location());
    }
}

void CPrinter::visit(BlockExpression *e) {
    // ------------- declare local variables ------------- //
    // only if this is the outer block
    if(!e->is_nested()) {
        std::vector<std::string> names;
        for(auto& symbol : e->scope()->locals()) {
            auto var = symbol.second->is_local_variable();
            // only print definition if variable is local or a
            // write-only indexed variable
            //if(var->is_local() || (!var->is_read())) {
            if(!(var->is_indexed() && var->is_read()) && !var->is_arg()) {
                names.push_back(symbol.first);
            }
        }
        if(names.size()>0) {
            text_.add_gutter() << "value_type " << names[0];
            for(auto it=names.begin()+1; it!=names.end(); ++it) {
                    text_ <<  "{0}, " << *it;
            }
            text_.end_line("{0};");
        }
    }

    // ------------- statements ------------- //
    for(auto& stmt : e->body()) {
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
    for(auto& arg : e->args()) {
        text_ << ", value_type " << arg->is_argument()->name();
    }
    text_.end_line(") {");

    if(!e->scope()) { // error: semantic analysis has not been performed
        throw compiler_exception(
            "CPrinter attempt to print Procedure " + e->name()
            + " for which semantic analysis has not been performed",
            e->location());
    }

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

    if(!e->scope()) { // error: semantic analysis has not been performed
        throw compiler_exception(
            "CPrinter attempt to print APIMethod " + e->name()
            + " for which semantic analysis has not been performed",
            e->location());
    }

    increase_indentation();

    // create local indexed views
    for(auto &symbol : e->scope()->locals()) {
        auto var = symbol.second->is_local_variable();
        if(var->is_indexed()) {
            auto const& name = var->name();
            text_.add_gutter();
            if(var->is_read()) text_ << "const ";
            text_ << "indexed_view " + var->external_variable()->index_name();
            auto channel = var->external_variable()->ion_channel();
            if(channel==ionKind::none) {
                text_ << "(matrix_.vec_" + name + "(), node_indices_);\n";
            }
            else {
                auto iname = ion_store(channel);
                text_ << "(" << iname << "." << name << ", "
                      << ion_store(channel) << ".index);\n";
            }
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
    text_.add_line("START_PROFILE");

    // there can not be more than 1 instance of a density channel per grid point,
    // so we can assert that aliasing will not occur.
    if(optimize_) text_.add_line("#pragma ivdep");

    text_.add_line("for(int i_=0; i_<n_; ++i_) {");
    text_.increase_indentation();

    // loads from external indexed arrays
    for(auto &symbol : e->scope()->locals()) {
        auto var = symbol.second->is_local_variable();
        if(var->is_local()) {
            if(var->is_indexed() && var->is_read()) {
                auto ext = var->external_variable();
                text_.add_line("value_type " + ext->name() + " = "
                               + ext->index_name() + "[i];");
            }
        }
    }

    // print the body of the funtor
    e->body()->accept(this);

    for(auto &symbol : e->scope()->locals()) {
        auto var = symbol.second->is_local_variable();
        if(var->is_indexed() && var->is_write()) {
            //std::cout << "indexed output " << var->external_variable()->to_string() << std::endl;
            auto ext = var->external_variable();
            text_.add_gutter();
            text_ << ext->index_name() + "[i]";
            text_ << (ext->op() == tok::plus ? " += " : " -= ");
            text_ << ext->name() << ";\n";
        }
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

    // :: analyse outputs, to determine if they depend on any
    //    ghost fields.
    std::vector<APIMethod::memop_type*> aliased_variables;
    /*
    if(is_point_process) {
        for(auto &out : e->outputs()) {
            if(out.local->kind() == symbolKind::ghost) {
                aliased_variables.push_back(&out);
            }
        }
    }
    */
    bool aliased_output = aliased_variables.size()>0;

    // only proceed with optimized output if the ouputs are aliased
    // because all optimizations are for using ghost buffers to avoid
    // race conditions in vectorized code
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
    /*
    for(auto &in : e->inputs()) {
        text_.add_gutter();
        in.local->accept(this);
        text_ << " = ";
        in.external->accept(this);
        text_.end_line(";");
    }
    */

    e->body()->accept(this);

    text_.decrease_indentation();
    text_.add_line("}"); // end inner compute loop

    text_.add_line("i_ = BSTART;");
    text_.add_line("for(int j_=0; j_<BSIZE; ++j_, ++i_) {");
    text_.increase_indentation();

    // insert stores here
    /*
    for(auto &out : e->outputs()) {
        text_.add_gutter();
        out.external->accept(this);
        text_ << (out.op==tok::plus ? " += " : " -= ");
        out.local->accept(this);
        text_.end_line(";");
    }
    */

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
    /*
    for(auto &in : e->inputs()) {
        text_.add_gutter();
        in.local->accept(this);
        text_ << " = ";
        in.external->accept(this);
        text_.end_line(";");
    }
    */

    e->body()->accept(this);

    text_.decrease_indentation();
    text_.add_line("}"); // end inner compute loop
    text_.add_line("j_ = 0;");
    text_.add_line("for(int i_=NB*BSIZE; i_<n_; ++j_, ++i_) {");
    text_.increase_indentation();

    /*
    // insert stores here
    for(auto &out : e->outputs()) {
        text_.add_gutter();
        out.external->accept(this);
        text_ << (out.op==tok::plus ? " += " : " -= ");
        out.local->accept(this);
        text_.end_line(";");
    }
    */

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
        case tok::minus :
            text_ << "-";
            break;
        case tok::plus :
            text_ << "+";
            break;
        case tok::times :
            text_ << "*";
            break;
        case tok::divide :
            text_ << "/";
            break;
        case tok::lt     :
            text_ << "<";
            break;
        case tok::lte    :
            text_ << "<=";
            break;
        case tok::gt     :
            text_ << ">";
            break;
        case tok::gte    :
            text_ << ">=";
            break;
        case tok::EQ     :
            text_ << "==";
            break;
        default :
            throw compiler_exception(
                "CPrinter unsupported binary operator " + yellow(token_string(e->op())),
                e->location());
    }
    rhs->accept(this);
    if(use_brackets) {
        text_ << ")";
    }

    // reset parent precedence
    parent_op_ = pop;
}

