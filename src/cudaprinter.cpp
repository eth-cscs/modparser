#include "cudaprinter.hpp"
#include "lexer.hpp"

/******************************************************************************
******************************************************************************/

CUDAPrinter::CUDAPrinter(Module &m, bool o)
    :   module_(&m)//,
        //optimize_(o)
{
    // make a list of vector types, both parameters and assigned
    // and a list of all scalar types
    std::vector<VariableExpression*> scalar_variables;
    std::vector<VariableExpression*> array_variables;
    for(auto& sym: m.symbols()) {
        if(sym.second->kind()==k_symbol_variable) {
            auto var = sym.second->is_variable();
            if(var->is_range()) {
                array_variables.push_back(var);
            }
            else {
                scalar_variables.push_back(var);
            }
        }
    }

    //////////////////////////////////////////////
    // header files
    //////////////////////////////////////////////
    text_ << "#pragma once\n\n";
    text_ << "#include <cmath>\n";
    text_ << "#include <limits>\n\n";
    text_ << "#include <indexedview.hpp>\n";
    text_ << "#include <mechanism.hpp>\n";
    text_ << "#include <target.hpp>\n\n";

    ////////////////////////////////////////////////////////////
    // generate the parameter pack
    ////////////////////////////////////////////////////////////
    std::vector<std::string> param_pack;
    text_ << "template <typename T, typename I>\n";
    text_ << "struct " << m.name() << "_ParamPack {\n";
    text_ << "  // array parameters\n";
    for(auto const &var: array_variables) {
        text_ << "  T* " << var->name() << ";\n";
        param_pack.push_back(var->name() + ".data()");
    }
    text_ << "\n  // scalar parameters\n";
    for(auto const &var: scalar_variables) {
        text_ << "  T " << var->name() << ";\n";
        param_pack.push_back(var->name());
    }
    text_ << "\n  // ion channel dependencies\n";
    for(auto& ion: m.neuron_block().ions) {
        auto tname = "ion_" + ion.name;
        for(auto& field : ion.read) {
            text_ << "  T* ion_" + field + ";\n";
            param_pack.push_back(tname + "." + field + ".data()");
        }
        for(auto& field : ion.write) {
            text_ << "  T* ion_" + field + ";\n";
            param_pack.push_back(tname + "." + field + ".data()");
        }
        text_ << "  I* ion_" + ion.name + "_idx_;\n";
        param_pack.push_back(tname + ".index.data()");
    }

    text_ << "\n  // matrix\n";
    text_ << "  T* vec_rhs;\n";
    text_ << "  T* vec_d;\n";
    text_ << "  T* vec_v;\n";
    param_pack.push_back("matrix_.vec_rhs().data()");
    param_pack.push_back("matrix_.vec_d().data()");
    param_pack.push_back("matrix_.vec_v().data()");

    text_ << "\n  // node index information\n";
    text_ << "  I* ni;\n";
    text_ << "  unsigned long n;\n";
    text_ << "};\n\n";
    param_pack.push_back("node_indices_.data()");
    param_pack.push_back("node_indices_.size()");

    ////////////////////////////////////////////////////////
    // write the CUDA kernels
    ////////////////////////////////////////////////////////
    text_.add_line("namespace impl {");
    text_.add_line("namespace " + m.name() + " {");
    text_.add_line();
    {
        increase_indentation();
        // forward declarations of procedures
        for(auto const &var : m.symbols()) {
            if(   var.second->kind()==k_symbol_procedure
            && var.second->is_procedure()->kind() == k_proc_normal)
            {
                print_procedure_prototype(var.second->is_procedure());
                text_.end_line(";");
                text_.add_line();
            }
        }

        // print stubs that call API method kernels that are defined in the
        // imp::name namespace
        auto proctest = [] (procedureKind k) {return k == k_proc_normal
                                                  || k == k_proc_api;   };
        for(auto const &var : m.symbols()) {
            if(   var.second->kind()==k_symbol_procedure
            && proctest(var.second->is_procedure()->kind()))
            {
                var.second->accept(this);
            }
        }
        decrease_indentation();
    }
    text_.add_line("} // namespace " + m.name());
    text_.add_line("} // namespace impl");
    text_.add_line();

    //////////////////////////////////////////////
    //////////////////////////////////////////////
    std::string class_name = "Mechanism_" + m.name();

    text_ << "template<typename T, typename I>\n";
    text_ << "class " + class_name + " : public Mechanism<T, I, targetKind::gpu> {\n";
    text_ << "public:\n\n";
    text_ << "    using base = Mechanism<T, I, targetKind::gpu>;\n";
    text_ << "    using value_type  = typename base::value_type;\n";
    text_ << "    using size_type   = typename base::size_type;\n";
    text_ << "    using vector_type = typename base::vector_type;\n";
    text_ << "    using view_type   = typename base::view_type;\n";
    text_ << "    using index_type  = typename base::index_type;\n";
    text_ << "    using index_view  = typename index_type::view_type;\n";
    text_ << "    using indexed_view= typename base::indexed_view;\n\n";
    text_ << "    using matrix_type = typename base::matrix_type;\n\n";
    text_ << "    using param_pack_type = " << m.name() << "_ParamPack<T,I>;\n\n";

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
    text_ << "        param_pack_ = param_pack_type{\n";
    //for(auto i=0; i<param_pack.size(); ++i)
    for(auto &str: param_pack) {
        text_ << "          " << str << ",\n";
    }
    text_ << "        };\n";
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

    auto proctest = [] (procedureKind k) {return k == k_proc_api;};
    for(auto const &var : m.symbols()) {
        if(   var.second->kind()==k_symbol_procedure
        && proctest(var.second->is_procedure()->kind()))
        {
            auto proc = var.second->is_api_method();
            auto name = proc->name();
            text_ << "  void " << name << "() {\n";
            text_ << "    auto n = size();\n";
            text_ << "    auto thread_dim = 192;\n";
            text_ << "    dim3 dim_block(thread_dim);\n";
            text_ << "    dim3 dim_grid(n/dim_block.x + (n%dim_block.x ? 1 : 0) );\n\n";
            text_ << "    START_PROFILE\n";
            text_ << "    impl::" << m.name() << "::" << name << "<T,I>"
                    << "<<<dim_grid, dim_block>>>(param_pack_);\n";
            text_ << "    STOP_PROFILE\n";
            text_ << "  }\n";
        }
    }

    //////////////////////////////////////////////
    //////////////////////////////////////////////

    //text_ << "private:\n\n";
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
            // the cuda compiler has a bug that doesn't allow initialization of
            // class members with std::numer_limites<>. So simply set to zero.
            text_ << "    value_type " << var->name()
              << " = value_type{0};\n";
        }
    }

    text_ << "    using base::matrix_;\n";
    text_ << "    using base::node_indices_;\n\n";
    text_ << "    param_pack_type param_pack_;\n\n";
    text_ << "    DATA_PROFILE\n";
    text_ << "};\n";
}

