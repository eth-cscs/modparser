#include <iostream>
#include <cstring>

#include "parser.h"
#include "util.h"

// specialize on const char* for lazy evaluation of compile time strings
bool Parser::expect(TOK tok, const char* str) {
    if(tok==token_.type) {
        return true;
    }

    error(
        strlen(str)>0 ?
            str
        :   std::string("unexpected token ")+colorize(token_.name, kYellow));

    return false;
}

bool Parser::expect(TOK tok, std::string const& str) {
    if(tok==token_.type) {
        return true;
    }

    error(
        str.size()>0 ?
            str
        :   std::string("unexpected token ")+colorize(token_.name, kYellow));

    return false;
}

void Parser::error(std::string msg) {
    std::string location_info = pprintf("%:% ", module_.name(), token_.location);
    if(status_==ls_error) {
        // append to current string
        error_string_ += "\n" + colorize(location_info, kGreen) + msg;
    }
    else {
        error_string_ = colorize(location_info, kGreen) + msg;
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
        description_pass();
    }
}

bool Parser::description_pass() {
    // perform first pass to read the descriptive blocks and
    // record the location of the verb blocks
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
            case tok_breakpoint :
                // we calculate the character that points to the first
                // given line_ and token_.location, we can compute the current_
                // required to reset state in the lexer
                get_token(); // consume BREAKPOINT symbol
                verb_blocks_.push_back({token_, line_});
                skip_block();
                break;
            case tok_initial :
                get_token(); // consume INITIAL symbol
                verb_blocks_.push_back({token_, line_});
                skip_block();
                break;
            case tok_procedure :
                {
                    get_token();
                    auto e = parse_prototype();
                }
                skip_block();
                break;
            case tok_derivative :
                {
                    get_token();
                    auto e = parse_prototype();
                }
                skip_block();
                break;
            default :
                error(pprintf("expected block type, found '%'", token_.name));
                break;
        }
        if(status() == ls_error) {
            std::cerr << colorize("error: ", kRed) << error_string_ << std::endl;
            return false;
        }
    }

    // output the contents of the descriptive blocks
    /*
    std::cout << module_.state_block();
    std::cout << module_.units_block();
    std::cout << module_.parameter_block();
    std::cout << module_.neuron_block();
    std::cout << module_.assigned_block();
    */

    // create the lookup information for the identifiers based on information
    // in the descriptive blocks
    build_identifiers();

    // check for errors when building identifier table
    if(status() == ls_error) {
        std::cerr << colorize("error : ", kRed) << error_string_ << std::endl;
        return false;
    }

    // perform the second pass
    // iterate of the verb blocks (functions, procedures, derivatives, etc...)
    // and build their ASTs
    // look up all symbols that were constructed in build_identifiers

    return true;
}

// this will skip the block that follows
// precondition:
//      - current token in the stream is the opening brace of the block '{'
void Parser::skip_block() {
    //get_token(); // get the opening '{'
    //assert(token_.type == tok_lbrace);

    get_token(); // consume opening curly brace

    int num_curlys = 0;
    while(!(token_.type==tok_rbrace && num_curlys==0)) {
        switch(token_.type) {
            case tok_lbrace :
                num_curlys++;
                break;
            case tok_rbrace :
                num_curlys--;
                break;
            case tok_eof :
                error("expect a closing } at the end of a block");
                return;
            default :
                ;
        }

        get_token(); // get the next token
    }

    get_token(); //consume the final '}'
}

