#pragma once

#include "lexer.h"
#include "module.h"

class Parser : public Lexer {
public:
    explicit Parser(Module& m);

private:
    Module &module_;

    std::vector<Token> comma_separated_identifiers();

    void parse_neuron_block();
    void parse_state_block();

    // disable default and copy assignment
    Parser();
    Parser(Parser const &);
};

