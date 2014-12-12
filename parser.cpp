#include <iostream>
#include <cstring>

#include "parser.h"
#include "util.h"
#include "errorvisitor.h"
#include "perfvisitor.h"

// specialize on const char* for lazy evaluation of compile time strings
bool Parser::expect(TOK tok, const char* str) {
    if(tok==token_.type) {
        return true;
    }

    error(
        strlen(str)>0 ?
            str
        :   std::string("unexpected token ")+yellow(token_.name));

    return false;
}

bool Parser::expect(TOK tok, std::string const& str) {
    if(tok==token_.type) {
        return true;
    }

    error(
        str.size()>0 ?
            str
        :   std::string("unexpected token ")+yellow(token_.name));

    return false;
}

void Parser::error(std::string msg) {
    std::string location_info = pprintf("%:% ", module_.name(), token_.location);
    if(status_==ls_error) {
        // append to current string
        error_string_ += "\n" + white(location_info) + "\n  " +msg;
    }
    else {
        error_string_ = white(location_info) + "\n  " + msg;
        status_ = ls_error;
    }
}

void Parser::error(std::string msg, Location loc) {
    std::string location_info = pprintf("%:% ", module_.name(), loc);
    if(status_==ls_error) {
        // append to current string
        error_string_ += "\n" + green(location_info) + msg;
    }
    else {
        error_string_ = green(location_info) + msg;
        status_ = ls_error;
    }
}

Parser::Parser(Module& m, bool advance)
:   module_(m),
    Lexer(m.buffer())
{
    // prime the first token
    get_token();

    if(advance) {
        parse();
    }
}

bool Parser::parse() {
    // perform first pass to read the descriptive blocks and
    // record the location of the verb blocks
    Expression *e; // for use when parsing blocks
    while(token_.type!=tok_eof) {
        switch(token_.type) {
            case tok_title :
                parse_title();
                break;
            case tok_neuron :
                parse_neuron_block();
                break;
            case tok_state :
                parse_state_block();
                break;
            case tok_units :
                parse_units_block();
                break;
            case tok_parameter :
                parse_parameter_block();
                break;
            case tok_assigned :
                parse_assigned_block();
                break;
            // INITIAL, DERIVATIVE, PROCEDURE and BREAKPOINT blocks
            // are all lowered to ProcedureExpression
            case tok_breakpoint :
            case tok_initial    :
            case tok_derivative :
            case tok_procedure  :
                e = parse_procedure();
                if(!e) break;
                procedures_.push_back(e);
                break;
            case tok_function  :
                e = parse_function();
                if(!e) break;
                functions_.push_back(e);
                break;
            default :
                error(pprintf("expected block type, found '%'", token_.name));
                break;
        }
        if(status() == ls_error) {
            std::cerr << red("error: ") << error_string_ << std::endl;
            return false;
        }
    }

    return true;
}

bool Parser::semantic() {
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
    if(status() == ls_error) {
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
        symbols_[fun->name()] = Symbol(k_function, f);
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
        symbols_[proc->name()] = Symbol(k_procedure, p);
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

        if( s.kind == k_function || s.kind == k_procedure ) {
            // first perform semantic analysis
            s.expression->semantic(symbols_);
            // then use an error visitor to print out all the semantic errors
            ErrorVisitor* v = new ErrorVisitor(module_.name());
            s.expression->accept(v);
            errors += v->num_errors();
            delete v;
        }
    }

    if(errors) {
        std::cout << "\nthere were " << errors
                  << " errors in the semantic analysis" << std::endl;
        status_ = ls_error;
    }

    return status() == ls_happy;
}

