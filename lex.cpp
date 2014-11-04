#include <cassert>
#include <cstdio>

#include <iostream>
#include <string>

#include "lex.h"

inline bool in_range(const char &c, char first, char last) {
    return c>=first && c<=last;
}
// helpers for identifying character types
inline bool is_numeric(const char &c) {
    return in_range(c, '0', '9');
}
inline bool is_alpha(const char &c) {
    return (in_range(c, 'a', 'z') || in_range(c, 'A', 'Z') );
}
inline bool is_alphanumeric(const char &c) {
    return (is_numeric(c) || is_alpha(c) );
}
inline bool is_whitespace(const char &c) {
    return (c==' ' || c=='\t' || c=='\v' || c=='\f');
}
inline bool is_eof(const char &c) {
    return (c==0 || c==EOF);
}
inline bool is_operator(const char &c) {
    return (c=='+' || c=='-' || c=='*' || c=='/');
}

//*********************
// Lexer
//*********************

Token Lexer::parse() {
    Token t;

    // the while loop strips white space/new lines in front of the next token
    while(1) {
        location_.column = current_-line_+1;
        t.location = location_;

        switch(*current_) {
            // end of file
            case 0      :       // end of string
            case EOF    :       // end of file
                t.type = tok_eof;
                return t;

            // white space
            case ' '    :
            case '\t'   :
            case '\v'   :
            case '\f'   :
                current_++;
                continue;   // skip to next character

            // new line
            case '\n'   :
                current_++;
                line_ = current_;
                location_.line++;
                continue;   // skip to next line

            // comment (everything after : on a line is a comment)
            case ':'    :
                // strip characters until either end of file or end of line
                while( !is_eof(*current_) && *current_ != '\n') {
                    ++current_;
                }
                continue;

            // number
            case '0': case '1' : case '2' : case '3' : case '4':
            case '5': case '6' : case '7' : case '8' : case '9':
            case '.':
                t.type = tok_number;
                t.value = number();
                return t;

            // identifier or keyword
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
            case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
            case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
            case 'v': case 'w': case 'x': case 'y': case 'z':
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
            case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
            case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
            case 'V': case 'W': case 'X': case 'Y': case 'Z':
            case '_':
                // get std::string of the identifier
                // TODO handle keywords
                t.name = identifier();
                t.type = tok_identifier;
                return t;
            case '(':
                t.type = tok_lparen;
                t.name += character();
                return t;
            case ')':
                t.type = tok_rparen;
                t.name += character();
                return t;
            case '{':
                t.type = tok_lbrace;
                t.name += character();
                return t;
            case '}':
                t.type = tok_rbrace;
                t.name += character();
                return t;
            case '+':
                t.type = tok_plus;
                t.name += character();
                return t;
            case '-':
                t.type = tok_minus;
                t.name += character();
                return t;
            case '/':
                t.type = tok_divide;
                t.name += character();
                return t;
            case '*':
                t.type = tok_times;
                t.name += character();
                return t;
            case ',':
                t.type = tok_comma;
                t.name += character();
                return t;
            default:
                std::cerr << "error: unexpected character "
                          << (uint8_t)(*current_)
                          << " in input" << std::endl;
                assert(0);
        }
    }

    // return the token
    return t;
}

// scan floating point number from stream
double Lexer::number() {
    std::string str;
    char c = *current_;

    // assert that current position is at the start of a number
    assert( is_numeric(c) || c=='.' );
    uint8_t num_point = (c=='.' ? 1 : 0);

    // shouldn't we return a string too?
    str += c;
    current_++;
    while(1) {
        c = *current_;
        // finish if EOF or white space
        if(is_eof(c) || is_whitespace(c)) {
            break;
        }
        if(c=='\n') {
            break;
        }
        // a number can be followed by mathematical operator
        else if(is_operator(c)) {
            break;
        }
        // a number can be followed by parenthesis or a comma or a comment
        else if(c=='(' || c==',' || c==':') {
            break;
        }
        else if(is_numeric(c)) {
            str += c;
            current_++;
        }
        else if(c=='.') {
            num_point++;
            str += c;
            current_++;
        }
        // found an unexpected value
        // e.g. the following [a-zA-Z]
        else {
            std::cerr << "error : found unexpected character "
                      << c << " when reading a number"
                      << std::endl;
            assert(0);
        }
    }

    // check that there is at most one decimal point
    // i.e. disallow values like 2.2324.323
    assert(num_point<2);

    // convert string to double
    return std::stod(str);
}

// scan identifier from stream
//  examples of valid names:
//      _1 _a ndfs var9 num_a This_
//  examples of invalid names:
//      _ __ 9val 9_
std::string Lexer::identifier() {
    std::string name;
    char c = *current_;

    // assert that current position is at the start of a number
    // note that the first character can't be numeric
    assert( is_alpha(c) || c=='_' );

    name += c;
    current_++;
    while(1) {
        c = *current_;
        // finish if EOF or white space
        if(is_eof(c) || is_whitespace(c)) {
            break;
        }
        if(c=='\n') {
            break;
        }
        // an identifyer can be followed by mathematical operator
        else if(is_operator(c)) {
            break;
        }
        // an identifier can be followed by open or closed parenthesis
        else if(c=='(' || c ==')') {
            break;
        }
        // an identifier can be followed by a comma or a comment
        else if(c==',' || c==':') {
            break;
        }
        else if(is_alphanumeric(c) || c=='_') {
            name += c;
            current_++;
        }
        // found an unexpected value
        // e.g. the following [a-zA-Z]
        else {
            std::cerr << "error : found unexpected character '"
                      << c << "' when reading a number"
                      << std::endl;
            assert(0);
        }
    }

    return name;
}

// scan a single character from the buffer
char Lexer::character() {
    return *current_++;
}

//*********************
// Keywords
//*********************

struct Keyword {
    const char *name;
    TOK type;
};

static Keyword keywords[] = {
    {"NEURON",      tok_neuron},
    {"UNITS",       tok_units},
    {"PARAMETER",   tok_parameter},
    {"ASSIGNED",    tok_assigned},
    {"STATE",       tok_state},
    {"BREAKPOINT",  tok_breakpoint},
    {"DERIVATIVE",  tok_derivative},
    {"PROCEDURE",   tok_procedure},
    {"UNITSOFF",    tok_unitsoff},
    {"UNITSON",     tok_unitson},
    {"SUFFIX",      tok_suffix},
    {"NONSPECIFIC_CURRENT", tok_nonspecific_current},
    {"USEION",      tok_useion},
    {"READ",        tok_read},
    {"WRITE",       tok_write},
    {"RANGE",       tok_range},
    {"SOLVE",       tok_solve},
    {"METHOD",      tok_method},
    {"if",          tok_if},
    {"else",        tok_else},
    {nullptr,       tok_reserved},
};

std::unordered_map<std::string, TOK> Lexer::keyword_map;

void Lexer::keywords_init() {
    // check whether the map has already been initialized
    if(this->keyword_map.size()>0)
        return;

    for(int i = 0; keywords[i].name!=nullptr; ++i) {
        keyword_map.insert( {keywords[i].name, keywords[i].type} );
    }
}


