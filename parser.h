#pragma once

#include "lexer.h"
#include "module.h"

class Parser : public Lexer {
public:
    explicit Parser(Module& m);

private:
    Module &module_;

    void parse_neuron_block();

    // disable default and copy assignment
    Parser();
    Parser(Parser const &);
};