// consume a comma separated list of identifiers
// NOTE: leaves the current location at begining of the last identifier in the list
// OK:  empty list ""
// OK:  list with one identifier "a"
// OK:  list with mutiple identifier "a, b, c, d"
// BAD: list with keyword "a, b, else, d"
// list with trailing comma "a, b,\n"
// list with keyword "a, if, b"
std::vector<Token> Parser::comma_separated_identifiers() {
    std::vector<Token> tokens;
    int startline = location_.line;
    // handle is an empty list at the end of a line
    if(peek().location.line > startline) {
        // this happens when scanning WRITE below:
        //      USEION k READ a, b WRITE
        // leave to the caller to decide whether an empty list is an error
        return tokens;
    }
    while(1) {
        get_token();

        // first check if a new line was encounterd
        if(location_.line > startline) {
            return tokens;
        }
        else if(token_.type == tok_identifier) {
            tokens.push_back(token_);
        }
        else if(is_keyword(token_)) {
            error(pprintf("found keyword '%', expected a variable name", token_.name));
            return tokens;
        }
        else if(token_.type == tok_number) {
            error(pprintf("found number '%', expected a variable name", token_.name));
            return tokens;
        }
        else {
            error(pprintf("found '%', expected a variable name", token_.name));
            return tokens;
        }

        // look ahead to check for a comma.  This approach ensures that the
        // first token after the end of the list is not consumed
        if( peek().type == tok_comma ) {
            // load the comma
            get_token();
            // assert that the list can't run off the end of a line
            if(peek().location.line > startline) {
                error("line can't end with a '"+yellow(",")+"'");
                return tokens;
            }
        }
        else {
            break;
        }
    }

    return tokens;
}

/*
NEURON {
   THREADSAFE
   SUFFIX KdShu2007
   USEION k WRITE ik READ xy
   RANGE  gkbar, ik, ek
   GLOBAL minf, mtau, hinf, htau
}
*/
void Parser::parse_neuron_block() {
    NeuronBlock neuron_block;

    get_token();

    // assert that the block starts with a curly brace
    if(token_.type != tok_lbrace) {
        error(pprintf("NEURON block must start with a curly brace {, found '%'", token_.name));
        return;
    }

    // initialize neuron block
    neuron_block.threadsafe = false;

    // there are no use cases for curly brace in a NEURON block, so we don't have to count them
    // we have to get the next token before entering the loop to handle the case of
    // an empty block {}
    get_token();
    while(token_.type!=tok_rbrace) {
        switch(token_.type) {
            case tok_threadsafe :
                neuron_block.threadsafe = true;
                break;

            case tok_suffix :
                get_token();
                // assert that a valid name for the Neuron has been specified
                if(token_.type != tok_identifier) {
                    error(pprintf("invalid name for SUFFIX, found '%'", token_.name));
                    return;
                }
                neuron_block.suffix = token_.name;
                break;

            // this will be a comma-separated list of identifiers
            case tok_global :
                // the ranges are a comma-seperated list of identifiers
                {
                    std::vector<Token> identifiers =
                        comma_separated_identifiers();
                    // bail if there was an error reading the list
                    if(status_==ls_error) {
                        return;
                    }
                    for(auto const &id : identifiers) {
                        neuron_block.globals.push_back(id.name);
                    }
                }
                break;

            // this will be a comma-separated list of identifiers
            case tok_range  :
                // the ranges are a comma-seperated list of identifiers
                {
                    std::vector<Token> identifiers =
                        comma_separated_identifiers();
                    if(status_==ls_error) { // bail if there was an error reading the list
                        return;
                    }
                    for(auto const &id : identifiers) {
                        neuron_block.ranges.push_back(id.name);
                    }
                }
                break;

            case tok_useion :
                {
                    IonDep ion;
                    // we have to parse the name of the ion first
                    // we assume that the user has asked for a valid ion channel name.
                    get_token();
                    if(token_.type != tok_identifier) {
                        // todo: extended to test for valid ion names (k, Ca, ... others)
                        error(pprintf("invalid name for an ion chanel '%'", token_.name));
                        return;
                    }
                    ion.name = token_.name;

                    // this loop ensures that we don't gobble any tokens past the end of the USEION clause
                    while(peek().type == tok_read || peek().type == tok_write) {
                        get_token(); // consume the READ or WRITE
                        auto& target = (token_.type == tok_read) ? ion.read : ion.write;
                        std::vector<Token> identifiers = comma_separated_identifiers();
                        if(status_==ls_error) { // bail if there was an error reading the list
                            return;
                        }
                        for(auto const &id : identifiers) {
                            target.push_back(id.name);
                        }
                    }
                    // add the ion dependency to the NEURON block
                    neuron_block.ions.push_back(ion);
                }
                break;

            // the parser encountered an invalid symbol
            default :
                error(pprintf("there was an invalid symbol '%' in NEURON block", token_.name));
                return;
        }
        get_token();
    }

    // copy neuron block into module
    module_.neuron_block(neuron_block);

    //std::cout << neuron_block;

    // now we have a curly brace, so prime the next token
    get_token();
}

