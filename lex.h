#pragma once

// inspiration was taken from the Digital Mars D compiler
//      github.com/D-Programming-Language/dmd
#include <cassert>

#include <string>
#include <unordered_map>

enum TOK {
    tok_eof, // end of file

    // symbols
    // = + - * /
    tok_eq, tok_plus, tok_minus, tok_times, tok_divide,
    // ,
    tok_comma,

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
    tok_neuron, tok_units, tok_parameter,
    tok_assigned, tok_state, tok_breakpoint,
    tok_derivative, tok_procedure,

    // keywoards inside blocks
    tok_unitsoff, tok_unitson,
    tok_suffix, tok_nonspecific_current, tok_useion,
    tok_read, tok_write,
    tok_range,
    tok_solve, tok_method,

    // logical keywords
    tok_if, tok_else,

    tok_reserved, // placeholder for generating keyword lookup
};

// status of the lexer
enum LStat {
    ls_error,  // lexer has encounterd a problem
    ls_happy   // lexer is in a good place
};

struct Location {
    int line;
    int column;

    Location() : Location(1, 1)
    {}

    Location(int ln, int col)
    :   line(ln),
        column(col)
    {}
};

// what is in a token?
//  TOK indicating type of token
//  information about its location
//  value information if it is 
struct Token {
    Location location;
    TOK type;

    // don't bother using a union or an abstract base class for storing metadata associated
    // with different tokens, because the .mod files are very small, so memory constraints are
    // not an issue
    double value;
    std::string name;
};

bool is_keyword(Token const& t);


// class that implements the lexer
// takes a range of characters as input parameters
class Lexer {
public:
    Lexer(char* begin, char* end)
    :   begin_(begin),
        end_(end),
        current_(begin),
        location_(),
        line_(begin)
    {
        assert(begin_<=end_);
        keywords_init();
    }

    // get the next token
    Token parse();

    // scan a number from the stream
    double number();

    // scan an identifier string from the stream
    std::string identifier();

    // scan a character from the stream
    char character();

    Location location() {return location_;}

    // lookup table used for checking if an identifier matches a keyword
    static std::unordered_map<std::string, TOK> keyword_map;

    LStat status() {return status_;}

    const std::string& error_message() {return error_string_;};
private:
    // generate lookup tables (hash maps) for keywords
    void keywords_init();
    // helper for determining if an identifier string matches a keyword
    TOK get_identifier_type(std::string const& identifier);

    char *begin_, *end_;// pointer to start and 1 past the end of the buffer
    char *current_;     // pointer to current character
    char *line_;        // pointer to start of current line
    Location location_;  // current location (line,column) in buffer

    LStat status_ = ls_happy;
    std::string error_string_;
};
