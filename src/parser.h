#pragma once

#include <string>

#include "expression.h"
#include "lexer.h"
#include "module.h"

class Parser : public Lexer {
public:
    explicit Parser(Module& m, bool advance=true);
    Parser(std::string const&);
    bool parse();

    Expression* parse_prototype(std::string);
    Expression* parse_statement();
    Expression* parse_identifier();
    Expression* parse_number();
    Expression* parse_call();
    Expression* parse_expression();
    Expression* parse_primary();
    Expression* parse_parenthesis_expression();
    Expression* parse_line_expression();
    Expression* parse_binop(Expression *, Token);
    Expression* parse_unaryop();
    Expression* parse_local();
    Expression* parse_solve();
    Expression* parse_block(bool);
    Expression* parse_initial();
    Expression* parse_if();

    Expression* parse_procedure();
    Expression* parse_function();

    std::string const& error_message() {
        return error_string_;
    }

private:
    Module *module_;

    // functions for parsing descriptive blocks
    // these are called in the first pass, and do not
    // construct any AST information
    void parse_neuron_block();
    void parse_state_block();
    void parse_units_block();
    void parse_parameter_block();
    void parse_assigned_block();
    void parse_title();

    std::vector<Token> comma_separated_identifiers();
    std::vector<Token> unit_description();

    /// build the identifier list
    void add_variables_to_symbols();

    // helper function for logging errors
    void error(std::string msg);
    void error(std::string msg, Location loc);

    // disable default and copy assignment
    Parser();
    Parser(Parser const &);

    bool expect(TOK, const char *str="");
    bool expect(TOK, std::string const& str);
};

