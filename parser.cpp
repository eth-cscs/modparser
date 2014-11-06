#include <iostream>

#include "parser.h"
#include "util.h"

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
            default :
                error_string_ +=
                    pprintf("% at % expected block type, found '%'",
                            module_.name(), token_.location, token_.name);
                status_ = ls_error;
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
// empty list ""
// list with one identifier "a"
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
        else if(token_.type == tok_reserved) {
            error_string_ = pprintf("% at % read badly formed symbol '%', expected a variable name",
                                    module_.name(), token_.location, token_.name);
            status_ = ls_error;
            return tokens;
        }
        else if(token_.type == tok_identifier) {
            tokens.push_back(token_);
        }
        else if(is_keyword(token_)) {
            error_string_ = pprintf("% at % found keyword '%', expected a variable name",
                                    module_.name(), token_.location, token_.name);
            status_ = ls_error;
            return tokens;
        }
        else if(token_.type == tok_number) {
            error_string_ = pprintf("% at % found number '%', expected a variable name",
                                    module_.name(), token_.location, token_.name);
            status_ = ls_error;
            return tokens;
        }
        else {
            error_string_ = pprintf("% at % found '%', expected a variable name",
                                    module_.name(), token_.location, token_.name);
            status_ = ls_error;
            return tokens;
        }

        // look ahead to check for a comma.  This approach ensures that the
        // first token after the end of the list is not consumed
        if( peek().type == tok_comma ) {
            // consume the comma
            get_token();
            // assert that the list can't run off the end of a line
            if(peek().location.line > startline) {
                error_string_ = pprintf("% at % previous line ended with a ','",
                                        module_.name(), token_.location);
                status_ = ls_error;
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
        error_string_ = pprintf("% at % NEURON block must start with a curly brace {, found '%'",
                                module_.name(), token_.location, token_.name);
        status_ = ls_error;
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
                    error_string_ = pprintf("% at % invalide name for SUFFIX, found '%'",
                                            module_.name(), token_.location, token_.name);
                    status_ = ls_error;
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
                        error_string_ = pprintf("% at %  | this is an invalid name for an ion chanel '%'",
                                                module_.name(), token_.location, token_.name);
                        status_ = ls_error;
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
            default         :
                error_string_ = pprintf("% at % there was an innapropriate symbol '%' in NEURON block",
                                        module_.name(), token_.location, token_.name);
                status_ = ls_error;
                return;
        }
        get_token();
    }

    std::cout << neuron_block;

    // copy neuron block into module
    module_.neuron_block(neuron_block);

    // now we have a curly brace, so prime the next token
    get_token();
}