// consume a comma separated list of identifiers
// IMPORTANT: leaves the current location at begining of the last identifier in the list
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
                error("line can't end with a ','");
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
                    std::vector<Token> identifiers = comma_separated_identifiers();
                    if(status_==ls_error) { // bail if there was an error reading the list
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
                    std::vector<Token> identifiers = comma_separated_identifiers();
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

Expression* Parser::parse_prototype() {
    Token identifier = token_;

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

    std::vector<Expression*> arg_expressions;
    for(auto const& t : arg_tokens) {
        arg_expressions.push_back(new IdentifierExpression(t.location, t.name));
    }

    return new PrototypeExpression(identifier.location, identifier.name, arg_expressions);
}

void Parser::parse_title() {
    std::string title;
    int this_line = location().line;

    Token tok = peek();
    while(tok.location.line==this_line && tok.type!=tok_eof && status_==ls_happy) {
        get_token();
        title += token_.name;
        tok = peek();
    }

    // set the module title
    module_.title(title);

    // load next token
    get_token();
}

// must be called between the first and second parses
void Parser::build_identifiers() {
    // add state variables
    for(auto const &var : module_.state_block()) {
        Variable *id = new Variable(var);

        id->set_state(true);    // set state to true
        // state variables are private
        //      what about if the state variables is an ion concentration?
        id->set_linkage(k_local_link);
        id->set_visibility(k_local_visibility);
        id->set_ion_channel(k_ion_none);    // no ion channel
        id->set_range(k_range);             // always a range
        id->set_access(k_readwrite);

        identifiers_[var] = id;
    }

    // add the parameters
    for(auto const& var : module_.parameter_block()) {
        auto name = var.name();
        Variable *id = new Variable(name);

        id->set_state(false);           // never a state variable
        id->set_linkage(k_local_link);
        // parameters are visible to Neuron
        id->set_visibility(k_global_visibility);
        id->set_ion_channel(k_ion_none);
        // scalar by default, may later be upgraded to range
        id->set_range(k_scalar);
        id->set_access(k_read);

        // check for 'special' variables
        if(name == "v") { // global voltage values
            id->set_linkage(k_extern_link);
            id->set_range(k_range);
            // argh, the global version cannot be modified, however
            // the local ghost is sometimes modified
            // make this illegal, because it is probably sloppy programming
            id->set_access(k_read);
        } else if(name == "celcius") { // global celcius parameter
            id->set_linkage(k_extern_link);
        }

        identifiers_[name] = id;
    }

    // add the assigned variables
    for(auto const& var : module_.assigned_block()) {
        auto name = var.name();
        Variable *id = new Variable(name);

        id->set_state(false);           // never a state variable
        id->set_linkage(k_local_link);
        // local visibility by default
        id->set_visibility(k_local_visibility);
        id->set_ion_channel(k_ion_none); // can change later
        // ranges because these are assigned to in loop
        id->set_range(k_range);
        id->set_access(k_readwrite);

        identifiers_[name] = id;
    }

    ////////////////////////////////////////////////////
    // parse the NEURON block data, and use it to update
    // the variables in identifiers_
    ////////////////////////////////////////////////////
    // first the ION channels
    for(auto const& ion : module_.neuron_block().ions) {
        // assume that the ion channel variable has already been declared
        // we check for this, and throw an error if not
        for(auto const& var : ion.read) {
            auto id = dynamic_cast<Variable*>(identifiers_[var]);
            if(id==nullptr) { // assert that variable is alredy set
                error( pprintf(
                        "variable % from ion channel % has to be"
                        " declared as PARAMETER or ASSIGNED",
                         colorize(var, kYellow),
                         colorize(ion.name, kYellow)
                        )
                );
                return;
            }
            id->set_access(k_read);
            id->set_visibility(k_global_visibility);
            id->set_ion_channel(ion.kind());
        }
        for(auto const& var : ion.write) {
            auto id = dynamic_cast<Variable*>(identifiers_[var]);
            if(id==nullptr) { // assert that variable is alredy set
                error( pprintf(
                        "variable % from ion channel % has to be"
                        " declared as PARAMETER or ASSIGNED",
                         colorize(var, kYellow),
                         colorize(ion.name, kYellow)
                        )
                );
                return;
            }
            id->set_access(k_write);
            id->set_visibility(k_global_visibility);
            id->set_ion_channel(ion.kind());
        }
    }
    // then GLOBAL variables channels
    for(auto const& var : module_.neuron_block().globals) {
        auto id = dynamic_cast<Variable*>(identifiers_[var]);
        id->set_visibility(k_global_visibility);
    }

    // then RANGE variables
    for(auto const& var : module_.neuron_block().ranges) {
        auto id = dynamic_cast<Variable*>(identifiers_[var]);
        id->set_range(k_range);
    }

    for(auto var : identifiers_) {
        //std::cout << *dynamic_cast<Variable*>(var.second) << std::endl;
    }
}

// parse a procedure
ProcedureExpression* Parser::parse_procedure() {
    // consume PROCEDURE statement
    assert(token_.type == tok_procedure);
    get_token();

    // check that a valid identifier name was specified by the user
    if( !expect( tok_identifier ) ) return nullptr;

    // remember location and name of proceduce
    Token identifier = token_;

    // todo : perform lookup to ensure that name is not already taken

    // consume the procedure name
    get_token();

    // argument list
    if( !expect(tok_lparen) ) return nullptr;

    // read the argument list
    std::vector<Expression*> args;
    for(auto const& t : comma_separated_identifiers()) {
        args.push_back(new IdentifierExpression(t.location, t.name));
    }
    get_token(); // comma_separated_identifiers doesn't consume last symbol in list

    // check for closing left brace ) on parameter list
    if(!expect(tok_rparen) || status()==ls_error) return nullptr;

    get_token();

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

    //return new ProcedureExpression( identifier.location,
    ProcedureExpression* e = new ProcedureExpression( identifier.location,
                                                      identifier.name,
                                                      args,
                                                      body);

    return e;
}

// this is the first port of call when parsing a new line inside a verb block
// it tests to see whether the expression is:
//      :: LOCAL identifier
//      :: expression
Expression *Parser::parse_high_level() {
    switch(token_.type) {
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
    if(peek().type == tok_lparen) {
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
                colorize(token_.name, kYellow)));
            return nullptr;
        }
        return lhs  ;
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
                      colorize("=", kYellow),
                      colorize(token_.name, kYellow)));
        return nullptr;
    }

    return lhs;
}

Expression *Parser::parse_expression() {
    if(token_.type == tok_lparen) {
        return parse_parenthesis_expression();
    }
    //Expression *lhs = parse_primary();
    Expression *lhs = parse_unaryop();

    if(lhs==nullptr) { // error
        return nullptr;
    }

    // we parse a binary expression if followed by an operator
    if( binop_precedence(token_.type)>0 ) {
        if(token_.type==tok_eq) {
            error("assignment '"+colorize("=",kYellow)+"' not allowed in sub-expression");
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
            error(  colorize(token_string(op), kYellow)
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
            error(  colorize(token_string(op), kYellow)
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
                      + colorize(op.name, kYellow) );
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
                            colorize(token_.name, kYellow) ));
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
                    colorize(token_.name, kYellow)));
        return nullptr;
    }

    Expression *e = new LocalExpression(location_, token_.name);
    get_token(); // consume identifier

    // check that the rest of the line was empty
    // this is to stop people doing things like 'LOCAL x = 3'
    if(line == location_.line) {
        error(pprintf( "invalid token '%' after LOCAL declaration",
                       colorize(token_.name, kYellow)));
        return nullptr;
    }
    return e;
}
