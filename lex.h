#include <cassert>

enum Token {
    tok_eof, // end of file

    // symbols
    // = + - * /
    tok_eq, tok_plus, tok_minus, tok_times, tok_div,
    // ,
    tok_comma

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
    tok_solve, tok_method
};

class Lexer {
public:
    Lexer(char* begin, char* end) :
        begin_(begin), end_(end),
        size_(end-begin), position_(0)
    {
        assert(begin_<=end_);
    }


private:
    char *begin_, *end_;
    size_t position_;
    size_t size_;
}