void Parser::parse_state_block() {
    StateBlock state_block;

    get_token();

    // assert that the block starts with a curly brace
    if(token_.type != tok_lbrace) {
        error(pprintf("STATE block must start with a curly brace {, found '%'", token_.name));
        return;
    }

    // there are no use cases for curly brace in a STATE block, so we don't have to count them
    // we have to get the next token before entering the loop to handle the case of
    // an empty block {}
    get_token();
    while(token_.type!=tok_rbrace) {
        if(token_.type != tok_identifier) {
            error(pprintf("'%' is not a valid name for a state variable", token_.name));
            return;
        }
        state_block.state_variables.push_back(token_.name);
        get_token();
    }

    // add this state block information to the module
    module_.state_block(state_block);

    // now we have a curly brace, so prime the next token
    get_token();
}

// scan a unit block
void Parser::parse_units_block() {
    UnitsBlock units_block;

    get_token();

    // assert that the block starts with a curly brace
    if(token_.type != tok_lbrace) {
        error(pprintf("UNITS block must start with a curly brace {, found '%'", token_.name));
        return;
    }

    // there are no use cases for curly brace in a UNITS block, so we don't have to count them
    get_token();
    while(token_.type!=tok_rbrace) {
        // get the alias
        std::vector<Token> lhs = unit_description();
        if( status_!=ls_happy ) return;

        // consume the '=' sign
        if( token_.type!=tok_eq ) {
            error(pprintf("expected '=', found '%'", token_.name));
            return;
        }

        get_token(); // next token

        // get the units
        std::vector<Token> rhs = unit_description();
        if( status_!=ls_happy ) return;

        // store the unit definition
        units_block.unit_aliases.push_back({lhs, rhs});
    }

    // add this state block information to the module
    module_.units_block(units_block);

    // now we have a curly brace, so prime the next token
    get_token();
}

//////////////////////////////////////////////////////
// the parameter block describes variables that are
// to be used as parameters. Some are given values,
// others are simply listed, and some have units
// assigned to them. Here we want to get a list of the
// parameter names, along with values if given.
// We also store the token that describes the units
//////////////////////////////////////////////////////
void Parser::parse_parameter_block() {
    ParameterBlock block;

    get_token();

    // assert that the block starts with a curly brace
    if(token_.type != tok_lbrace) {
        error(pprintf("PARAMETER block must start with a curly brace {, found '%'", token_.name));
        return;
    }

    // there are no use cases for curly brace in a UNITS block, so we don't have to count them
    get_token();
    while(token_.type!=tok_rbrace && token_.type!=tok_eof) {
        int line = location_.line;
        Id parm;

        // read the parameter name
        if(token_.type != tok_identifier) {
            goto parm_error;
        }
        parm.token = token_; // save full token

        get_token();

        // look for equality
        if(token_.type==tok_eq) {
            get_token(); // consume '='
            if(token_.type==tok_minus) {
                parm.value = "-";
                get_token();
            }
            if(token_.type != tok_number) {
                goto parm_error;
            }
            parm.value += token_.name; // store value as a string
            get_token();
        }

        // get the parameters
        if(line==location_.line && token_.type == tok_lparen) {
            parm.units = unit_description();
            if(status_ == ls_error) {
                goto parm_error;
            }
        }

        block.parameters.push_back(parm);
    }

    // errer if EOF before closeing curly brace
    if(token_.type==tok_eof) {
        error("PARAMETER block must have closing '}'");
        goto parm_error;
    }

    get_token(); // consume closing brace

    module_.parameter_block(block);

    return;
parm_error:
    // only write error message if one hasn't already been logged by the lexer
    if(status_==ls_happy) {
        error(pprintf("PARAMETER block unexpected symbol '%'", token_.name));
    }
    return;
}

