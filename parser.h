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
    std::vector<std::pair<Token, const char*>> verb_blocks_;

    void parse_neuron_block();
    void parse_state_block();
    void parse_units_block();
    void parse_parameter_block();
    void parse_assigned_block();

    void skip_block();

    // helper function for logging errors
    void error(std::string msg);

    // disable default and copy assignment
    Parser();
    Parser(Parser const &);
};

