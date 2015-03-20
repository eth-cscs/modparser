#pragma once

#include <string>
#include <map>
#include <unordered_map>

#include "location.hpp"

enum TOK {
    tok_eof, // end of file

    /////////////////////////////
    // symbols
    /////////////////////////////
    // = + - * / ^
    tok_eq, tok_plus, tok_minus, tok_times, tok_divide, tok_pow,
    // comparison
    tok_not,    // !
    tok_lt,     // <
    tok_lte,    // <=
    tok_gt,     // >
    tok_gte,    // >=
    tok_EQ,     // ==
    tok_ne,     // !=

    // , '
    tok_comma, tok_prime,

    // { }
    tok_lbrace, tok_rbrace,
    // ( )
    tok_lparen, tok_rparen,

    // variable/function names
    tok_identifier,

    // numbers
    tok_number,

    /////////////////////////////
    // keywords
    /////////////////////////////
    // block keywoards
    tok_title,
    tok_neuron, tok_units, tok_parameter,
    tok_assigned, tok_state, tok_breakpoint,
    tok_derivative, tok_procedure, tok_initial, tok_function,
    tok_net_receive,

    // keywoards inside blocks
    tok_unitsoff, tok_unitson,
    tok_suffix, tok_nonspecific_current, tok_useion,
    tok_read, tok_write,
    tok_range, tok_local,
    tok_solve, tok_method,
    tok_threadsafe, tok_global,
    tok_point_process,

    // unary operators
    tok_exp, tok_sin, tok_cos, tok_log,

    // logical keywords
    tok_if, tok_else,

    // solver methods
    tok_cnexp,

    tok_reserved, // placeholder for generating keyword lookup
};

// what is in a token?
//  TOK indicating type of token
//  information about its location
struct Token {
    // the spelling string contains the text of the token as it was written
    // in the input file
    //   type = tok_number     : spelling = "3.1415"  (e.g.)
    //   type = tok_identifier : spelling = "foo_bar" (e.g.)
    //   type = tok_plus       : spelling = "+"       (always)
    //   type = tok_if         : spelling = "if"      (always)
    std::string spelling;
    TOK type;
    Location location;

    Token(TOK tok, std::string const& sp, Location loc=Location(0,0))
    :   spelling(sp),
        type(tok),
        location(loc)
    {}

    Token()
    :   spelling(""),
        type(tok_reserved),
        location(Location())
    {};
};

// lookup table used for checking if an identifier matches a keyword
extern std::unordered_map<std::string, TOK> keyword_map;

// for stringifying a token type
extern std::map<TOK, std::string> token_map;

void initialize_token_maps();
std::string token_string(TOK token);
bool is_keyword(Token const& t);
std::ostream& operator<< (std::ostream& os, Token const& t);