void Parser::parse_assigned_block() {
    AssignedBlock block;

    get_token();

    // assert that the block starts with a curly brace
    if(token_.type != tok_lbrace) {
        error(pprintf("ASSIGNED block must start with a curly brace {, found '%'", token_.name));
        return;
    }

    // there are no use cases for curly brace in an ASSIGNED block, so we don't have to count them
    get_token();
    while(token_.type!=tok_rbrace && token_.type!=tok_eof) {
        int line = location_.line;
        std::vector<Token> variables; // we can have more than one variable on a line

        // the first token must be ...
        if(token_.type != tok_identifier) {
            goto ass_error;
        }
        // read all of the identifiers until we run out of identifiers or reach a new line
        while(token_.type == tok_identifier && line == location_.line) {
            variables.push_back(token_);
            get_token();
        }

        // there are some parameters at the end of the line
        if(line==location_.line && token_.type == tok_lparen) {
            auto u = unit_description();
            if(status_ == ls_error) {
                goto ass_error;
            }
            for(auto const& t : variables) {
                block.parameters.push_back(Id(t, "", u));
            }
        }
        else {
            for(auto const& t : variables) {
                block.parameters.push_back(Id(t, "", {}));
            }
        }
    }

    // errer if EOF before closeing curly brace
    if(token_.type==tok_eof) {
        error("ASSIGNED block must have closing '}'");
        goto ass_error;
    }

    get_token(); // consume closing brace

    module_.assigned_block(block);

    return;
ass_error:
    // only write error message if one hasn't already been logged by the lexer
    if(status_==ls_happy) {
        error(pprintf("ASSIGNED block unexpected symbol '%'", token_.name));
    }
    return;
}

std::vector<Token> Parser::unit_description() {
    static const TOK legal_tokens[] = {tok_identifier, tok_divide, tok_number};
    int startline = location_.line;
    std::vector<Token> tokens;

    // chec that we start with a left parenthesis
    if(token_.type != tok_lparen)
        goto unit_error;
    get_token();

    while(token_.type != tok_rparen) {
        // check for illegal tokens or a new line
        if( !is_in(token_.type,legal_tokens) || startline < location_.line )
            goto unit_error;

        // add this token to the set
        tokens.push_back(token_);
        get_token();
    }
    // remove trailing right parenthesis ')'
    get_token();

    return tokens;

unit_error:
    error(pprintf("incorrect unit description '%'", tokens));
    return tokens;
}

// Returns a prototype expression for a function or procedure call
// Takes an optional argument that allows the user to specify the
// name of the prototype, which is used for prototypes where the name
// is implcitly defined (e.g. INITIAL and BREAKPOINT blocks)
PrototypeExpression* Parser::parse_prototype(std::string name=std::string()) {
    Token identifier = token_;

    if(name.size()) {
        // we assume that the current token_ is still pointing at
        // the keyword, i.e. INITIAL or BREAKPOINT
        identifier.type = tok_identifier;
        identifier.name = name;
    }

    // load the parenthesis
    get_token();

    // check for an argument list enclosed in parenthesis (...)
    // return a prototype with an empty argument list if not found
    if( token_.type != tok_lparen ) {
        return new PrototypeExpression(identifier.location, identifier.name, {});
    }

    std::vector<Token> arg_tokens = comma_separated_identifiers();
    if(status()==ls_error) { // break away if there was an error
        return nullptr;
    }

    // advance to next token, because the list reader leaves token_
    // pointing to the last entry in the list
    get_token();

    if(token_.type != tok_rparen) {
        error("procedure argument list must have closing parenthesis ')'");
        return nullptr;
    }
    get_token(); // consume closing parenthesis

    // pack the arguments into LocalExpressions
    std::vector<Expression*> arg_expressions;
    for(auto const& t : arg_tokens) {
        arg_expressions.push_back(new LocalExpression(t.location, t.name));
    }

    return new PrototypeExpression(identifier.location, identifier.name, arg_expressions);
}

