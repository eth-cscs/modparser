#include <iostream>

#include "parser.h"
#include "util.h"


void Parser::error(std::string msg) {
    std::string location_info = pprintf("%:% ", module_.name(), token_.location);
    if(status_==ls_error) {
        // append to current string
        error_string_ += "\n" + location_info + msg;
    }
    else {
        error_string_ = location_info + msg;
        status_ = ls_error;
    }
}

Parser::Parser(Module& m)
:   module_(m),
    Lexer(m.buffer())
{
    // prime the first token
    get_token();

    while(token_.type!=tok_eof) {
        switch(token_.type) {
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
            default :
                error(pprintf("expected block type, found '%'", token_.name));
                break;
        }
        if(status() == ls_error) {
            std::cerr << "\033[1;31m" << error_string_ << "\033[0m" << std::endl;
            break;
        }
    }
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
        // should this be an error?
        // it is the sort of thing that would happen when scanning WRITE below:
        //  USEION k READ a, b WRITE
        // probably best left to the caller to decide whether an empty list is an error
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
            // consume the comma
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

    std::cout << neuron_block;

    // now we have a curly brace, so prime the next token
    get_token();
}

void Parser::parse_state_block() {
    StateBlock state_block;

    get_token();

    // assert that the block starts with a curly brace
    if(token_.type != tok_lbrace) {
        error(pprintf("NEURON block must start with a curly brace {, found '%'", token_.name));
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

    std::cout << state_block;

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
        error(pprintf("NEURON block must start with a curly brace {, found '%'", token_.name));
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

    std::cout << units_block;

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
        error(pprintf("NEURON block must start with a curly brace {, found '%'", token_.name));
        return;
    }

    // there are no use cases for curly brace in a UNITS block, so we don't have to count them
    get_token();
    while(token_.type!=tok_rbrace && token_.type!=tok_eof) {
        int line = location_.line;
        Variable parm;

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
    std::cout << block;

    // errer if EOF before closeing curly brace
    if(token_.type==tok_eof) {
        error("PARAMETER block must have closing '}'");
        goto parm_error;
    }

    get_token(); // consume closing brace

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
        error(pprintf("NEURON block must start with a curly brace {, found '%'", token_.name));
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
                block.parameters.push_back(Variable(t, "", u));
            }
        }
        else {
            for(auto const& t : variables) {
                block.parameters.push_back(Variable(t, "", {}));
            }
        }
    }
    std::cout << block;

    // errer if EOF before closeing curly brace
    if(token_.type==tok_eof) {
        error("ASSIGNED block must have closing '}'");
        goto ass_error;
    }

    get_token(); // consume closing brace

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

