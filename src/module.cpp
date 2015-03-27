#include <fstream>
#include <iostream>
#include <set>

#include "errorvisitor.hpp"
#include "expressionclassifier.hpp"
#include "inline.hpp"
#include "module.hpp"
#include "parser.hpp"

Module::Module(std::string const& fname)
: fname_(fname)
{
    // open the file at the end
    std::ifstream fid;
    fid.open(fname.c_str(), std::ios::binary | std::ios::ate);
    if(!fid.is_open()) { // return if no file opened
        return;
    }

    // determine size of file
    std::size_t size = fid.tellg();
    fid.seekg(0, std::ios::beg);

    // allocate space for storage and read
    buffer_.resize(size+1);
    fid.read(buffer_.data(), size);
    buffer_[size] = 0; // append \0 to terminate string
}

Module::Module(std::vector<char> const& buffer)
{
    buffer_ = buffer;

    // add \0 to end of buffer if not already present
    if(buffer_[buffer_.size()-1] != 0)
        buffer_.push_back(0);
}

std::vector<Module::symbol_ptr>&
Module::procedures() {
    return procedures_;
}

std::vector<Module::symbol_ptr>const&
Module::procedures() const {
    return procedures_;
}

std::vector<Module::symbol_ptr>&
Module::functions() {
    return functions_;
}

std::vector<Module::symbol_ptr>const&
Module::functions() const {
    return functions_;
}

Module::symbol_map&
Module::symbols() {
    return symbols_;
}

Module::symbol_map const&
Module::symbols() const {
    return symbols_;
}

void Module::error(std::string const& msg, Location loc) {
    std::string location_info = pprintf("%:% ", file_name(), loc);
    if(status_==k_compiler_error) {
        // append to current string
        error_string_ += "\n" + white(location_info) + msg;
    }
    else {
        error_string_ = white(location_info) + msg;
        status_ = k_compiler_error;
    }
}

