#pragma once

// inspiration was taken from the Digital Mars D compiler
//      github.com/D-Programming-Language/dmd
#include <cassert>

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

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

// status of the lexer
enum LStat {
    k_compiler_error,  // lexer has encounterd a problem
    k_compiler_happy   // lexer is in a good place
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
struct Token {
    // the name string contains the text of the token
    // the string is context sensitive:
    // name should really have been called 'spelling'
    //   type = tok_number     : name = "3.1415"  (e.g.)
    //   type = tok_plus       : name = "+"       (always)
    //   type = tok_if         : name = "if"      (always)
    //   type = tok_identifier : name = "foo_bar" (e.g.)
    std::string name;
    TOK type;
    Location location;

    Token(TOK tok, std::string const& nam, Location loc=Location(0,0))
    :   name(nam),
        type(tok),
        location(loc)
    {}

    Token()
    :   name(""),
        type(tok_reserved),
        location(Location())
    {};
};

extern std::string token_string(TOK token);

std::ostream& operator<< (std::ostream& os, Token const& t);
bool is_keyword(Token const& t);
std::ostream& operator<< (std::ostream& os, Location const& L);


// class that implements the lexer
// takes a range of characters as input parameters
class Lexer {
public:
    Lexer(const char * begin, const char* end)
    :   begin_(begin),
        end_(end),
        current_(begin),
        line_(begin),
        location_()
    {
        assert(begin_<=end_);
        keywords_init();
        token_strings_init();
        binop_prec_init();
    }

    Lexer(std::vector<char> const& v)
    :   Lexer(v.data(), v.data()+v.size())
    {}

    Lexer(std::string const& s)
    :   buffer_(s.data(), s.data()+s.size()+1)
    {
        begin_   = buffer_.data();
        end_     = buffer_.data() + buffer_.size();
        current_ = begin_;
        line_    = begin_;
        //std::cout << "init " << std::endl;
        //std::cout << begin_ << std::endl;
        //std::cout << "close" << std::endl;
    }

    // get the next token
    Token parse();

    void get_token() {
        token_ = parse();
    }

    // return the next token in the stream without advancing the current position
    Token peek();

    // scan a number from the stream
    std::string number();

    // scan an identifier string from the stream
    std::string identifier();

    // scan a character from the stream
    char character();

    Location location() {return location_;}

    // lookup table used for checking if an identifier matches a keyword
    static std::unordered_map<std::string, TOK> keyword_map;
    // for stringifying a token type
    static std::map<TOK, std::string> token_map;
    // binary operator precedence
    static std::map<TOK, int> binop_prec_;

    LStat status() {return status_;}

    const std::string& error_message() {return error_string_;};

    static int binop_precedence(TOK tok);
protected:
    // buffer used for short-lived parsers
    std::vector<char> buffer_;

    // generate lookup tables (hash maps) for keywords
    void keywords_init();
    void token_strings_init();
    void binop_prec_init();

    // helper for determining if an identifier string matches a keyword
    TOK get_identifier_type(std::string const& identifier);

    const char *begin_, *end_;// pointer to start and 1 past the end of the buffer
    const char *current_;     // pointer to current character
    const char *line_;        // pointer to start of current line
    Location location_;  // current location (line,column) in buffer

    LStat status_ = k_compiler_happy;
    std::string error_string_;

    Token token_;
};

