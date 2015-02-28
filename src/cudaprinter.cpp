#include "cudaprinter.hpp"
#include "lexer.h"

/******************************************************************************
                              TextBuffer
******************************************************************************/
TextBuffer& TextBuffer::add_gutter() {
    text_ << gutter_;
    return *this;
}
void TextBuffer::add_line(std::string const& line) {
    text_ << gutter_ << line << std::endl;
}
void TextBuffer::add_line() {
    text_ << std::endl;
}
void TextBuffer::end_line(std::string const& line) {
    text_ << line << std::endl;
}
void TextBuffer::end_line() {
    text_ << std::endl;
}

std::string TextBuffer::str() const {
    return text_.str();
}

void TextBuffer::set_gutter(int width) {
    indent_ = width;
    gutter_ = std::string(indent_, ' ');
}

void TextBuffer::increase_indentation() {
    indent_ += indentation_width_;
    if(indent_<0) {
        indent_=0;
    }
    gutter_ = std::string(indent_, ' ');
}
void TextBuffer::decrease_indentation() {
    indent_ -= indentation_width_;
    if(indent_<0) {
        indent_=0;
    }
    gutter_ = std::string(indent_, ' ');
}
std::stringstream& TextBuffer::text() {
    return text_;
}

/******************************************************************************
                              CUDAPrinter driver
******************************************************************************/

CUDAPrinter::CUDAPrinter(Module &m, bool o) {
    optimize_ = o;

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
    text_ << "#include <cmath>\n";
    text_ << "#include <limits>\n\n";
    text_ << "#include <indexedview.hpp>\n";
    text_ << "#include <mechanism.hpp>\n";
    text_ << "#include <target.hpp>\n\n";

    ////////////////////////////////////////////////////////////
    // generate the parameter pack
    ////////////////////////////////////////////////////////////
    std::vector<std::string> param_pack;
    text_ << "template <typename T, typename T>\n";
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
            text_ << "  T* ion_" + field + "_;\n";
            param_pack.push_back(tname + "." + field + ".data()");
        }
        for(auto& field : ion.write) {
            text_ << "  T* ion_" + field + "_;\n";
            param_pack.push_back(tname + "." + field + ".data()");
        }
        text_ << "  I* ion_" + ion.name + "_idx_;\n";
        param_pack.push_back(tname + ".index.data()");
    }

    text_ << "\n  // matrix\n";
    text_ << "  vec_rhs;\n";
    text_ << "  vec_d;\n";
    text_ << "  vec_v;\n";
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
    text_ << "namespace impl {\n";
    text_ << "namespace " + m.name() + " {\n";

    {
        auto v = new CUDAPrinterVisitor(&m, optimize_);
        v->increase_indentation();
        // print stubs that call API method kernels that are defined in the
        // imp::name namespace
        auto proctest = [] (procedureKind k) {return k == k_proc_normal;};
        for(auto const &var : m.symbols()) {
            if(   var.second.kind==k_symbol_procedure
            && proctest(var.second.expression->is_procedure()->kind()))
            {
                var.second.expression->accept(v);
            }
        }
        v->decrease_indentation();
        text_ << v->text();
    }

    text_ << "} // namespace " + m.name() + "\n";
    text_ << "} // namespace impl\n";

    //////////////////////////////////////////////
    //////////////////////////////////////////////
    std::string class_name = "Mechanism_" + m.name();

    text_ << "template<typename T, typename I>\n";
    text_ << "class " + class_name + " : public Mechanism<T, I, targetKind::gpu> {\n";
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

    text_ << "    size_type memory() const override {\n";
    text_ << "        return node_indices_.size()*sizeof(value_type)*" << num_vars << ";\n";
    text_ << "    }\n\n";

    text_ << "    void set_params(value_type t_, value_type dt_) override {\n";
    text_ << "        t = t_;\n";
    text_ << "        dt = dt_;\n";
    text_ << "        param_pack_ = " << m.name() << "_ParamPack{\n";
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

    auto v = new CUDAPrinterVisitor(&m, optimize_);
    v->increase_indentation();
    // print stubs that call API method kernels that are defined in the
    // imp::name namespace
    auto proctest = [] (procedureKind k) {return k == k_proc_api;};
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


/******************************************************************************
                              CUDAPrinterVisitor
******************************************************************************/

void CUDAPrinterVisitor::visit(Expression *e) {
    std::cout << "CUDAPrinterVisitor :: error printing : " << e->to_string() << std::endl;
    assert(false);
}

void CUDAPrinterVisitor::visit(LocalExpression *e) {
}

void CUDAPrinterVisitor::visit(NumberExpression *e) {
    text_ << " " << e->value();
}

void CUDAPrinterVisitor::visit(IdentifierExpression *e) {
    if(auto var = e->variable()) {
        var->accept(this);
    }
}

