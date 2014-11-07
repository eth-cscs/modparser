#pragma once

#include "lexer.h"
#include "module.h"

class Parser : public Lexer {
public:
    explicit Parser(Module& m);

private:
    Module &module_;

    std::vector<Token> comma_separated_identifiers();
    std::vector<Token> unit_description();

    void parse_neuron_block();
    void parse_state_block();
    void parse_units_block();

    // disable default and copy assignment
    Parser();
    Parser(Parser const &);
};

