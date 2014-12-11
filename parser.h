#pragma once

#include "expression.h"
#include "lexer.h"
#include "module.h"

class Parser : public Lexer {
public:
    explicit Parser(Module& m, bool advance=true);
    bool parse();
    bool semantic();

    PrototypeExpression* parse_prototype(std::string);
    Expression* parse_high_level();
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

    Expression* parse_procedure();
    Expression* parse_function();

    std::string const& error_message() {
        return error_string_;
    }

    std::vector<Expression*>&
    procedures() { return procedures_; }

    std::vector<Expression*>const&
    procedures() const { return procedures_; }

    std::vector<Expression*>&
    functions() { return functions_; }

    std::vector<Expression*>const&
    functions() const { return functions_; }

    std::unordered_map<std::string, Identifier*>&
    symbols() { return symbols_; }

    std::unordered_map<std::string, Identifier*>const&
    symbols() const { return symbols_; }

private:
    Module &module_;

    std::vector<Token> comma_separated_identifiers();
    std::vector<Token> unit_description();
    std::vector<Expression *> procedures_;
    std::vector<Expression *> functions_;

    // hash table for lookup of variable and call names
    std::unordered_map<std::string, Identifier*> symbols_;

    // helpers for generating unary and binary AST nodes according to
    // a token type passed by the user
    Expression* unary_expression(Location, TOK, Expression*);
    Expression* binary_expression(Location, TOK, Expression*, Expression*);

    // functions for parsing descriptive blocks
    // these are called in the first pass, and do not
    // construct any AST information
    void parse_neuron_block();
    void parse_state_block();
    void parse_units_block();
    void parse_parameter_block();
    void parse_assigned_block();
    void parse_title();

    void skip_block();

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