void Parser::parse_title() {
    std::string title;
    int this_line = location().line;

    Token tok = peek();
    while(   tok.location.line==this_line
          && tok.type!=tok_eof
          && status_==ls_happy)
    {
        get_token();
        title += token_.name;
        tok = peek();
    }

    // set the module title
    module_.title(title);

    // load next token
    get_token();
}

/// populate the symbol table with class scope variables
void Parser::add_variables_to_symbols() {
    // add state variables
    for(auto const &var : module_.state_block()) {
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

        symbols_[var] = Symbol(k_variable, id);
    }

    // add the parameters
    for(auto const& var : module_.parameter_block()) {
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

        symbols_[name] = Symbol(k_variable, id);
    }

    // add the assigned variables
    for(auto const& var : module_.assigned_block()) {
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

        symbols_[name] = Symbol(k_variable, id);
    }

    ////////////////////////////////////////////////////
    // parse the NEURON block data, and use it to update
    // the variables in symbols_
    ////////////////////////////////////////////////////
    // first the ION channels
    for(auto const& ion : module_.neuron_block().ions) {
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
                        )
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
                        )
                );
                return;
            }
            id->access(k_write);
            id->visibility(k_global_visibility);
            id->ion_channel(ion.kind());
        }
    }
    // then GLOBAL variables channels
    for(auto const& var : module_.neuron_block().globals) {
        auto id = dynamic_cast<VariableExpression*>(symbols_[var].expression);
        id->visibility(k_global_visibility);
    }

    // then RANGE variables
    for(auto const& var : module_.neuron_block().ranges) {
        auto id = dynamic_cast<VariableExpression*>(symbols_[var].expression);
        id->range(k_range);
    }
}

/// parse a procedure
/// can handle both PROCEDURE and INITIAL blocks
/// an initial block is stored as a procedure with name 'initial' and empty argument list
Expression* Parser::parse_procedure() {
    PrototypeExpression* proto = nullptr;

    switch( token_.type ) {
        case tok_derivative:
        case tok_procedure:
            get_token(); // consume keyword token

            // check that a valid identifier name was specified by the user
            if( !expect( tok_identifier ) ) return nullptr;

            proto = parse_prototype();
            break;
        case tok_initial:
            proto = parse_prototype("initial");
            break;
        case tok_breakpoint:
            proto = parse_prototype("breakpoint");
            break;
        default:
            // it is a compiler error if trying to parse_procedure() without
            // having DERIVATIVE, PROCEDURE, INITIAL or BREAKPOINT keyword
            assert(false);
    }

    // check for opening left brace {
    if(!expect(tok_lbrace)) return nullptr;
    get_token(); // consume left brace '{'

    std::vector<Expression*> body;

    while(1) {
        if(token_.type == tok_rbrace)
            break;

        Expression *e = parse_high_level();
        if(e==nullptr) {
            return nullptr;
        }

        body.push_back(e);
    }

    get_token(); // consume closing '}'

    return new ProcedureExpression(
                    proto->location(), proto->name(), proto->args(), body);
}

Expression* Parser::parse_function() {
    // check for compiler implementation error
    assert(token_.type == tok_function);

    get_token(); // consume FUNCTION token

    // check that a valid identifier name was specified by the user
    if( !expect( tok_identifier ) ) return nullptr;

    // parse the prototype
    PrototypeExpression *proto = parse_prototype();
    if(proto==nullptr) return nullptr;

    // check for opening left brace {
    if(!expect(tok_lbrace)) return nullptr;

    get_token();
    std::vector<Expression*> body;

    while(1) {
        if(token_.type == tok_rbrace)
            break;

        Expression *e = parse_high_level();
        if(e==nullptr) {
            return nullptr;
        }

        body.push_back(e);
    }

    get_token(); // eat closing '}'

    return new FunctionExpression(
            proto->location(), proto->name(), proto->args(), body);
}