void CUDAPrinterVisitor::visit(VariableExpression *e) {
    text_ << "params." << e->name();
    if(e->is_range()) {
        text_ << "[tid_]";
    }
}

void CUDAPrinterVisitor::visit(IndexedVariable *e) {
    // here we have to do some more specific tests
    // is it an ion or matrix?
    text_ << "params." << e->name() << "[gid_]";
}

void CUDAPrinterVisitor::visit(UnaryExpression *e) {
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

void CUDAPrinterVisitor::visit(BlockExpression *e) {
    // ------------- declare local variables ------------- //
    // only if this is the outer block
    if(!e->is_nested()) {
        std::vector<std::string> names;
        for(auto var : e->scope()->locals()) {
            if(var.second.kind == k_symbol_local)
                names.push_back(var.first);
        }
        if(names.size()>0) {
            text_.add_gutter() << "T " << names[0];
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

void CUDAPrinterVisitor::visit(IfExpression *e) {
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

void CUDAPrinterVisitor::visit(ProcedureExpression *e) {
    // ------------- print prototype ------------- //
    text_.add_gutter() << "template <typename T, typename I>\n";
    text_.add_line("__device__");
    text_.add_gutter() << "void " << e->name()
                       << "(ParamPack_" << module_->name() << "<T,I> const& params_,"
                       << "const int tid_";
    for(auto arg : e->args()) {
        text_ << ", T " << arg->is_argument()->name();
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

void CUDAPrinterVisitor::visit(APIMethod *e) {
    // ------------- print prototype ------------- //
    text_.add_gutter() << "void " << e->name() << "() {";
    text_.end_line();

    assert(e->scope()!=nullptr); // error: semantic analysis has not been performed

    text_.add_line("{");
    increase_indentation();

    // print the CUDA set up headers
    text_.add_line("auto n = size();");
    text_.add_line("auto thread_dim = 192;");
    text_.add_line("dim3 dim_block(thread_dim);");
    text_.add_line("dim3 dim_grid(n/dim_block.x + (n%dim_block.x ? 1 : 0) );");
    text_.add_line();

    text_.add_line("START_PROFILE");
    text_.add_gutter() << "//impl::"
                       << module_->name()
                       << "::current_kernel<T,I>"
                       << "<<<dim_grid, dim_block>>>(param_pack_);";
    text_.add_line();
    text_.add_line("STOP_PROFILE");
    decrease_indentation();
    text_.add_line("}\n");
}

void CUDAPrinterVisitor::print_APIMethod_unoptimized(APIMethod* e) {
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
        in.local.expression->accept(this);
        text_ << " = ";
        in.external.expression->accept(this);
        text_.end_line(";");
    }

    e->body()->accept(this);

    // insert stores here
    for(auto &out : e->outputs()) {
        text_.add_gutter();
        out.external.expression->accept(this);
        text_ << (out.op==tok_plus ? " += " : " -= ");
        out.local.expression->accept(this);
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

void CUDAPrinterVisitor::print_APIMethod_optimized(APIMethod* e) {
    // ------------- get mechanism properties ------------- //
    bool is_point_process = module_->kind() == k_module_point;

    // :: analyse outputs, to determine if they depend on any
    //    ghost fields.
    std::vector<MemOp*> aliased_variables;
    if(is_point_process) {
        for(auto &out : e->outputs()) {
            if(out.local.kind == k_symbol_ghost) {
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
        text_.add_line("value_type " + out->local.expression->is_identifier()->name() + "[BSIZE];");
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

    // insert loads from external state here
    for(auto &in : e->inputs()) {
        text_.add_gutter();
        in.local.expression->accept(this);
        text_ << " = ";
        in.external.expression->accept(this);
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
        out.external.expression->accept(this);
        text_ << (out.op==tok_plus ? " += " : " -= ");
        out.local.expression->accept(this);
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

    // insert loads from external state here
    for(auto &in : e->inputs()) {
        text_.add_gutter();
        in.local.expression->accept(this);
        text_ << " = ";
        in.external.expression->accept(this);
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
        out.external.expression->accept(this);
        text_ << (out.op==tok_plus ? " += " : " -= ");
        out.local.expression->accept(this);
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

void CUDAPrinterVisitor::visit(CallExpression *e) {
    text_ << e->name() << "(i_";
    for(auto& arg: e->args()) {
        text_ << ", ";
        arg->accept(this);
    }
    text_ << ")";
}

void CUDAPrinterVisitor::visit(AssignmentExpression *e) {
    e->lhs()->accept(this);
    text_ << " = ";
    e->rhs()->accept(this);
}

void CUDAPrinterVisitor::visit(PowBinaryExpression *e) {
    text_ << "std::pow(";
    e->lhs()->accept(this);
    text_ << ", ";
    e->rhs()->accept(this);
    text_ << ")";
}

void CUDAPrinterVisitor::visit(BinaryExpression *e) {
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

