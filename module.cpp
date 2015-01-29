#include <fstream>
#include <iostream>
#include <set>

#include "errorvisitor.h"
#include "expressionclassifier.h"
#include "inline.h"
#include "module.h"
#include "parser.h"

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

std::vector<Expression*>&
Module::procedures() {
    return procedures_;
}

std::vector<Expression*>const&
Module::procedures() const {
    return procedures_;
}

std::vector<Expression*>&
Module::functions() {
    return functions_;
}

std::vector<Expression*>const&
Module::functions() const {
    return functions_;
}

std::unordered_map<std::string, Symbol>&
Module::symbols() {
    return symbols_;
}

std::unordered_map<std::string, Symbol>const&
Module::symbols() const {
    return symbols_;
}

void Module::error(std::string const& msg, Location loc) {
    std::string location_info = pprintf("%:% ", name(), loc);
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
    for(auto f: functions_) {
        auto fun = static_cast<FunctionExpression*>(f);
        // check to see if the symbol has already been defined
        bool is_found = (symbols_.find(fun->name()) != symbols_.end());
        if(is_found) {
            error(
                pprintf("function '%' clashes with previously defined symbol",
                        fun->name()),
                fun->location()
            );
            return false;
        }
        // add symbol to table
        symbols_[fun->name()] = Symbol(k_symbol_function, f);
    }
    for(auto p: procedures_) {
        auto proc = static_cast<ProcedureExpression*>(p);
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
        symbols_[proc->name()] = Symbol(k_symbol_procedure, p);
    }

    ////////////////////////////////////////////////////////////////////////////
    // now iterate over the functions and procedures and perform semantic
    // analysis on each. This includes
    //  -   variable, function and procedure lookup
    //  -   generate local variable table
    ////////////////////////////////////////////////////////////////////////////
    int errors = 0;
    for(auto e : symbols_) {
        Symbol s = e.second;

        if( s.kind == k_symbol_function || s.kind == k_symbol_procedure ) {
            // first perform semantic analysis
            s.expression->semantic(symbols_);
            // then use an error visitor to print out all the semantic errors
            ErrorVisitor* v = new ErrorVisitor(name());
            s.expression->accept(v);
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
    if( has_symbol("initial", k_symbol_procedure) ) {
        // clone the initial procedure
        auto initial = symbols_["initial"].expression->is_procedure();
        auto loc = initial->location();
        auto proc_init = new APIMethod(
                            initial->location(),
                            "nrn_init",
                            {}, // no arguments
                            new BlockExpression(loc, {}, false));

        if( symbols_.find("nrn_init")!=symbols_.end() ) {
            error("'nrn_init' clashes with reserved name, please rename it",
                  loc);
            return false;
        }
        symbols_["nrn_init"] = Symbol(k_symbol_procedure, proc_init);

        // get a reference to the empty body of the init function
        auto& body = proc_init->body()->body();
        for(auto e : *(initial->body())) {
            if(e->is_local_declaration()) continue;
            body.push_back(e->clone());
        }
        // perform semantic analysis for init
        proc_init->semantic(symbols_);

        // set input vec_v
        proc_init->inputs().push_back({tok_eq, proc_init->scope()->find("v"), symbols_["vec_v"]});
        procedures_.push_back(proc_init);
    }
    else {
        error("an INITIAL block is required", Location());
        return false;
    }
    if( has_symbol("breakpoint", k_symbol_procedure) ) {
        auto breakpoint = symbols_["breakpoint"].expression->is_procedure();
        auto loc = breakpoint->location();

        // helper for making identifiers on the fly
        auto id = [] (std::string const& name) {
            return new IdentifierExpression(Location(), name);
        };
        //..........................................................
        //..........................................................
        // nrn_state : The temporal integration of state variables
        //..........................................................
        //..........................................................

        // find the SOLVE statement
        SolveExpression* sexp = nullptr;
        for(auto e: *(breakpoint->body())) {
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
                        new BlockExpression(loc, {}, false)
                    );

            // perform semantic analysis for init
            proc_state->semantic(symbols_);

            procedures_.push_back(proc_state);
            symbols_["nrn_state"] = Symbol(k_symbol_procedure, proc_state);
        }
        else {
            // get the DERIVATIVE block
            auto dblock = sexp->procedure();
            auto proc_state =
                new APIMethod(
                        dblock->location(),
                        "nrn_state",
                        {},
                        new BlockExpression(loc, {}, false)
                    );
            // check for symbol clash
            if( symbols_.find("nrn_state")!=symbols_.end() ) {
                error("'nrn_state' clashes with reserved name, please rename it",
                      loc);
                return false;
            }

            // get a reference to the empty body of the init function
            bool has_derivaties = false;
            auto& body = proc_state->body()->body();
            for(auto e : *(dblock->body())) {
                if(e->is_local_declaration()) continue;
                if(e->is_assignment()) {
                    auto lhs = e->is_assignment()->lhs();
                    auto rhs = e->is_assignment()->rhs();
                    if(lhs->is_derivative()) {
                        has_derivaties = true;
                        auto sym = lhs->is_derivative()->symbol();
                        auto name = lhs->is_derivative()->name();

                        // create visitor for lineary analysis
                        auto v = new ExpressionClassifierVisitor(sym);

                        // analyse the rhs
                        rhs->accept(v);

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
                        auto a = v->linear_coefficient();
                        auto id_a  = id("a_");
                        // statement : a_ = a
                        auto stmt_a  = binary_expression(Location(), tok_eq, id_a, a);

                        // define ba
                        auto b = v->constant_term();
                        auto id_ab = id("ba_");
                        // expression : b/a
                        auto expr_ba = binary_expression(Location(), tok_divide, b, id_a);
                        // statement  : ba_ = b/a
                        auto stmt_ba = binary_expression(Location(), tok_eq, id_ab, expr_ba);

                        // the update function
                        auto e_string = name + "  = -ba_ + "
                                        "(" + name + " + ba_)*exp(a_*dt)";
                        auto stmt_update = Parser(e_string).parse_line_expression();

                        body.push_back(stmt_a);
                        body.push_back(stmt_ba);
                        body.push_back(stmt_update);
                        continue;
                    }
                    else {
                        body.push_back(e->clone());
                    }
                }
                body.push_back(e->clone());
            }

            symbols_["nrn_state"] = Symbol(k_symbol_procedure, proc_state);
            proc_state->semantic(symbols_);

            // if there are derivative expressions, we need to add symbols for
            // local variables a_ and ab_ to the symbol table
            if(has_derivaties) {
                auto scp = proc_state->scope();

                // If this assertion fails, it is because the variable shadows
                // one that is already present. Which sholuld be caught
                // earlier because we want to prohibit variable names that end
                // with an underscore, like a_ and ab_
                assert(scp->find("a_").kind  == k_symbol_none);
                assert(scp->find("ab_").kind == k_symbol_none);

                // TODO: ensure that we can define a local variable with
                // expression==nullptr, because that makes sense in situations
                // like this, where we inject a symbol
                scp->add_local_symbol("a_",  id("a_"), k_symbol_local);
                scp->add_local_symbol("ba_", id("ba_"), k_symbol_local);
            }

            // set input vec_v
            proc_state->inputs().push_back({tok_eq, proc_state->scope()->find("v"), symbols_["vec_v"]});

            procedures_.push_back(proc_state);
        }

        //..........................................................
        //..........................................................
        // nrn_current : update contributions to currents
        //..........................................................
        //..........................................................
        std::list<Expression*> block;

        auto is_ion_update = [] (Expression* e) -> VariableExpression* {
            if(auto a = e->is_assignment()) {
                // semantic analysis has been performed on the original expression
                if(auto var = a->lhs()->is_identifier()->variable()) {
                    return var==nullptr ? nullptr
                                        : (var->is_ion() ? var : nullptr);
                }
            }
            return nullptr;
        };

        // add statements that initialize the reduction variables
        std::list<std::string> outputs;
        for(auto e: *(breakpoint->body())) {
            if(e->is_solve_statement()) continue;

            // add the expression
            block.push_back(e->clone());

            // we are updating an ionic current
            // so keep track of current and conductance accumulation
            if(auto var = is_ion_update(e)) {
                auto lhs = e->is_assignment()->lhs()->is_identifier();
                auto rhs = e->is_assignment()->rhs();

                // analyze the expression for linear terms
                auto v = new ExpressionClassifierVisitor(symbols_["v"]);
                rhs->accept(v);

                if(v->classify()==k_expression_lin) {
                    // add current update
                    std::string e_string = "current_ = current_ + " + var->name();
                    block.push_back(Parser(e_string).parse_line_expression());

                    auto id_g = id("conductance_");
                    auto g_stmt =
                        binary_expression(tok_eq, id_g,
                            binary_expression(tok_plus, id_g, v->linear_coefficient()));
                    block.push_back(g_stmt);
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
            }
        }

        auto proc_current =
            new APIMethod(
                    breakpoint->location(),
                    "nrn_current",
                    {},
                    new BlockExpression(loc, block, false)
                );

        symbols_["nrn_current"] = Symbol(k_symbol_procedure, proc_current);
        proc_current->semantic(symbols_);

        auto scp = proc_current->scope();
        scp->add_local_symbol("current_",     id("current_"), k_symbol_local);
        scp->add_local_symbol("conductance_", id("conductance_"), k_symbol_local);

        // now set up the input/output tables with full semantic information
        for(auto &var: outputs) {
            proc_current->outputs().push_back({tok_plus, scp->find(var), symbols_["ion_"+var]});
        }
        // only output contribution to RHS if there is one to make
        if(outputs.size()>0) {
            proc_current->outputs().push_back({tok_minus, scp->find("current_"), symbols_["vec_rhs"]});
            // assume that all input ion variables are used
            for(auto var: symbols_) {
                auto e = var.second.expression->is_variable();
                if( e && e->is_ion() && e->is_readable()) {
                    proc_current->inputs().push_back({tok_eq, scp->find(e->name()), symbols_["ion_"+e->name()]});
                }
            }
        }
        // set input vec_v
        proc_current->inputs().push_back({tok_eq, scp->find("v"), symbols_["vec_v"]});
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
    auto proc_jacob =
        new APIMethod(
            Location(), "nrn_jacob", {}, new BlockExpression(Location(), {}, false) );
    symbols_["nrn_jacob"] = Symbol(k_symbol_procedure, proc_jacob);
    proc_jacob->semantic(symbols_);

    // set output update for vec_d
    proc_jacob->outputs().push_back({tok_plus, symbols_["g_"], symbols_["vec_v"]});

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
    symbols_["t"]    = Symbol(k_symbol_variable, t);

    auto dt = new VariableExpression(Location(), "dt");
    dt->state(false);            dt->linkage(k_local_link);
    dt->ion_channel(k_ion_none); dt->range(k_scalar);
    dt->access(k_read);          dt->visibility(k_global_visibility);
    symbols_["dt"]    = Symbol(k_symbol_variable, dt);

    auto g_ = new VariableExpression(Location(), "g_");
    g_->state(false);            g_->linkage(k_local_link);
    g_->ion_channel(k_ion_none); g_->range(k_range);
    g_->access(k_readwrite);     g_->visibility(k_global_visibility);
    symbols_["g_"]    = Symbol(k_symbol_variable, g_);

    // add indexed variables to the table
    auto vec_rhs = new IndexedVariable(Location(), "vec_rhs");
    vec_rhs->access(k_write);
    vec_rhs->ion_channel(k_ion_none);
    symbols_["vec_rhs"] = Symbol(k_symbol_indexed_variable, vec_rhs);

    auto vec_d = new IndexedVariable(Location(), "vec_d");
    vec_d->access(k_write);
    vec_d->ion_channel(k_ion_none);
    symbols_["vec_d"] = Symbol(k_symbol_indexed_variable, vec_d);

    auto vec_v = new IndexedVariable(Location(), "vec_v");
    vec_v->access(k_read);
    vec_v->ion_channel(k_ion_none);
    symbols_["vec_v"] = Symbol(k_symbol_indexed_variable, vec_v);

    // add state variables
    for(auto const &var : state_block()) {
        // TODO : using an empty Location because the source location is not
        // recorded when a state block is parsed.
        VariableExpression *id = new VariableExpression(Location(), var);

        id->state(true);    // set state to true
        // state variables are private
        //      what about if the state variables is an ion concentration?
        id->linkage(k_local_link);
        id->visibility(k_local_visibility);
        id->ion_channel(k_ion_none);    // no ion channel
        id->range(k_range);             // always a range
        id->access(k_readwrite);

        symbols_[var] = Symbol(k_symbol_variable, id);
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

        symbols_[name] = Symbol(k_symbol_variable, id);
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

        symbols_[name] = Symbol(k_symbol_variable, id);
    }

    ////////////////////////////////////////////////////
    // parse the NEURON block data, and use it to update
    // the variables in symbols_
    ////////////////////////////////////////////////////
    // first the ION channels
    // check for nonspecific current
    if( neuron_block().has_nonspecific_current() ) {
        auto e = neuron_block().nonspecific_current;
        auto id = dynamic_cast<VariableExpression*>(symbols_[e->name()].expression);
        if(id==nullptr) {
            error( pprintf(
                    "nonspecific current % must be declared as "
                    " declared as PARAMETER or ASSIGNED",
                     yellow(e->name())),
                    e->location()); // location of definition
        }
        std::string name = id->name();
        if(name[0] != 'i') {
            error( pprintf(
                    "nonspecific current % does not start with 'i'",
                     yellow(e->name())),
                    e->location()); // location of definition
        }
        id->access(k_readwrite);
        id->visibility(k_global_visibility);
        id->ion_channel(k_ion_nonspecific);
    }
    for(auto const& ion : neuron_block().ions) {
        // assume that the ion channel variable has already been declared
        // we check for this, and throw an error if not
        for(auto const& var : ion.read) {
            auto id =
                dynamic_cast<VariableExpression*>(symbols_[var].expression);
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
            symbols_[shadow_id->name()] = {k_symbol_indexed_variable, shadow_id};
        }
        for(auto const& var : ion.write) {
            auto id =
                dynamic_cast<VariableExpression*>(symbols_[var].expression);
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
            symbols_[shadow_id->name()] = {k_symbol_indexed_variable, shadow_id};
        }
    }
    // then GLOBAL variables
    for(auto const& var : neuron_block().globals) {
        //auto id = dynamic_cast<VariableExpression*>(symbols_[var].expression);
        auto id = symbols_[var].expression->is_variable();
        assert(id); // this shouldn't happen, ever
        id->visibility(k_global_visibility);
    }

    // then RANGE variables
    for(auto const& var : neuron_block().ranges) {
        auto id = symbols_[var].expression->is_variable();
        assert(id); // this shouldn't happen, ever
        id->range(k_range);
    }
}