// this is the first port of call when parsing a new line inside a verb block
// it tests to see whether the expression is:
//      :: LOCAL identifier
//      :: expression
Expression *Parser::parse_high_level() {
    switch(token_.type) {
        case tok_solve :
            return parse_solve();
        case tok_local :
            return parse_local();
        case tok_identifier :
            return parse_line_expression();
        default:
            error(pprintf("unexpected token type % '%'", token_string(token_.type), token_.name));
            return nullptr;
    }
    return nullptr;
}

Expression *Parser::parse_identifier() {
    assert(token_.type==tok_identifier);

    // save name and location of the identifier
    Token idtoken = token_;
    Expression* id = new IdentifierExpression(token_.location, token_.name);

    // consume identifier
    get_token();

    // return variable identifier
    return id;
}

Expression *Parser::parse_call() {
    // save name and location of the identifier
    Token idtoken = token_;

    // consume identifier
    get_token();

    // check for a function call
    // assert this is so
    assert(token_.type == tok_lparen);

    std::vector<Expression*> args;

    // parse a function call
    get_token(); // consume '('

    while(token_.type != tok_rparen) {
        Expression *e = parse_expression();
        if(!e) return nullptr;

        args.push_back(e);

        // reached the end of the argument list
        if(token_.type == tok_rparen) break;

        // insist on a comma between arguments
        if( !expect(tok_comma, "call arguments must be separated by ','") )
            return nullptr;
        get_token(); // consume ','
    }

    // check that we have a closing parenthesis
    if(!expect(tok_rparen, "function call missing closing ')'") ) {
        return nullptr;
    }
    get_token(); // consume ')'

    return new CallExpression(token_.location, idtoken.name, args);
}

// parse a full line expression, i.e. one of
//      :: procedure call        e.g. rates(v+0.01)
//      :: assignment expression e.g. x = y + 3
// to parse a subexpression, see parse_expression()
// proceeds by first parsing the LHS (which may be a variable or function call)
// then attempts to parse the RHS if
//      1. the lhs is not a procedure call
//      2. the operator that follows is =
Expression *Parser::parse_line_expression() {
    int line = location_.line;
    Expression *lhs;
    Token next = peek();
    if(next.type == tok_lparen) {
        lhs = parse_call();
        // we have to ensure that a procedure call is alone on the line
        // to avoid :
        //      :: assigning to it            e.g. foo() = x + 6
        //      :: stray symbols coming after e.g. foo() + x
        // here we assume that foo is a procedure call, if it is an eroneous
        // function call this has to be caught in the second pass..
        // or optimized away with a warning
        if(!lhs) return nullptr;
        if(location_.line == line && token_.type != tok_eof) {
            error(pprintf(
                "expected a new line after call expression, found '%'",
                yellow(token_.name)));
            return nullptr;
        }
        return lhs  ;
    } else if(next.type == tok_prime) {
        lhs = new DerivativeExpression(location_, token_.name);
        // consume both name and derivative operator
        get_token();
        get_token();
        // a derivative statement must be followed by '='
        if(token_.type!=tok_eq) {
            error("a derivative declaration must have an assignment of the form\n  x' = expression\n  where x is a state variable");
            return nullptr;
        }
    } else {
        //lhs = parse_primary();
        lhs = parse_unaryop();
    }

    if(lhs==nullptr) { // error
        return nullptr;
    }

    // we parse a binary expression if followed by an operator
    if(token_.type == tok_eq) {
        Token op = token_;  // save the '=' operator with location
        get_token();        // consume the '=' operator
        return parse_binop(lhs, op);
    } else if(line == location_.line && token_.type != tok_eof){
        error(pprintf("expected an assignment '%' or new line, found '%'",
                      yellow("="),
                      yellow(token_.name)));
        return nullptr;
    }

    return lhs;
}

