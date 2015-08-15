#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <set>

#include "errorvisitor.hpp"
#include "expressionclassifier.hpp"
#include "functionexpander.hpp"
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
    if(error_string_.size()) {// append to current string
        error_string_ += "\n";
    }
    error_string_ += red("error   ") + white(location_info) + msg;
    status_ = lexerStatus::error;
}

void Module::warning(std::string const& msg, Location loc) {
    std::string location_info = pprintf("%:% ", file_name(), loc);
    if(error_string_.size()) {// append to current string
        error_string_ += "\n";
    }
    error_string_ += purple("warning ") + white(location_info) + msg;
    has_warning_ = true;
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

    // first add variables defined in the NEURON, ASSIGNED and PARAMETER
    // blocks these symbols have "global" scope, i.e. they are visible to all
    // functions and procedurs in the mechanism
    add_variables_to_symbols();

    // Helper which iterates over a vector of Symbols, inserting them into the
    // symbol table.
    // Returns false if the list contains a symbol that is already in the
    // symbol table.
    auto add_symbols = [this] (std::vector<symbol_ptr>& symbol_list) {
        for(auto& symbol: symbol_list) {
            bool is_found = (symbols_.find(symbol->name()) != symbols_.end());
            if(is_found) {
                error(
                    pprintf("'%' clashes with previously defined symbol",
                            symbol->name()),
                    symbol->location()
                );
                return false;
            }
            // add symbol to table
            symbols_[symbol->name()] = std::move(symbol);
        }
        return true;
    };

    // add functions and procedures to symbol table
    if(!add_symbols(functions_))  return false;
    if(!add_symbols(procedures_)) return false;

    ////////////////////////////////////////////////////////////////////////////
    // now iterate over the functions and procedures and perform semantic
    // analysis on each. This includes
    //  -   variable, function and procedure lookup
    //  -   generate local variable table for each function/procedure
    ////////////////////////////////////////////////////////////////////////////
    int errors = 0;
    for(auto& e : symbols_) {
        auto& s = e.second;

        if( s->kind() == symbolKind::function || s->kind() == symbolKind::procedure ) {
            // first perform semantic analysis
            s->semantic(symbols_);

            // then use an error visitor to print out all the semantic errors
            auto v = make_unique<ErrorVisitor>(file_name());
            s->accept(v.get());
            errors += v->num_errors();

            // inline function calls
            // this requires that the symbol table has already been built
            if(v->num_errors()==0) {
                auto &b =  s->kind()==symbolKind::function ?
                    s->is_function()->body()->body() :
                    s->is_procedure()->body()->body();
                for(auto e=b.begin(); e!=b.end(); ++e) {
                    auto new_expressions = expand_function_calls((*e).get());
                    b.splice(e, std::move(new_expressions));
                }
            }
        }
    }

    if(errors) {
        std::cout << "\nthere were " << errors
                  << " errors in the semantic analysis" << std::endl;
        status_ = lexerStatus::error;
        return false;
    }

    // All API methods are generated from statements in one of the special procedures
    // defined in NMODL, e.g. the nrn_init() API call is based on the INITIAL block.
    // When creating an API method, the first task is to look up the source procedure,
    // i.e. the INITIAL block for nrn_init(). This lambda takes care of this repetative
    // lookup work, with error checking.
    auto make_empty_api_method = [this]
            (std::string const& name, std::string const& source_name)
            -> std::pair<APIMethod*, ProcedureExpression*>
    {
        if( !has_symbol(source_name, symbolKind::procedure) ) {
            error(pprintf("unable to find symbol '%'", yellow(source_name)),
                   Location());
            return std::make_pair(nullptr, nullptr);
        }

        auto source = symbols_[source_name]->is_procedure();
        auto loc = source->location();

        if( symbols_.find(name)!=symbols_.end() ) {
            error(pprintf("'%' clashes with reserved name, please rename it",
                          yellow(name)),
                  symbols_.find(name)->second->location());
            return std::make_pair(nullptr, source);
        }

        symbols_[name] = make_symbol<APIMethod>(
                          loc, name,
                          std::vector<expression_ptr>(), // no arguments
                          make_expression<BlockExpression>
                            (loc, std::list<expression_ptr>(), false)
                         );

        auto proc = symbols_[name]->is_api_method();
        return std::make_pair(proc, source);
    };

    //.........................................................................
    // nrn_init : based on the INITIAL block (i.e. the 'initial' procedure
    //.........................................................................
    auto initial_api = make_empty_api_method("nrn_init", "initial");
    auto api_init  = initial_api.first;
    auto proc_init = initial_api.second;

    if(api_init)
    {
        auto& body = api_init->body()->body();

        for(auto& e : *proc_init->body()) {
            body.emplace_back(e->clone());
        }

        api_init->semantic(symbols_);
    }
    else {
        if(!proc_init) {
            error("an INITIAL block is required", Location());
        }
        return false;
    }

    // Look in the symbol table for a procedure with the name "breakpoint".
    // This symbol corresponds to the BREAKPOINT block in the .mod file
    // There are two APIMethods generated from BREAKPOINT.
    // The first is nrn_state, which is the first case handled below.
    // The second is nrn_current, which is handled after this block
    auto state_api = make_empty_api_method("nrn_state", "breakpoint");
    auto api_state  = state_api.first;
    auto breakpoint = state_api.second;

    if( breakpoint ) {
        // helper for making identifiers on the fly
        auto id = [] (std::string const& name) {
            return make_expression<IdentifierExpression>(Location(), name);
        };
        //..........................................................
        // nrn_state : The temporal integration of state variables
        //..........................................................

        // find the SOLVE statement
        SolveExpression* solve_expression = nullptr;
        for(auto& e: *(breakpoint->body())) {
            solve_expression = e->is_solve_statement();
            if(solve_expression) break;
        }

        // handle the case where there is no SOLVE in BREAKPOINT
        if( solve_expression==nullptr ) {
            warning( " there is no SOLVE statement, required to update the"
                     " state variables, in the BREAKPOINT block",
                     Location());
        }
        else {
            // get the DERIVATIVE block
            auto dblock = solve_expression->procedure();

            // body refers to the currently empty body of the APIMethod that
            // will hold the AST for the nrn_state function.
            auto& body = api_state->body()->body();

            auto has_provided_integration_method =
                solve_expression->method() == solverMethod::cnexp;

            // loop over the statements in the SOLVE block from the mod file
            // put each statement into the new APIMethod, performing
            // transformations if necessary.
            for(auto& e : *(dblock->body())) {
                if(auto ass = e->is_assignment()) {
                    auto lhs = ass->lhs();
                    auto rhs = ass->rhs();
                    if(auto deriv = lhs->is_derivative()) {
                        // Check that a METHOD was provided in the original SOLVE
                        // statment. We have to do this because it is possible
                        // to call SOLVE without a METHOD, in which case there should
                        // be no derivative expressions in the DERIVATIVE block.
                        if(!has_provided_integration_method) {
                            error("The DERIVATIVE block has a derivative expression"
                                  " but no METHOD was specified in the SOLVE statement",
                                  deriv->location());
                            return false;
                        }

                        auto sym  = deriv->symbol();
                        auto name = deriv->name();

                        // create visitor for linear analysis
                        auto v = make_unique<ExpressionClassifierVisitor>(sym);

                        // analyse the rhs
                        rhs->accept(v.get());

                        // quit if ODE is not linear
                        if( v->classify() != expressionClassification::linear ) {
                            error("unable to integrate nonlinear state ODEs",
                                  rhs->location());
                            return false;
                        }

                        // the linear differential equation is of the form
                        //      s' = a*s + b
                        // integration by separation of variables gives the following
                        // update function to integrate s for one time step dt
                        //      s = -b/a + (s+b/a)*exp(a*dt)
                        // we are going to build this update function by
                        //  1. generating statements that define a_=a and ba_=b/a
                        //  2. generating statements that update the solution

                        // statement : a_ = a
                        auto stmt_a  =
                            binary_expression(Location(),
                                              tok::eq,
                                              id("a_"),
                                              v->linear_coefficient()->clone());

                        // expression : b/a
                        auto expr_ba =
                            binary_expression(Location(),
                                              tok::divide,
                                              v->constant_term()->clone(),
                                              id("a_"));
                        // statement  : ba_ = b/a
                        auto stmt_ba = binary_expression(Location(), tok::eq, id("ba_"), std::move(expr_ba));

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

            // perform semantic analysis
            api_state->semantic(symbols_);
        }

        //..........................................................
        // nrn_current : update contributions to currents
        //..........................................................
        std::list<expression_ptr> block;

        // helper which tests a statement to see if it updates an ion
        // channel variable.
        auto is_ion_update = [] (Expression* e) {
            if(auto a = e->is_assignment()) {
                // semantic analysis has been performed on the original expression
                // which ensures that the lhs is an identifier and a variable
                if(auto sym = a->lhs()->is_identifier()->symbol()) {
                    // assume that a scalar stack variable is being used for
                    // the indexed value: i.e. the value is not cached
                    if(auto var = sym->is_local_variable()) {
                        return var->ion_channel();
                    }
                }
            }
            return ionKind::none;
        };

        // add statements that initialize the reduction variables
        for(auto& e: *(breakpoint->body())) {
            // ignore solve and conductance statements
            if(e->is_solve_statement())       continue;
            if(e->is_conductance_statement()) continue;

            // add the expression
            block.emplace_back(e->clone());

            // we are updating an ionic current
            // so keep track of current and conductance accumulation
            auto channel = is_ion_update(e.get());
            if(channel != ionKind::none) {
                auto lhs = e->is_assignment()->lhs()->is_identifier();
                auto rhs = e->is_assignment()->rhs();

                // analyze the expression for linear terms
                //auto v = make_unique<ExpressionClassifierVisitor>(symbols_["v"].get());
                auto v_symbol = breakpoint->scope()->find("v");
                auto v = make_unique<ExpressionClassifierVisitor>(v_symbol);
                rhs->accept(v.get());

                if(v->classify()==expressionClassification::linear) {
                    // add current update
                    std::string e_string = "current_ = current_ + " + lhs->name();
                    block.emplace_back(Parser(e_string).parse_line_expression());

                    auto g_stmt =
                        binary_expression(tok::eq,
                                          id("conductance_"),
                                          binary_expression(tok::plus,
                                                            id("conductance_"),
                                                            v->linear_coefficient()->clone()));
                    block.emplace_back(std::move(g_stmt));
                }
                else {
                    error("current update functions must be a linear"
                          " function of v : " + rhs->to_string(), e->location());
                    return false;
                }
            }
        }
        auto v = make_unique<ConstantFolderVisitor>();
        for(auto& e : block) {
            e->accept(v.get());
        }

        symbols_["nrn_current"] =
            make_symbol<APIMethod>(
                    breakpoint->location(), "nrn_current",
                    std::vector<expression_ptr>(),
                    make_expression<BlockExpression>(breakpoint->location(),
                                                     std::move(block), false)
            );
        symbols_["nrn_current"]->semantic(symbols_);
    }
    else {
        error("a BREAKPOINT block is required", Location());
        return false;
    }

    return status() == lexerStatus::happy;
}

/// populate the symbol table with class scope variables
void Module::add_variables_to_symbols() {
    // add reserved symbols (not v, because for some reason it has to be added
    // by the user)
    auto create_variable = [this] (const char* name, rangeKind rng, accessKind acc) {
        auto t = new VariableExpression(Location(), name);
        t->state(false);
        t->linkage(linkageKind::local);
        t->ion_channel(ionKind::none);
        t->range(rng);
        t->access(acc);
        t->visibility(visibilityKind::global);
        symbols_[name] = symbol_ptr{t};
    };

    create_variable("t",  rangeKind::scalar, accessKind::read);
    create_variable("dt", rangeKind::scalar, accessKind::read);

    // add indexed variables to the table
    auto create_indexed_variable = [this]
        (std::string const& name, std::string const& indexed_name,
         tok op, accessKind acc, ionKind ch)
    {
        symbols_[name] =
            make_symbol<IndexedVariable>(Location(), name, indexed_name, acc, op, ch);
    };

    create_indexed_variable("current_", "vec_rhs", tok::minus,
                            accessKind::write, ionKind::none);
    create_indexed_variable("conductance_", "vec_d", tok::plus,
                            accessKind::write, ionKind::none);
    create_indexed_variable("v", "vec_v", tok::eq,
                            accessKind::read,  ionKind::none);

    // add state variables
    for(auto const &var : state_block()) {
        VariableExpression *id = new VariableExpression(Location(), var);

        id->state(true);    // set state to true
        // state variables are private
        //      what about if the state variables is an ion concentration?
        id->linkage(linkageKind::local);
        id->visibility(visibilityKind::local);
        id->ion_channel(ionKind::none);    // no ion channel
        id->range(rangeKind::range);       // always a range
        id->access(accessKind::readwrite);

        symbols_[var] = symbol_ptr{id};
    }

    // add the parameters
    for(auto const& var : parameter_block()) {
        auto name = var.name();
        if(name == "v") { // global voltage values
            // ignore voltage, which is added as an indexed variable by default
            continue;
        }
        VariableExpression *id = new VariableExpression(Location(), name);

        id->state(false);           // never a state variable
        id->linkage(linkageKind::local);
        // parameters are visible to Neuron
        id->visibility(visibilityKind::global);
        id->ion_channel(ionKind::none);
        // scalar by default, may later be upgraded to range
        id->range(rangeKind::scalar);
        id->access(accessKind::read);

        // check for 'special' variables
        if(name == "celcius") { // global celcius parameter
            id->linkage(linkageKind::external);
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
        if(name == "v") { // global voltage values
            // ignore voltage, which is added as an indexed variable by default
            continue;
        }
        VariableExpression *id = new VariableExpression(Location(), name);

        id->state(false);           // never a state variable
        id->linkage(linkageKind::local);
        // local visibility by default
        id->visibility(visibilityKind::local);
        id->ion_channel(ionKind::none); // can change later
        // ranges because these are assigned to in loop
        id->range(rangeKind::range);
        id->access(accessKind::readwrite);

        symbols_[name] = symbol_ptr{id};
    }

    ////////////////////////////////////////////////////
    // parse the NEURON block data, and use it to update
    // the variables in symbols_
    ////////////////////////////////////////////////////
    // first the ION channels
    // add ion channel variables
    auto update_ion_symbols = [this, create_indexed_variable]
            (std::string const& var, accessKind acc, ionKind channel)
    {
        // add the ion variable's indexed shadow
        if(has_symbol(var)) {
            auto sym = symbols_[var].get();

            // has the user declared a range/parameter with the same name?
            if(sym->kind()!=symbolKind::indexed_variable) {
                warning(
                    pprintf("the symbol % clashes with the ion channel variable,"
                            " and will be ignored", yellow(var)),
                    sym->location()
                );
                // erase symbol
                symbols_.erase(var);
            }
        }

        create_indexed_variable(var, "ion_"+var,
                                acc==accessKind::read ? tok::eq : tok::plus,
                                acc, channel);
    };

    // check for nonspecific current
    if( neuron_block().has_nonspecific_current() ) {
        auto const& i = neuron_block().nonspecific_current;
        update_ion_symbols(i.spelling, accessKind::write, ionKind::nonspecific);
    }


    for(auto const& ion : neuron_block().ions) {
        for(auto const& var : ion.read) {
            update_ion_symbols(var, accessKind::read, ion.kind());
        }
        for(auto const& var : ion.write) {
            update_ion_symbols(var, accessKind::write, ion.kind());
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
        auto& sym = symbols_[var.spelling];
        if(auto id = sym->is_variable()) {
            id->visibility(visibilityKind::global);
        }
        else if (!sym->is_indexed_variable()){
            throw compiler_exception(
                "unable to find symbol " + yellow(var.spelling) + " in symbols",
                Location());
        }
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
        auto& sym = symbols_[var.spelling];
        if(auto id = sym->is_variable()) {
            id->range(rangeKind::range);
        }
        else if (!sym->is_indexed_variable()){
            throw compiler_exception(
                "unable to find symbol " + yellow(var.spelling) + " in symbols",
                Location());
        }
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
        if(kind == symbolKind::procedure) {
            // we are only interested in true procedurs and APIMethods
            auto proc = symbol.second->is_procedure();
            auto pkind = proc->kind();
            if(pkind == procedureKind::normal || pkind == procedureKind::api )
                body = symbol.second->is_procedure()->body();
            else
                continue;
        }
        // for now don't look at functions
        //else if(kind == symbolKind::function) {
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
    }

    return true;
}

