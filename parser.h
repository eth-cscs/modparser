#pragma once

#include "expression.h"
#include "lexer.h"
#include "module.h"

class Parser : public Lexer {
public:
    explicit Parser(Module& m, bool advance=true);
    bool description_pass();

    Expression* parse_prototype();
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

    std::string const& error_message() {
        return error_string_;
    }

    ProcedureExpression* parse_procedure();

    std::vector<Expression*>&
    procedures() { return procedures_; }

    std::vector<Expression*>const&
    procedures() const { return procedures_; }

    std::unordered_map<std::string, Identifier*>&
    identifiers() { return identifiers_; }

    std::unordered_map<std::string, Identifier*>const&
    identifiers() const { return identifiers_; }

private:
    Module &module_;

    std::vector<Token> comma_separated_identifiers();
    std::vector<Token> unit_description();
    std::vector<std::pair<Token, const char*>> verb_blocks_;
    std::vector<Expression *> procedures_;

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
    void build_identifiers();

    // helper function for logging errors
    void error(std::string msg);

    // disable default and copy assignment
    Parser();
    Parser(Parser const &);

    bool expect(TOK, const char *str="");
    bool expect(TOK, std::string const& str);

    // hash table for lookup of variable and call names
    std::unordered_map<std::string, Identifier*> identifiers_;
};

