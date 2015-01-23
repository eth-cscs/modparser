#include <fstream>
#include <iostream>
#include <set>

#include "errorvisitor.h"
#include "expressionclassifier.h"
#include "inline.h"
#include "module.h"

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
        error_string_ += "\n" + green(location_info) + msg;
    }
    else {
        error_string_ = green(location_info) + msg;
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
        auto proc_init = new ProcedureExpression(
                            initial->location(),
                            "init",       // corresponds to nrn_init()
                            {},           // no arguments
                            new BlockExpression(loc, {}, false),
                            k_proc_api);

        if( symbols_.find("init")!=symbols_.end() ) {
            error("'init' defined at % clashes with INITIAL, please rename it",
                  loc);
            return false;
        }
        symbols_["init"] = Symbol(k_symbol_procedure, proc_init);

        // get a reference to the empty body of the init function
        auto& body = proc_init->body()->body();
        for(auto e : *(initial->body())) {
            if(e->is_local_declaration()) continue;
            body.push_back(e->clone());
        }
        // perform semantic analysis for init
        proc_init->semantic(symbols_);

        procedures_.push_back(proc_init);
    }
    else {
        error("an INITIAL block is required", Location());
        return false;
    }
    if( has_symbol("breakpoint", k_symbol_procedure) ) {
        ////////////////////////////////////////////////////////////
        // first the solve
        ////////////////////////////////////////////////////////////
        auto breakpoint = symbols_["breakpoint"].expression->is_procedure();
        auto loc = breakpoint->location();
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
                new ProcedureExpression(
                        loc,
                        "state",
                        {},
                        new BlockExpression(loc, {}, false),
                        k_proc_api
                    );

            // perform semantic analysis for init
            proc_state->semantic(symbols_);

            procedures_.push_back(proc_state);
            symbols_["state"] = Symbol(k_symbol_procedure, proc_state);
        }
        else {
            // get the DERIVATIVE block
            auto dblock = sexp->procedure();
            auto proc_state =
                new ProcedureExpression(
                        dblock->location(),
                        "state",
                        {},
                        new BlockExpression(loc, {}, false),
                        k_proc_api
                    );
            // check for symbol clash
            if( symbols_.find("state")!=symbols_.end() ) {
                error("'state' defined at % clashes with state, please rename it",
                      loc);
                return false;
            }

            // get a reference to the empty body of the init function
            auto& body = proc_state->body()->body();
            for(auto e : *(dblock->body())) {
                std::cout << red("... ") << e->to_string() << std::endl;
                if(e->is_local_declaration()) continue;
                if(e->is_assignment()) {
                    auto lhs = e->is_assignment()->lhs();
                    auto rhs = e->is_assignment()->rhs();
                    if(lhs->is_derivative()) {
                        std::cout << red("derivative ") << e->to_string()
                                  << std::endl;
                        auto sym = lhs->is_derivative()->symbol();

                        // create visitor for lineary analysis
                        auto v = new ExpressionClassifierVisitor(sym);

                        // analyse the rhs
                        rhs->accept(v);
                        //auto coeff = v->linear_coefficient();
                        if( v->classify() != k_expression_lin ) {
                            error("unable to integrate nonlinear state ODEs",
                                  rhs->location());
                            return false;
                        }

                        continue;
                    }
                    else {
                        body.push_back(e->clone());
                    }
                }
                body.push_back(e->clone());
            }

            symbols_["state"] = Symbol(k_symbol_procedure, proc_state);
            proc_state->semantic(symbols_);

            procedures_.push_back(proc_state);
        }
    }
    else {
        error("an INITIAL block is required", Location());
        return false;
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
    symbols_["t"]    = Symbol(k_symbol_variable, t);
    auto dt = new VariableExpression(Location(), "dt");
    dt->state(false);            dt->linkage(k_local_link);
    dt->ion_channel(k_ion_none); dt->range(k_scalar);
    dt->access(k_read);          dt->visibility(k_global_visibility);
    symbols_["dt"]    = Symbol(k_symbol_variable, dt);

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
                     yellow(e->name())
                    ),
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
            if(id==nullptr) { // assert that variable is alredy set
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
        }
        for(auto const& var : ion.write) {
            auto id =
                dynamic_cast<VariableExpression*>(symbols_[var].expression);
            if(id==nullptr) { // assert that variable is alredy set
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
        }
    }
    // then GLOBAL variables channels
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

