#pragma once

#include "expression.h"
#include "lexer.h"
#include "module.h"

class Parser : public Lexer {
public:
    explicit Parser(Module& m);

    Expression* parse_prototype();

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
    void parse_title();

    void skip_block();

    /// build the identifier list
    void build_identifiers();

    // helper function for logging errors
    void error(std::string msg);

    // disable default and copy assignment
    Parser();
    Parser(Parser const &);

    // hash table for lookup of variable and call names
    std::unordered_map<std::string, Identifier*> identifiers_;
};