void CUDAPrinter::visit(Expression *e) {
    throw compiler_exception(
        "CUDAPrinter doesn't know how to print " + e->to_string(),
        e->location());
}

void CUDAPrinter::visit(LocalDeclaration *e) {
}

void CUDAPrinter::visit(NumberExpression *e) {
    text_ << " " << e->value();
}

void CUDAPrinter::visit(IdentifierExpression *e) {
    e->symbol()->accept(this);
}

void CUDAPrinter::visit(Symbol *e) {
    text_ << e->name();
}

void CUDAPrinter::visit(VariableExpression *e) {
    text_ << "params_." << e->name();
    if(e->is_range()) {
        text_ << "[" << index_string(e) << "]";
    }
}

std::string CUDAPrinter::index_string(Symbol *s) {
    if(s->is_variable()) {
        return "tid_";
    }
    else if(auto var = s->is_indexed_variable()) {
        switch(var->ion_channel()) {
            case k_ion_none :
                return "gid_";
            case k_ion_Ca   :
                return "caid_";
            case k_ion_Na   :
                return "naid_";
            case k_ion_K    :
                return "kid_";
            // a nonspecific ion current should never be indexed: it is
            // local to a mechanism
            case k_ion_nonspecific:
                break;
            default :
                throw compiler_exception(
                    "CUDAPrinter unknown ion type",
                    s->location());
        }
    }
    return "";
}

void CUDAPrinter::visit(IndexedVariable *e) {
    text_ << "params_." << e->name() << "[" << index_string(e) << "]";
}

void CUDAPrinter::visit(UnaryExpression *e) {
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
            throw compiler_exception(
                "CUDAPrinter unsupported unary operator " + yellow(token_string(e->op())),
                e->location());
    }
}

