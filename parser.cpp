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

/*
NEURON {
   THREADSAFE
   SUFFIX KdShu2007
   USEION k WRITE ik READ xy
   RANGE  gkbar, ik, ek
   GLOBAL minf, mtau, hinf, htau
}
keywords : THREADSAFE SUFFIX USEION RANGE GLOBAL (WRITE READ)
THREADSAFE
SUFFIX
USEION (WRITE READ)
RANGE
GLOBAL
*/
void Parser::parse_neuron_block() {
    NeuronBlock neuron_block;

    get_token();

    // assert that the block starts with a curly brace
    if(token_.type != tok_lbrace) {
        pprintf("% at % NEURON block must start with a curly brace {, found '%'",
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

            case tok_useion :
                break;

            // this will be a comma-separated list of identifiers
            case tok_range  :
                break;

            // this will be a comma-separated list of identifiers
            case tok_global :
                break;

            // the parser encountered an invalid symbol
            default         :
                error_string_ = pprintf("% at % NEURON found innapropriate symbol '%'",
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
