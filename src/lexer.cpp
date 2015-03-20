#include <cstdio>

#include <iostream>
#include <string>

#include "lexer.hpp"
#include "util.hpp"

// helpers for identifying character types
inline bool in_range(char c, char first, char last) {
    return c>=first && c<=last;
}
inline bool is_numeric(char c) {
    return in_range(c, '0', '9');
}
inline bool is_alpha(char c) {
    return (in_range(c, 'a', 'z') || in_range(c, 'A', 'Z') );
}
inline bool is_alphanumeric(char c) {
    return (is_numeric(c) || is_alpha(c) );
}
inline bool is_whitespace(char c) {
    return (c==' ' || c=='\t' || c=='\v' || c=='\f');
}
inline bool is_eof(char c) {
    return (c==0 || c==EOF);
}
inline bool is_operator(char c) {
    return (c=='+' || c=='-' || c=='*' || c=='/' || c=='^' || c=='\'');
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
                t.spelling = "eof";
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

            // new line
            case '\r'   :
                current_++;
                if(*current_ != '\n') {
                    error_string_ = pprintf("bad line ending: \\n must follow \\r");
                    status_ = k_compiler_error;
                    t.type = tok_reserved;
                    return t;
                }
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
                t.spelling = number();

                // test for error when reading number
                t.type = (status_==k_compiler_error) ? tok_reserved : tok_number;
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
                t.spelling = identifier();
                t.type
                    = status_==k_compiler_error
                    ? tok_reserved
                    : get_identifier_type(t.spelling);
                return t;
            case '(':
                t.type = tok_lparen;
                t.spelling += character();
                return t;
            case ')':
                t.type = tok_rparen;
                t.spelling += character();
                return t;
            case '{':
                t.type = tok_lbrace;
                t.spelling += character();
                return t;
            case '}':
                t.type = tok_rbrace;
                t.spelling += character();
                return t;
            case '=': {
                t.spelling += character();
                if(*current_=='=') {
                    t.spelling += character();
                    t.type=tok_EQ;
                }
                else {
                    t.type = tok_eq;
                }
                return t;
            }
            case '!': {
                t.spelling += character();
                if(*current_=='=') {
                    t.spelling += character();
                    t.type=tok_ne;
                }
                else {
                    t.type = tok_not;
                }
                return t;
            }
            case '+':
                t.type = tok_plus;
                t.spelling += character();
                return t;
            case '-':
                t.type = tok_minus;
                t.spelling += character();
                return t;
            case '/':
                t.type = tok_divide;
                t.spelling += character();
                return t;
            case '*':
                t.type = tok_times;
                t.spelling += character();
                return t;
            case '^':
                t.type = tok_pow;
                t.spelling += character();
                return t;
            // comparison binary operators
            case '<': {
                t.spelling += character();
                if(*current_=='=') {
                    t.spelling += character();
                    t.type = tok_lte;
                }
                else {
                    t.type = tok_lt;
                }
                return t;
            }
            case '>': {
                t.spelling += character();
                if(*current_=='=') {
                    t.spelling += character();
                    t.type = tok_gte;
                }
                else {
                    t.type = tok_gt;
                }
                return t;
            }
            case '\'':
                t.type = tok_prime;
                t.spelling += character();
                return t;
            case ',':
                t.type = tok_comma;
                t.spelling += character();
                return t;
            default:
                error_string_ = pprintf("found unexpected character '%' when trying to find next token", *current_);
                status_ = k_compiler_error;
                t.spelling += character();
                t.type = tok_reserved;
                return t;
        }
    }

    // return the token
    return t;
}

Token Lexer::peek() {
    // save the current position
    const char *oldpos  = current_;
    const char *oldlin  = line_;
    Location    oldloc  = location_;

    Token t = parse(); // read the next token

    // reset position
    current_  = oldpos;
    location_ = oldloc;
    line_     = oldlin;

    return t;
}

// scan floating point number from stream
std::string Lexer::number() {
    std::string str;
    char c = *current_;

    // start counting the number of points in the number
    uint8_t num_point = (c=='.' ? 1 : 0);

    // shouldn't we return a string too?
    str += c;
    current_++;
    while(1) {
        c = *current_;
        if(is_numeric(c)) {
            str += c;
            current_++;
        }
        else if(c=='.') {
            num_point++;
            str += c;
            current_++;
        }
        else {
            break;
        }
    }

    // check that there is at most one decimal point
    // i.e. disallow values like 2.2324.323
    if(num_point>1) {
        error_string_ = pprintf("too many .'s when reading the number '%'", colorize(str, kYellow));
        status_ = k_compiler_error;
    }

    return str;
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
    if( !(is_alpha(c) || c=='_') ) {
        throw compiler_exception(
            "Lexer attempting to read number when none is available",
            location_);
    }

    name += c;
    current_++;
    while(1) {
        c = *current_;

        if(is_alphanumeric(c) || c=='_') {
            name += c;
            current_++;
        }
        else {
            break;
        }
    }

    return name;
}

// scan a single character from the buffer
char Lexer::character() {
    return *current_++;
}

std::map<TOK, int> Lexer::binop_prec_;

void Lexer::binop_prec_init() {
    if(binop_prec_.size()>0)
        return;

    // I have taken the operator precedence from C++
    binop_prec_[tok_eq]     = 2;
    binop_prec_[tok_EQ]     = 4;
    binop_prec_[tok_ne]     = 4;
    binop_prec_[tok_lt]     = 5;
    binop_prec_[tok_lte]    = 5;
    binop_prec_[tok_gt]     = 5;
    binop_prec_[tok_gte]    = 5;
    binop_prec_[tok_plus]   = 10;
    binop_prec_[tok_minus]  = 10;
    binop_prec_[tok_times]  = 20;
    binop_prec_[tok_divide] = 20;
    binop_prec_[tok_pow]    = 30;
}

int Lexer::binop_precedence(TOK tok) {
    auto r = binop_prec_.find(tok);
    if(r==binop_prec_.end())
        return -1;
    return r->second;
}

// pre  : identifier is a valid identifier ([_a-zA-Z][_a-zA-Z0-9]*)
// post : if(identifier is a keyword) return tok_<keyword>
//        else                        return tok_identifier
TOK Lexer::get_identifier_type(std::string const& identifier) {
    auto pos = keyword_map.find(identifier);
    return pos==keyword_map.end() ? tok_identifier : pos->second;
}