void CUDAPrinter::visit(BlockExpression *e) {
    // ------------- declare local variables ------------- //
    // only if this is the outer block
    if(!e->is_nested()) {
        std::vector<std::string> names;
        for(auto& var : e->scope()->locals()) {
            if(var.second->kind() == k_symbol_local) {
                text_.add_line("auto " + var.first + " = T{0};");
            }
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

void CUDAPrinter::visit(IfExpression *e) {
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

void CUDAPrinter::print_procedure_prototype(ProcedureExpression *e) {
    text_.add_gutter() << "template <typename T, typename I>\n";
    text_.add_line("__device__");
    text_.add_gutter() << "void " << e->name()
                       << "(" << module_->name() << "_ParamPack<T,I> const& params_,"
                       << "const int tid_";
    for(auto& arg : e->args()) {
        text_ << ", T " << arg->is_argument()->name();
    }
    text_ << ")";
}

void CUDAPrinter::visit(ProcedureExpression *e) {
    // error: semantic analysis has not been performed
    if(!e->scope()) { // error: semantic analysis has not been performed
        throw compiler_exception(
            "CUDAPrinter attempt to print Procedure " + e->name()
            + " for which semantic analysis has not been performed",
            e->location());
    }

    // ------------- print prototype ------------- //
    print_procedure_prototype(e);
    text_.end_line(" {");

    // ------------- print body ------------- //
    increase_indentation();

    e->body()->accept(this);

    // ------------- close up ------------- //
    decrease_indentation();
    text_.add_line("}");
    text_.add_line();
    return;
}

void CUDAPrinter::visit(APIMethod *e) {
    // ------------- print prototype ------------- //
    text_.add_gutter() << "template <typename T, typename I>\n";
    text_.add_line(       "__global__");
    text_.add_gutter() << "void " << e->name()
                       << "(" << module_->name() << "_ParamPack<T,I> params_) {";
    text_.add_line();

    if(!e->scope()) { // error: semantic analysis has not been performed
        throw compiler_exception(
            "CUDAPrinter attempt to print APIMethod " + e->name()
            + " for which semantic analysis has not been performed",
            e->location());
    }
    increase_indentation();

    text_.add_line("auto tid_ = threadIdx.x + blockDim.x*blockIdx.x;");
    text_.add_line("auto const grid_step_ = blockDim.x * gridDim.x;");
    text_.add_line("auto const n_ = params_.n;");
    text_.add_line();
    text_.add_line("while(tid_<n_) {");
    increase_indentation();

    text_.add_line("auto gid_ = params_.ni[tid_];");

    print_APIMethod_body(e);

    text_.add_line("tid_ += grid_step_;");

    decrease_indentation();
    text_.add_line("}");

    decrease_indentation();
    text_.add_line("}\n");
}

void CUDAPrinter::print_APIMethod_body(APIMethod* e) {
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

    // declare local variables
    for(auto &var: aliased_variables) {
        auto const& name = var->local->name();
        text_.add_line("auto " + name + " = T{0};");
    }

    // load indexes of ion channels
    auto uses_k  = false;
    auto uses_na = false;
    auto uses_ca = false;
    for(auto &in : e->inputs()) {
        auto ch = in.external->is_indexed_variable()->ion_channel();
        if(!uses_k   && (uses_k  = (ch == k_ion_K)) )
            text_.add_line("auto kid_  = params_.ion_k_idx_[tid_];");
        if(!uses_ca  && (uses_ca = (ch == k_ion_Ca)) )
            text_.add_line("auto caid_ = params_.ion_ca_idx_[tid_];");
        if(!uses_na  && (uses_na = (ch == k_ion_Na)) )
            text_.add_line("auto naid_ = params_.ion_na_idx_[tid_];");
    }

    text_.add_line();

    // insert loads from external state
    for(auto &in : e->inputs()) {
        text_.add_gutter();
        in.local->accept(this);
        text_ << " = ";
        in.external->accept(this);
        text_.end_line(";");
    }

    text_.add_line();

    e->body()->accept(this);

    // insert stores here
    // take care to use atomic operations for the updates for point processes, where
    // more than one thread may try add/subtract to the same memory location
    for(auto &out : e->outputs()) {
        text_.add_gutter();
        if(!is_point_process) {
            out.external->accept(this);
            text_ << (out.op==tok_plus ? " += " : " -= ");
            out.local->accept(this);
        }
        else {
            auto ext = out.external->is_indexed_variable();
            // for now we assume that only matrix variables are updated in this manner
            if(ext->ion_channel() != k_ion_none) {
                throw compiler_exception(
                    "CUDAPrinter : don't know how to update an ion variable this way",
                    out.external->location()
                );
            }

            text_ << (out.op==tok_plus ? "atomicAdd" : "atomicSub") << "(&";
            ext->accept(this);
            text_ << ", ";
            out.local->accept(this);
            text_ << ")";
        }
        text_.end_line(";");
    }

    text_.add_line();
    return;
}

void CUDAPrinter::visit(CallExpression *e) {
    text_ << e->name() << "<T,I>(params_, tid_";
    for(auto& arg: e->args()) {
        text_ << ", ";
        arg->accept(this);
    }
    text_ << ")";
}

void CUDAPrinter::visit(AssignmentExpression *e) {
    e->lhs()->accept(this);
    text_ << " = ";
    e->rhs()->accept(this);
}

void CUDAPrinter::visit(PowBinaryExpression *e) {
    text_ << "std::pow(";
    e->lhs()->accept(this);
    text_ << ", ";
    e->rhs()->accept(this);
    text_ << ")";
}

void CUDAPrinter::visit(BinaryExpression *e) {
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
            throw compiler_exception(
                "CUDAPrinter unsupported binary operator " + yellow(token_string(e->op())),
                e->location());
    }
    rhs->accept(this);
    if(use_brackets) {
        text_ << ")";
    }

    // reset parent precedence
    parent_op_ = pop;
}