Expression *Parser::parse_expression() {
    Expression *lhs = parse_unaryop();

    if(lhs==nullptr) { // error
        return nullptr;
    }

    // we parse a binary expression if followed by an operator
    if( binop_precedence(token_.type)>0 ) {
        if(token_.type==tok_eq) {
            error("assignment '"+yellow("=")+"' not allowed in sub-expression");
            return nullptr;
        }
        Token op = token_;  // save the operator
        get_token();        // consume the operator
        return parse_binop(lhs, op);
    }

    return lhs;
}

Expression* Parser::unary_expression( Location loc,
                                      TOK op,
                                      Expression* e) {
    switch(op) {
        case tok_minus :
            return new NegUnaryExpression(loc, e);
        case tok_exp :
            return new ExpUnaryExpression(loc, e);
        case tok_cos :
            return new CosUnaryExpression(loc, e);
        case tok_sin :
            return new SinUnaryExpression(loc, e);
        case tok_log :
            return new LogUnaryExpression(loc, e);
       default :
            error(  yellow(token_string(op))
                  + " is not a valid unary operator");
            return nullptr;
    }
    assert(false); // something catastrophic went wrong
    return nullptr;
}

Expression* Parser::binary_expression( Location loc,
                                       TOK op,
                                       Expression* lhs,
                                       Expression* rhs) {
    switch(op) {
        case tok_eq     :
            return new AssignmentExpression(loc, lhs, rhs);
        case tok_plus   :
            return new AddBinaryExpression(loc, lhs, rhs);
        case tok_minus  :
            return new SubBinaryExpression(loc, lhs, rhs);
        case tok_times  :
            return new MulBinaryExpression(loc, lhs, rhs);
        case tok_divide :
            return new DivBinaryExpression(loc, lhs, rhs);
        case tok_pow    :
            return new PowBinaryExpression(loc, lhs, rhs);
        default         :
            error(  yellow(token_string(op))
                  + " is not a valid binary operator");
            return nullptr;
    }

    assert(false); // something catastrophic went wrong
    return nullptr;
}

/// Parse a unary expression.
/// If called when the current node in the AST is not a unary expression the call
/// will be forwarded to parse_primary. This mechanism makes it possible to parse
/// all nodes in the expression using parse_unary, which simplifies the call sites
/// with either a primary or unary node is to be parsed.
/// It also simplifies parsing nested unary functions, e.g. x + - - y
Expression *Parser::parse_unaryop() {
    Expression *e;
    Token op = token_;
    switch(token_.type) {
        case tok_plus  :
            // plus sign is simply ignored
            get_token(); // consume '+'
            return parse_unaryop();
        case tok_minus :
            get_token();       // consume '-'
            e = parse_unaryop(); // handle recursive unary
            if(!e) return nullptr;
            return unary_expression(token_.location, op.type, e);
        case tok_exp   :
        case tok_sin   :
        case tok_cos   :
        case tok_log   :
            get_token();        // consume operator (exp, sin, cos or log)
            if(token_.type!=tok_lparen) {
                error(  "missing parenthesis after call to "
                      + yellow(op.name) );
                return nullptr;
            }
            e = parse_unaryop(); // handle recursive unary
            if(!e) return nullptr;
            return unary_expression(token_.location, op.type, e);
        default     :
            return parse_primary();
    }
    // not a unary expression, so simply parse primary
    assert(false); // all cases should be handled by the switch
    return nullptr;
}

/// parse a primary expression node
/// expects one of :
///  ::  number
///  ::  identifier
///  ::  call
///  ::  parenthesis expression (parsed recursively)
Expression *Parser::parse_primary() {
    switch(token_.type) {
        case tok_number:
            return parse_number();
        case tok_identifier:
            if( peek().type == tok_lparen ) {
                return parse_call();
            }
            return parse_identifier();
        case tok_lparen:
            return parse_parenthesis_expression();
        default: // fall through to return nullptr at end of function
            error( pprintf( "unexpected token '%' in expression",
                            yellow(token_.name) ));
    }

    return nullptr;
}

