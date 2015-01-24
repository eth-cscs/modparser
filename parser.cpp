#include <iostream>
#include <list>
#include <cstring>

#include "parser.h"
#include "util.h"
#include "perfvisitor.h"
#include "constantfolder.h"

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
    std::string location_info = pprintf(
            "%:% ", module_ ? module_->name() : "", token_.location);
    if(status_==k_compiler_error) {
        // append to current string
        error_string_ += "\n" + white(location_info) + "\n  " +msg;
    }
    else {
        error_string_ = white(location_info) + "\n  " + msg;
        status_ = k_compiler_error;
    }
}

void Parser::error(std::string msg, Location loc) {
    std::string location_info = pprintf(
            "%:% ", module_ ? module_->name() : "", loc);
    if(status_==k_compiler_error) {
        // append to current string
        error_string_ += "\n" + green(location_info) + msg;
    }
    else {
        error_string_ = green(location_info) + msg;
        status_ = k_compiler_error;
    }
}

Parser::Parser(Module& m, bool advance)
:   Lexer(m.buffer()),
    module_(&m)
{
    // prime the first token
    get_token();

    if(advance) {
        parse();
    }
}

Parser::Parser(std::string const& buf)
:   Lexer(buf),
    module_(nullptr)
{
    // prime the first token
    get_token();
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
            // INITIAL, DERIVATIVE, PROCEDURE, NET_RECEIVE and BREAKPOINT blocks
            // are all lowered to ProcedureExpression
            case tok_net_receive:
            case tok_breakpoint :
            case tok_initial    :
            case tok_derivative :
            case tok_procedure  :
                e = parse_procedure();
                if(!e) break;
                module_->procedures().push_back(e);
                break;
            case tok_function  :
                e = parse_function();
                if(!e) break;
                module_->functions().push_back(e);
                break;
            default :
                error(pprintf("expected block type, found '%'", token_.name));
                break;
        }
        if(status() == k_compiler_error) {
            std::cerr << red("error: ") << error_string_ << std::endl;
            return false;
        }
    }

    return true;
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
    get_token(); // prime the first token after the list

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
        error(pprintf("NEURON block must start with a curly brace {, found '%'",
                      token_.name));
        return;
    }

    // initialize neuron block
    neuron_block.threadsafe = false;

    // there are no use cases for curly brace in a NEURON block, so we don't
    // have to count them we have to get the next token before entering the loop
    // to handle the case of an empty block {}
    get_token();
    while(token_.type!=tok_rbrace) {
        switch(token_.type) {
            case tok_threadsafe :
                neuron_block.threadsafe = true;
                get_token(); // consume THREADSAFE
                break;

            case tok_suffix :
            case tok_point_process :
                neuron_block.kind = (token_.type==tok_suffix) ? k_module_density
                                                              : k_module_point;
                get_token(); // consume SUFFIX / POINT_PROCESS
                // assert that a valid name for the Neuron has been specified
                if(token_.type != tok_identifier) {
                    error(pprintf("invalid name for SUFFIX, found '%'", token_.name));
                    return;
                }
                neuron_block.name = token_.name;

                get_token(); // consume the name
                break;

            // this will be a comma-separated list of identifiers
            case tok_global :
                // the ranges are a comma-seperated list of identifiers
                {
                    std::vector<Token> identifiers =
                        comma_separated_identifiers();
                    // bail if there was an error reading the list
                    if(status_==k_compiler_error) {
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
                    if(status_==k_compiler_error) { // bail if there was an error reading the list
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
                    // we assume that the user has asked for a valid ion channel
                    get_token();
                    if(token_.type != tok_identifier) {
                        // TODO: extended to test for valid ion names (k, Ca, ... others)
                        error(pprintf("invalid name for an ion chanel '%'",
                                      token_.name));
                        return;
                    }
                    ion.name = token_.name;
                    get_token(); // consume the ion name

                    // this loop ensures that we don't gobble any tokens past
                    // the end of the USEION clause
                    while(token_.type == tok_read || token_.type == tok_write) {
                        auto& target = (token_.type == tok_read) ? ion.read
                                                                 : ion.write;
                        std::vector<Token> identifiers
                            = comma_separated_identifiers();
                        // bail if there was an error reading the list
                        if(status_==k_compiler_error) {
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

            case tok_nonspecific_current :
                // Assume that there is one non-specific current per mechanism.
                // It would be easy to extend this to multiple currents,
                // however there are no mechanisms in the CoreNeuron repository
                // that do this, so err on the side of caution.
                {
                    get_token(); // consume NONSPECIFIC_CURRENT

                    // get the name of the current
                    auto id = parse_identifier();

                    if(status_==k_compiler_error) {
                        return;
                    }

                    neuron_block.nonspecific_current = id->is_identifier();
                }
                break;

            // the parser encountered an invalid symbol
            default :
                error(pprintf("there was an invalid statement '%' in NEURON block",
                              token_.name));
                return;
        }
    }

    // copy neuron block into module
    module_->neuron_block(neuron_block);

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
    module_->state_block(state_block);

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
        if( status_!=k_compiler_happy ) return;

        // consume the '=' sign
        if( token_.type!=tok_eq ) {
            error(pprintf("expected '=', found '%'", token_.name));
            return;
        }

        get_token(); // next token

        // get the units
        std::vector<Token> rhs = unit_description();
        if( status_!=k_compiler_happy ) return;

        // store the unit definition
        units_block.unit_aliases.push_back({lhs, rhs});
    }

    // add this state block information to the module
    module_->units_block(units_block);

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
            if(status_ == k_compiler_error) {
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

    module_->parameter_block(block);

    return;
parm_error:
    // only write error message if one hasn't already been logged by the lexer
    if(status_==k_compiler_happy) {
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
            if(status_ == k_compiler_error) {
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

    module_->assigned_block(block);

    return;
ass_error:
    // only write error message if one hasn't already been logged by the lexer
    if(status_==k_compiler_happy) {
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
Expression* Parser::parse_prototype(std::string name=std::string()) {
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
        return new PrototypeExpression( identifier.location,
                                        identifier.name,
                                        {});
    }

    get_token(); // consume '('
    std::vector<Token> arg_tokens;
    while(token_.type != tok_rparen) {
        // check identifier
        if(token_.type != tok_identifier) {
            error(  "expected a valid identifier, found '"
                  + yellow(token_.name) + "'");
            return nullptr;
        }

        arg_tokens.push_back(token_);

        get_token(); // consume the identifier

        // look for a comma
        if(!(token_.type == tok_comma || token_.type==tok_rparen)) {
            error(  "expected a comma or closing parenthesis, found '"
                  + yellow(token_.name) + "'");
            return nullptr;
        }

        if(token_.type == tok_comma) {
            get_token(); // consume ','
        }
    }

    if(token_.type != tok_rparen) {
        error("procedure argument list must have closing parenthesis ')'");
        return nullptr;
    }
    get_token(); // consume closing parenthesis

    // pack the arguments into LocalExpressions
    std::vector<Expression*> arg_expressions;
    for(auto const& t : arg_tokens) {
        arg_expressions.push_back(new ArgumentExpression(t.location, t));
    }

    return new PrototypeExpression( identifier.location,
                                    identifier.name,
                                    arg_expressions);
}

void Parser::parse_title() {
    std::string title;
    int this_line = location().line;

    Token tok = peek();
    while(   tok.location.line==this_line
          && tok.type!=tok_eof
          && status_==k_compiler_happy)
    {
        get_token();
        title += token_.name;
        tok = peek();
    }

    // set the module title
    module_->title(title);

    // load next token
    get_token();
}

/// parse a procedure
/// can handle both PROCEDURE and INITIAL blocks
/// an initial block is stored as a procedure with name 'initial' and empty argument list
Expression* Parser::parse_procedure() {
    Expression* p = nullptr;
    procedureKind kind = k_proc_normal;

    switch( token_.type ) {
        case tok_derivative:
            kind = k_proc_derivative;
            get_token(); // consume keyword token
            if( !expect( tok_identifier ) ) return nullptr;
            p = parse_prototype();
            break;
        case tok_procedure:
            kind = k_proc_normal;
            get_token(); // consume keyword token
            if( !expect( tok_identifier ) ) return nullptr;
            p = parse_prototype();
            break;
        case tok_initial:
            kind = k_proc_initial;
            p = parse_prototype("initial");
            break;
        case tok_breakpoint:
            kind = k_proc_breakpoint;
            p = parse_prototype("breakpoint");
            break;
        case tok_net_receive:
            kind = k_proc_net_receive;
            p = parse_prototype("net_receive");
            break;
        default:
            // it is a compiler error if trying to parse_procedure() without
            // having DERIVATIVE, PROCEDURE, INITIAL or BREAKPOINT keyword
            assert(false);
    }
    if(p==nullptr) return nullptr;

    // check for opening left brace {
    if(!expect(tok_lbrace)) return nullptr;

    // parse the body of the function
    Expression* body = parse_block(false);
    if(body==nullptr) return nullptr;

    auto proto = p->is_prototype();
    if(kind != k_proc_net_receive) {
        return new ProcedureExpression(
                    proto->location(), proto->name(), proto->args(), body, kind);
    }
    else {
        return new NetReceiveExpression(
                    proto->location(), proto->name(), proto->args(), body);
    }
}

Expression* Parser::parse_function() {
    // check for compiler implementation error
    assert(token_.type == tok_function);

    get_token(); // consume FUNCTION token

    // check that a valid identifier name was specified by the user
    if( !expect( tok_identifier ) ) return nullptr;

    // parse the prototype
    Expression *p = parse_prototype();
    if(p==nullptr) return nullptr;

    // check for opening left brace {
    if(!expect(tok_lbrace)) return nullptr;

    // parse the body of the function
    Expression* body = parse_block(false);
    if(body==nullptr) return nullptr;

    PrototypeExpression *proto = p->is_prototype();
    return new FunctionExpression(
            proto->location(), proto->name(), proto->args(), body);
}

// this is the first port of call when parsing a new line inside a verb block
// it tests to see whether the expression is:
//      :: LOCAL identifier
//      :: expression
Expression *Parser::parse_statement() {
    switch(token_.type) {
        case tok_if :
            return parse_if();
            break;
        case tok_solve :
            return parse_solve();
        case tok_local :
            return parse_local();
        case tok_identifier :
            return parse_line_expression();
        case tok_initial :
            // only used for INITIAL block in NET_RECEIVE
            return parse_initial();
        default:
            error(pprintf("unexpected token type % '%'", token_string(token_.type), token_.name));
            return nullptr;
    }
    return nullptr;
}

Expression *Parser::parse_identifier() {
    assert(token_.type==tok_identifier);

    // save name and location of the identifier
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

    return new CallExpression(idtoken.location, idtoken.name, args);
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
    Location loc = location_;

    get_token(); // consume LOCAL

    // create local expression stub
    Expression *e = new LocalExpression(loc);
    if(e==nullptr) return nullptr;

    // add symbols
    while(1) {
        if(!expect(tok_identifier)) return nullptr;

        // try adding variable name to list
        if(!e->is_local_declaration()->add_variable(token_)) {
            error(e->error_message());
            return nullptr;
        }
        get_token(); // consume identifier

        // look for comma that indicates continuation of the variable list
        if(token_.type == tok_comma) {
            get_token();
        } else {
            break;
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
           "where 'x' is the name of a DERIVATIVE block", loc);
    return nullptr;
}

Expression *Parser::parse_if() {
    assert(token_.type==tok_if);

    Token if_token = token_;
    get_token(); // consume 'if'

    if(!expect(tok_lparen)) return nullptr;

    // parse the conditional
    Expression* cond = parse_parenthesis_expression();
    if(cond==nullptr) return nullptr;

    // parse the block of the true branch
    Expression* true_branch = parse_block(false);
    if(true_branch==nullptr) return nullptr;

    // parse the false branch if there is an else
    Expression* false_branch = nullptr;
    if(token_.type == tok_else) {
        get_token(); // consume else

        // handle 'else if {}' case recursively
        if(token_.type == tok_if) {
            false_branch = parse_if();
        }
        // we have a closing 'else {}'
        else if(token_.type == tok_lbrace) {
            false_branch = parse_block(false);
        }
        else {
            error("expect either '"+yellow("if")+"' or '"+yellow("{")+" after else");
            return nullptr;
        }
    }

    return new IfExpression(if_token.location, cond, true_branch, false_branch);
}

// takes a flag indicating whether the block is at procedure/function body,
// or lower. Can be used to check for illegal statements inside a nested block,
// e.g. LOCAL declarations.
Expression *Parser::parse_block(bool is_nested) {
    // blocks have to be enclosed in curly braces {}
    assert(token_.type==tok_lbrace);

    get_token(); // consume '{'

    // save the location of the first statement as the starting point for the block
    Location block_location = token_.location;

    std::list<Expression*> body;
    while(token_.type != tok_rbrace) {
        Expression *e = parse_statement();
        if(e==nullptr) return nullptr;

        if(is_nested) {
            if(e->is_local_declaration()) {
                error("LOCAL variable declarations are not allowed inside a nested scope");
                return nullptr;
            }
        }

        body.push_back(e);
    }

    if(token_.type != tok_rbrace) {
        error("could not find closing '" + yellow("}")
              + "' for else statement that started at "
              + ::to_string(block_location));
        return nullptr;
    }
    get_token(); // consume closing '}'

    return new BlockExpression(block_location, body, is_nested);
}

Expression *Parser::parse_initial() {
    // has to start with INITIAL: error in compiler implementaion otherwise
    assert(token_.type==tok_initial);

    // save the location of the first statement as the starting point for the block
    Location block_location = token_.location;

    get_token(); // consume 'INITIAL'

    if(!expect(tok_lbrace)) return nullptr;
    get_token(); // consume '{'

    std::list<Expression*> body;
    while(token_.type != tok_rbrace) {
        Expression *e = parse_statement();
        if(e==nullptr) return nullptr;

        // disallow variable declarations in an INITIAL block
        if(e->is_local_declaration()) {
            error("LOCAL variable declarations are not allowed inside a nested scope");
            return nullptr;
        }

        body.push_back(e);
    }

    if(token_.type != tok_rbrace) {
        error("could not find closing '" + yellow("}")
              + "' for else statement that started at "
              + ::to_string(block_location));
        return nullptr;
    }
    get_token(); // consume closing '}'

    return new InitialBlock(block_location, body);
}