bool Module::semantic() {
    ////////////////////////////////////////////////////////////////////////////
    // create the symbol table
    // there are three types of symbol to look up
    //  1. variables
    //  2. function calls
    //  3. procedure calls
    // the symbol table is generated, then we can traverse the AST and verify
    // that all symbols are correctly used
    ////////////////////////////////////////////////////////////////////////////

    // first add variables
    add_variables_to_symbols();

    // Check for errors when adding variables to the symbol table
    // Don't quit if there is an error, instead continue with the
    // semantic analysis so that error messages for other errors can
    // be displayed
    if(status() == k_compiler_error) {
        std::cerr << red("error: ") << error_string_ << std::endl;
    }

    // add functions and procedures
    for(auto& f: functions_) {
        // check to see if the symbol has already been defined
        bool is_found = (symbols_.find(f->name()) != symbols_.end());
        if(is_found) {
            error(
                pprintf("function '%' clashes with previously defined symbol",
                        f->name()),
                f->location()
            );
            return false;
        }
        // add symbol to table
        symbols_[f->name()] = std::move(f);
    }
    for(auto& proc: procedures_) {
        bool is_found = (symbols_.find(proc->name()) != symbols_.end());
        if(is_found) {
            error(
                pprintf("procedure '%' clashes with previously defined symbol",
                        proc->name()),
                proc->location()
            );
            return false;
        }
        // add symbol to table
        symbols_[proc->name()] = std::move(proc);
    }

    ////////////////////////////////////////////////////////////////////////////
    // now iterate over the functions and procedures and perform semantic
    // analysis on each. This includes
    //  -   variable, function and procedure lookup
    //  -   generate local variable table
    ////////////////////////////////////////////////////////////////////////////
    int errors = 0;
    for(auto& e : symbols_) {
        auto& s = e.second;

        if( s->kind() == k_symbol_function || s->kind() == k_symbol_procedure ) {
            // first perform semantic analysis
            s->semantic(symbols_);
            // then use an error visitor to print out all the semantic errors
            ErrorVisitor* v = new ErrorVisitor(file_name());
            s->accept(v);
            errors += v->num_errors();
            delete v;
        }
    }

    if(errors) {
        std::cout << "\nthere were " << errors
                  << " errors in the semantic analysis" << std::endl;
        status_ = k_compiler_error;
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////
    //  generate the metadata required to make the init block
    //
    //  we want to create a new procedure expression by
    //      - remove all local declarations
    //      - clone all expressions
    //      - generate new scope
    //          - keep global symbols
    //          - generate new local symbols
    //      - inline procedures
    //          - localize variables
    //
    //  we can the do multiple rounds of constant folding, eliminating variables
    //  as we go. The point of this would be that once performed, the arrays
    //  hinf/minf in KdShy2007 would be indicated as being not used on rhs by
    //  any methods, and thus as being safe for removal
    ////////////////////////////////////////////////////////////////////////////

    auto add_output = []
        (APIMethod* p, TOK op, std::string const& local, std::string const& global)
    {
        auto l = p->scope()->find(local);
        auto g = p->scope()->find_global(global);
        if(!l || !g) {
            throw compiler_exception(
                  "unable to add store operation "
                  + yellow(local) + " -> " + yellow(global) + " to " + yellow(p->name())
                  + (l ? "" : " : " + yellow(local)  + " is not a variable")
                  + (g ? "" : " : " + yellow(global) + " is not a global variable"),
                  p->location());
        }
        p->outputs().push_back({op, l, g});
    };
    auto add_input = []
        (APIMethod* p, TOK op, std::string const& local, std::string const& global)
    {
        auto l = p->scope()->find(local);
        auto g = p->scope()->find_global(global);
        if(!l || !g) {
            throw compiler_exception(
                    "unable to add load operation "
                    + yellow(local) + " <- " + yellow(global) + " to " + yellow(p->name())
                    + (l ? "" : " : " + yellow(local)  + " is not a variable")
                    + (g ? "" : " : " + yellow(global) + " is not a global variable"),
                    p->location());
        }
        p->inputs().push_back({op, l, g});
    };

    if( has_symbol("initial", k_symbol_procedure) ) {
        // clone the initial procedure
        auto initial = symbols_["initial"]->is_procedure();
        auto loc = initial->location();
        auto proc_init =
            new APIMethod(initial->location(),
                          "nrn_init",
                          std::vector<expression_ptr>(), // no arguments
                          make_expression<BlockExpression>(loc,
                                                           std::list<expression_ptr>(),
                                                           false)
                         );

        if( symbols_.find("nrn_init")!=symbols_.end() ) {
            error("'nrn_init' clashes with reserved name, please rename it",
                  loc);
            return false;
        }
        symbols_["nrn_init"] = symbol_ptr(proc_init);

        // get a reference to the empty body of the init function
        auto& body = proc_init->body()->body();
        for(auto& e : *(initial->body())) {
            body.emplace_back(e->clone());
        }
        // perform semantic analysis for init
        proc_init->semantic(symbols_);

        // set input vec_v
        proc_init->inputs().push_back({tok_eq, proc_init->scope()->find("v"), symbols_["vec_v"].get()});
    }
    else {
        error("an INITIAL block is required", Location());
        return false;
    }
    if( has_symbol("breakpoint", k_symbol_procedure) ) {
        auto breakpoint = symbols_["breakpoint"]->is_procedure();
        auto loc = breakpoint->location();

        // helper for making identifiers on the fly
        auto id = [] (std::string const& name) {
            return make_expression<IdentifierExpression>(Location(), name);
        };
        //..........................................................
        //..........................................................
        // nrn_state : The temporal integration of state variables
        //..........................................................
        //..........................................................

        // find the SOLVE statement
        SolveExpression* sexp = nullptr;
        for(auto& e: *(breakpoint->body())) {
            sexp = e->is_solve_statement();
            if(sexp) break;
        }

        // handle the case where there is no SOLVE in BREAKPOINT
        if( sexp==nullptr ) {
            std::cout << purple("warning:")
                      << " there is no SOLVE statement, required to update the"
                      << " state variables, in the BREAKPOINT block"
                      << std::endl;
            auto proc_state =
                new APIMethod(
                        loc,
                        "nrn_state",
                        {},
                        make_expression<BlockExpression>
                            (loc, std::list<expression_ptr>(), false)
                    );

            // perform semantic analysis for init
            proc_state->semantic(symbols_);

            symbols_["nrn_state"] = symbol_ptr{proc_state};
        }
        else {
            // get the DERIVATIVE block
            auto dblock = sexp->procedure();
            auto proc_state =
                new APIMethod(
                        dblock->location(),
                        "nrn_state",
                        {},
                        make_expression<BlockExpression>
                            (loc, std::list<expression_ptr>(), false)
                    );
            // check for symbol clash
            if( symbols_.find("nrn_state")!=symbols_.end() ) {
                error("'nrn_state' clashes with reserved name, please rename it",
                      loc);
                return false;
            }

            // get a reference to the empty body of the init function
            auto& body = proc_state->body()->body();
            for(auto& e : *(dblock->body())) {
                if(auto ass = e->is_assignment()) {
                    auto lhs = ass->lhs();
                    auto rhs = ass->rhs();
                    if(auto deriv = lhs->is_derivative()) {
                        auto sym  = deriv->symbol();
                        auto name = deriv->name();

                        // create visitor for lineary analysis
                        auto v = make_unique<ExpressionClassifierVisitor>(sym);

                        // analyse the rhs
                        rhs->accept(v.get());

                        // quit if ODE is not linear
                        if( v->classify() != k_expression_lin ) {
                            error("unable to integrate nonlinear state ODEs",
                                  rhs->location());
                            return false;
                        }

                        // the linear differential equation is of the form
                        // s' = a*s + b
                        // integration by separation of variables gives the following
                        // update function to integrate s for one time step dt
                        // s = -b/a + (s+b/a)*exp(a*dt)
                        // we are going to build this update function by
                        //  1. generating statements that define a_=a and ba_=b/a
                        //  2. generating statements that update the solution
                        // TODO : pass without cloning?
                        // statement : a_ = a
                        auto stmt_a  =
                            binary_expression(Location(),
                                              tok_eq,
                                              id("a_"),
                                              v->linear_coefficient()->clone());

                        // expression : b/a
                        auto expr_ba =
                            binary_expression(Location(),
                                              tok_divide,
                                              v->constant_term()->clone(),
                                              id("a_"));
                        // statement  : ba_ = b/a
                        auto stmt_ba = binary_expression(Location(), tok_eq, id("ba_"), std::move(expr_ba));

                        // the update function
                        auto e_string = name + "  = -ba_ + "
                                        "(" + name + " + ba_)*exp(a_*dt)";
                        auto stmt_update = Parser(e_string).parse_line_expression();

                        // add declaration of local variables
                        body.emplace_back(Parser("LOCAL a_").parse_local());
                        body.emplace_back(Parser("LOCAL ba_").parse_local());
                        // add integration statements
                        body.emplace_back(std::move(stmt_a));
                        body.emplace_back(std::move(stmt_ba));
                        body.emplace_back(std::move(stmt_update));
                        continue;
                    }
                    else {
                        body.push_back(e->clone());
                        continue;
                    }
                }
                body.push_back(e->clone());
            }

            symbols_["nrn_state"] = symbol_ptr{proc_state};
            proc_state->semantic(symbols_);

            // set input vec_v
            add_input(proc_state, tok_eq, "v", "vec_v");
        }

        //..........................................................
        //..........................................................
        // nrn_current : update contributions to currents
        //..........................................................
        //..........................................................
        std::list<expression_ptr> block;

        auto is_ion_update = [] (Expression* e) -> VariableExpression* {
            if(auto a = e->is_assignment()) {
                // semantic analysis has been performed on the original expression
                // which means that the lhs is an identifier, and that it is a variable
                // because it has to be 
                if(auto var = a->lhs()->is_identifier()->symbol()->is_variable()) {
                    return var ? (var->is_ion() ? var : nullptr)
                               : nullptr;
                }
            }
            return nullptr;
        };

        // add statements that initialize the reduction variables
        bool update_current = false;
        std::list<std::string> outputs;
        for(auto& e: *(breakpoint->body())) {
            // ignore solve statements
            if(e->is_solve_statement()) continue;

            // add the expression
            block.emplace_back(e->clone());

            // we are updating an ionic current
            // so keep track of current and conductance accumulation
            if(auto var = is_ion_update(e.get())) {
                auto lhs = e->is_assignment()->lhs()->is_identifier();
                auto rhs = e->is_assignment()->rhs();

                // analyze the expression for linear terms
                auto v = make_unique<ExpressionClassifierVisitor>(symbols_["v"].get());
                rhs->accept(v.get());

                if(v->classify()==k_expression_lin) {
                    block.emplace_back(Parser("LOCAL current_").parse_local());
                    block.emplace_back(Parser("LOCAL conductance_").parse_local());

                    // add current update
                    std::string e_string = "current_ = current_ + " + var->name();
                    block.emplace_back(Parser(e_string).parse_line_expression());

                    auto g_stmt =
                        binary_expression(tok_eq,
                                          id("conductance_"),
                                          binary_expression(tok_plus,
                                                            id("conductance_"),
                                                            v->linear_coefficient()->clone()));
                    block.emplace_back(std::move(g_stmt));
                }
                else {
                    error("current update functions must be a linear function of v : " + rhs->to_string(),
                          e->location());
                    return false;
                }
                // nonspecific currents are not written out to an external
                // ion channel
                if(var->ion_channel()!=k_ion_nonspecific) {
                    outputs.push_back(lhs->name());
                }
                update_current = true;
            }
        }
        //if(update_current) {
        //    block.push_back(Parser("g_ = conductance_").parse_line_expression());
        //}
        auto v = make_unique<ConstantFolderVisitor>();
        for(auto& e : block) {
            e->accept(v.get());
        }

        auto proc_current =
            new APIMethod(
                    breakpoint->location(),
                    "nrn_current",
                    {},
                    make_expression<BlockExpression>(loc, std::move(block), false)
                );

        symbols_["nrn_current"] = symbol_ptr(proc_current);
        proc_current->semantic(symbols_);

        auto scp = proc_current->scope();
        // now set up the input/output tables with full semantic information
        for(auto &var: outputs) {
            add_output(proc_current, tok_plus, var, "ion_"+var);
        }

        // only output contribution to RHS if there is one to make
        if(update_current) {
            add_output(proc_current, tok_minus, "current_",     "vec_rhs");
            add_output(proc_current, tok_plus,  "conductance_", "vec_d");
            if(outputs.size()>0) {
                // only need input ionic values if external ion channel dependency
                // assume that all input ion variables are used
                for(auto& var: symbols_) {
                    auto e = var.second->is_variable();
                    if( e && e->is_ion() && e->is_readable()) {
                        add_input(proc_current, tok_eq, e->name(), "ion_"+e->name());
                    }
                }
            }
        }
        // set input vec_v
        proc_current->inputs().push_back({tok_eq, scp->find("v"), symbols_["vec_v"].get()});
    }
    else {
        error("a BREAKPOINT block is required", Location());
        return false;
    }
    //..........................................................
    //..........................................................
    // nrn_jacob : add conductance g to diagonal VEC_D
    //..........................................................
    //..........................................................
    // remove, because we fold the rhs update into nrn_current

    // apply semantic analysis to the entries in the output and input indexes
    for(auto &s : symbols_) {
        if(auto method = s.second->is_api_method()) {
            auto scope = method->scope();
            for(auto &in: method->inputs()) {
                in.local->semantic(scope);
                in.external->semantic(scope);
            }
            for(auto &out: method->outputs()) {
                out.local->semantic(scope);
                out.external->semantic(scope);
            }
        }
    }

    return status() == k_compiler_happy;
}

/// populate the symbol table with class scope variables
void Module::add_variables_to_symbols() {
    // add reserved symbols (not v, because for some reason it has to be added
    // by the user)
    auto t = new VariableExpression(Location(), "t");
    t->state(false);            t->linkage(k_local_link);
    t->ion_channel(k_ion_none); t->range(k_scalar);
    t->access(k_read);          t->visibility(k_global_visibility);
    symbols_["t"]    = symbol_ptr{t};

    auto dt = new VariableExpression(Location(), "dt");
    dt->state(false);            dt->linkage(k_local_link);
    dt->ion_channel(k_ion_none); dt->range(k_scalar);
    dt->access(k_read);          dt->visibility(k_global_visibility);
    symbols_["dt"]    = symbol_ptr{dt};

    auto g_ = new VariableExpression(Location(), "g_");
    g_->state(false);            g_->linkage(k_local_link);
    g_->ion_channel(k_ion_none); g_->range(k_range);
    g_->access(k_readwrite);     g_->visibility(k_global_visibility);
    symbols_["g_"]    = symbol_ptr{g_};

    // add indexed variables to the table
    auto vec_rhs = new IndexedVariable(Location(), "vec_rhs");
    vec_rhs->access(k_write);
    vec_rhs->ion_channel(k_ion_none);
    symbols_["vec_rhs"] = symbol_ptr{vec_rhs};

    auto vec_d = new IndexedVariable(Location(), "vec_d");
    vec_d->access(k_write);
    vec_d->ion_channel(k_ion_none);
    symbols_["vec_d"] = symbol_ptr{vec_d};

    auto vec_v = new IndexedVariable(Location(), "vec_v");
    vec_v->access(k_read);
    vec_v->ion_channel(k_ion_none);
    symbols_["vec_v"] = symbol_ptr{vec_v};

    // add state variables
    for(auto const &var : state_block()) {
        VariableExpression *id = new VariableExpression(Location(), var);

        id->state(true);    // set state to true
        // state variables are private
        //      what about if the state variables is an ion concentration?
        id->linkage(k_local_link);
        id->visibility(k_local_visibility);
        id->ion_channel(k_ion_none);    // no ion channel
        id->range(k_range);             // always a range
        id->access(k_readwrite);

        symbols_[var] = symbol_ptr{id};
    }

    // add the parameters
    for(auto const& var : parameter_block()) {
        auto name = var.name();
        VariableExpression *id = new VariableExpression(Location(), name);

        id->state(false);           // never a state variable
        id->linkage(k_local_link);
        // parameters are visible to Neuron
        id->visibility(k_global_visibility);
        id->ion_channel(k_ion_none);
        // scalar by default, may later be upgraded to range
        id->range(k_scalar);
        id->access(k_read);

        // check for 'special' variables
        if(name == "v") { // global voltage values
            id->linkage(k_extern_link);
            id->range(k_range);
            // argh, the global version cannot be modified, however
            // the local ghost is sometimes modified
            // make this illegal, because it is probably sloppy programming
            id->access(k_read);
        } else if(name == "celcius") { // global celcius parameter
            id->linkage(k_extern_link);
        }

        // set default value if one was specified
        if(var.value.size()) {
            id->value(std::stod(var.value));
        }

        symbols_[name] = symbol_ptr{id};
    }

    // add the assigned variables
    for(auto const& var : assigned_block()) {
        auto name = var.name();
        VariableExpression *id = new VariableExpression(Location(), name);

        id->state(false);           // never a state variable
        id->linkage(k_local_link);
        // local visibility by default
        id->visibility(k_local_visibility);
        id->ion_channel(k_ion_none); // can change later
        // ranges because these are assigned to in loop
        id->range(k_range);
        id->access(k_readwrite);

        symbols_[name] = symbol_ptr{id};
    }

    ////////////////////////////////////////////////////
    // parse the NEURON block data, and use it to update
    // the variables in symbols_
    ////////////////////////////////////////////////////
    // first the ION channels
    // check for nonspecific current
    if( neuron_block().has_nonspecific_current() ) {
        auto const& i = neuron_block().nonspecific_current;
        auto id = symbols_[i.spelling]->is_variable();
        if(id==nullptr) {
            error( pprintf(
                        "nonspecific current % must be declared as "
                        " declared as PARAMETER or ASSIGNED",
                        yellow(i.spelling)),
                    i.location); // location of definition
        }
        std::string name = id->name();
        if(name[0] != 'i') {
            error( pprintf(
                       "nonspecific current % does not start with 'i'",
                       yellow(i.spelling)),
                   i.location); // location of definition
        }
        id->access(k_readwrite);
        id->visibility(k_global_visibility);
        id->ion_channel(k_ion_nonspecific);
    }
    for(auto const& ion : neuron_block().ions) {
        // assume that the ion channel variable has already been declared
        // we check for this, and throw an error if not
        for(auto const& var : ion.read) {
            auto id = symbols_[var]->is_variable();
            if(id==nullptr) { // assert that variable is already set
                error( pprintf(
                        "variable % from ion channel % has to be"
                        " declared as PARAMETER or ASSIGNED",
                         yellow(var), yellow(ion.name)
                        ),
                        Location() // empty location
                );
                return;
            }
            id->access(k_read);
            id->visibility(k_global_visibility);
            id->ion_channel(ion.kind());

            // add the ion variable's indexed shadow
            auto shadow_id = new IndexedVariable(Location(), "ion_"+id->name());
            shadow_id->access(k_read);
            shadow_id->ion_channel(ion.kind());
            symbols_[shadow_id->name()] = symbol_ptr{shadow_id};
        }
        for(auto const& var : ion.write) {
            auto id = symbols_[var]->is_variable();
            if(id==nullptr) { // assert that variable is already set
                error( pprintf(
                        "variable % from ion channel % has to be"
                        " declared as PARAMETER or ASSIGNED",
                         yellow(var), yellow(ion.name)
                        ),
                        Location() // empty location
                );
                return;
            }
            id->access(k_write);
            id->visibility(k_global_visibility);
            id->ion_channel(ion.kind());

            // add the ion variable's indexed shadow
            auto shadow_id = new IndexedVariable(Location(), "ion_"+id->name());
            shadow_id->access(k_write);
            shadow_id->ion_channel(ion.kind());
            symbols_[shadow_id->name()] = symbol_ptr{shadow_id};
        }
    }
    // then GLOBAL variables
    for(auto const& var : neuron_block().globals) {
        if(!symbols_[var.spelling]) {
            error( yellow(var.spelling) +
                   " is declared as GLOBAL, but has not been declared in the" +
                   " ASSIGNED block",
                   var.location);
            return;
        }
        auto id = symbols_[var.spelling]->is_variable();
        if(!id) {
            throw compiler_exception(
                "unable to find symbol '" + var.spelling + "' in symbols",
                Location());
        }
        id->visibility(k_global_visibility);
    }

    // then RANGE variables
    for(auto const& var : neuron_block().ranges) {
        if(!symbols_[var.spelling]) {
            error( yellow(var.spelling) +
                   " is declared as RANGE, but has not been declared in the" +
                   " ASSIGNED or PARAMETER block",
                   var.location);
            return;
        }
        auto id = symbols_[var.spelling]->is_variable();
        if(!id) {
            throw compiler_exception(
                "unable to find symbol '" + var.spelling + "' in symbols",
                Location());
        }
        id->range(k_range);
    }
}

bool Module::optimize() {
    // how to structure the optimizer
    // loop over APIMethods
    //      - apply optimization to each in turn
    auto folder = make_unique<ConstantFolderVisitor>();
    for(auto &symbol : symbols_) {
        auto kind = symbol.second->kind();
        BlockExpression* body;
        if(kind == k_symbol_procedure) {
            // we are only interested in true procedurs and APIMethods
            auto proc = symbol.second->is_procedure();
            auto pkind = proc->kind();
            if(pkind == k_proc_normal || pkind == k_proc_api )
                body = symbol.second->is_procedure()->body();
            else
                continue;
        }
        // for now don't look at functions
        //else if(kind == k_symbol_function) {
        //    body = symbol.second.expression->is_function()->body();
        //}
        else {
            continue;
        }

        /////////////////////////////////////////////////////////////////////
        // loop over folding and propogation steps until there are no changes
        /////////////////////////////////////////////////////////////////////

        // perform constant folding
        for(auto& line : *body) {
            line->accept(folder.get());
        }

        // preform expression simplification
        // i.e. removing zeros/refactoring reciprocals/etc

        // perform constant propogation

        /////////////////////////////////////////////////////////////////////
        // remove dead local variables
        /////////////////////////////////////////////////////////////////////

        /////////////////////////////////////////////////////////////////////
        // tag ghost fields in APIMethods
        /////////////////////////////////////////////////////////////////////
        if(kind == k_symbol_procedure) {
            auto proc = symbol.second->is_api_method();
            if(proc && this->kind()==k_module_point) {
                for(auto &out: proc->outputs()) {
                    // by definition the output has to be an indexed variable
                    if(out.local->kind() == k_symbol_local) {
                        out.local->set_kind(k_symbol_ghost);
                    }
                }
            }
        }

    }

    return true;
}