Expression *Parser::parse_parenthesis_expression() {
    // never call unless at start of parenthesis
    assert(token_.type==tok_lparen);

    get_token(); // consume '('

    Expression* e = parse_expression();

    // check for closing parenthesis ')'
    if( !e || !expect(tok_rparen) ) return nullptr;

    get_token(); // consume ')'

    return e;
}

Expression* Parser::parse_number() {
    assert(token_.type = tok_number);

    Expression *e = new NumberExpression(token_.location, token_.name);

    get_token(); // consume the number

    return e;
}

Expression *Parser::parse_binop(Expression *lhs, Token op_left) {
    // only way out of the loop below is by return:
    //      :: return with nullptr on error
    //      :: return when loop runs out of operators
    //          i.e. if(pp<0)
    //      :: return when recursion applied to remainder of expression
    //          i.e. if(p_op>p_left)
    while(1) {
        // get precedence of the left operator
        int p_left = binop_precedence(op_left.type);

        //Expression* e = parse_primary();
        Expression* e = parse_unaryop();
        if(!e) return nullptr;

        Token op = token_;
        int p_op = binop_precedence(op.type);

        //  if no binop, parsing of expression is finished with (op_left lhs e)
        if(p_op < 0) {
            return binary_expression(op_left.location, op_left.type, lhs, e);
        }

        get_token(); // consume op
        if(p_op > p_left) {
            Expression *rhs = parse_binop(e, op);
            if(!rhs) return nullptr;
            return binary_expression(op_left.location, op_left.type, lhs, rhs);
        }

        lhs = binary_expression(op_left.location, op_left.type, lhs, e);
        op_left = op;
    }
    assert(false);
    return nullptr;
}

/// parse a local variable definition
/// a local variable definition is a line with the form
///     LOCAL x
/// where x is a valid identifier name
Expression *Parser::parse_local() {
    assert(token_.type==tok_local);
    int line = location_.line;

    get_token(); // consume LOCAL

    if(token_.type != tok_identifier) {
        error(pprintf("'%' is not a valid name for a local variable",
                    yellow(token_.name)));
        return nullptr;
    }

    Expression *e = new LocalExpression(location_, token_.name);
    get_token(); // consume identifier

    // check that the rest of the line was empty
    // this is to stop people doing things like 'LOCAL x = 3'
    if(line == location_.line) {
        if(token_.type != tok_eof) {
            error(pprintf( "invalid token '%' after LOCAL declaration",
                        yellow(token_.name)));
            return nullptr;
        }
    }
    return e;
}

/// parse a SOLVE statement
/// a SOLVE statement specifies a procedure and a method
///     SOLVE procedure METHOD method
Expression *Parser::parse_solve() {
    assert(token_.type==tok_solve);
    int line = location_.line;
    Location loc = location_; // solve location for expression
    std::string name;
    solverMethod method;

    get_token(); // consume the SOLVE keyword

    if(token_.type != tok_identifier) goto solve_statment_error;

    name = token_.name; // save name of procedure
    get_token(); // consume the procedure identifier

    if(token_.type != tok_method) goto solve_statment_error;

    get_token(); // consume the METHOD keyword

    // for now the parser only knows the cnexp method, because that is the only
    // method used by the modules in CoreNeuron
    if(token_.type != tok_cnexp) goto solve_statment_error;
    method = k_cnexp;

    get_token(); // consume the method description

    // check that the rest of the line was empty
    if(line == location_.line) {
        if(token_.type != tok_eof) goto solve_statment_error;
    }

    return new SolveExpression(loc, name, method);

solve_statment_error:
    error( "SOLVE statements must have the form\n"
           "  SOLVE x METHOD cnexp\n"
           "where 'x' is the name of a DERIVATIVE block");
    return nullptr;
}

