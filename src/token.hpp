#pragma once

#include <string>
#include <map>
#include <unordered_map>

#include "location.hpp"

enum class tok {
    eof, // end of file

    /////////////////////////////
    // symbols
    /////////////////////////////
    // = + - * / ^
    eq, plus, minus, times, divide, pow,
    // comparison
    lnot,    // !   named logical not, to avoid clash with C++ not keyword
    lt,      // <
    lte,     // <=
    gt,      // >
    gte,     // >=
    equality,// ==
    ne,      // !=

    // , '
    comma, prime,

    // { }
    lbrace, rbrace,
    // ( )
    lparen, rparen,

    // variable/function names
    identifier,

    // numbers
    number,

    /////////////////////////////
    // keywords
    /////////////////////////////
    // block keywoards
    title,
    neuron, units, parameter,
    assigned, state, breakpoint,
    derivative, procedure, initial, function,
    net_receive,

    // keywoards inside blocks
    unitsoff, unitson,
    suffix, nonspecific_current, useion,
    read, write,
    range, local,
    solve, method,
    threadsafe, global,
    point_process,

    // unary operators
    exp, sin, cos, log,

    // logical keywords
    if_stmt, else_stmt, // add _stmt to avoid clash with c++ keywords

    // solver methods
    cnexp,

    conductance,

    reserved, // placeholder for generating keyword lookup
};

// what is in a token?
//  tok indicating type of token
//  information about its location
struct Token {
    // the spelling string contains the text of the token as it was written
    // in the input file
    //   type = tok::number     : spelling = "3.1415"  (e.g.)
    //   type = tok::identifier : spelling = "foo_bar" (e.g.)
    //   type = tok::plus       : spelling = "+"       (always)
    //   type = tok::if         : spelling = "if"      (always)
    std::string spelling;
    tok type;
    Location location;

    Token(tok tok, std::string const& sp, Location loc=Location(0,0))
    :   spelling(sp),
        type(tok),
        location(loc)
    {}

    Token()
    :   spelling(""),
        type(tok::reserved),
        location(Location())
    {};
};

// lookup table used for checking if an identifier matches a keyword
extern std::unordered_map<std::string, tok> keyword_map;

// for stringifying a token type
extern std::map<tok, std::string> token_map;

void initialize_token_maps();
std::string token_string(tok token);
bool is_keyword(Token const& t);
std::ostream& operator<< (std::ostream& os, Token const& t);

