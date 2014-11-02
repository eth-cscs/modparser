#include <cstdio>
#include <string>

#include "lex.h"

// helpers for identifying character types
inline bool is_numeric(const char &c) {
    return (c>='0' && c<='9');
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

/**********************
 * Lexer
 **********************/

Token Lexer::parse() {
    Token t;

    // the while loop strips white space/new lines in front of the next token
    while(1) {
        location.column = line_-current_;
        t.location = location;

        switch(*current_) {
            // end of file
            case 0      :       // end of string
            case EOF    :       // end of file
                t.token = tok_eof;
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
                continue;   // skip to next line

            // number
            case '0': case '1' : case '2' : case '3' : case '4':
            case '5': case '6' : case '7' : case '8' : case '9':
            case '.':
                t.token = tok_number;
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
                t.name name = identifier();
                //if(is_keyword(name))
                break;
            case '(':
                t.token = tok_lparen;
                break;
            case ')':
                t.token = tok_rparen;
                break;
            case '{':
                t.token = tok_lbrace;
                break;
            case '}':
                t.token = tok_rbrace;
                break;
            case '+':
                t.token = tok_plus;
                break;
            case '-':
                t.token = tok_minus;
                break;
            case '/':
                t.token = tok_divide;
                break;
            case '*':
                t.token = tok_times;
                break;
            case ',':
                t.token = tok_comma;
                break;
        }
    }

    // return the token
    return t;
}

// scan floating point number from stream
double Lexer::number() {
    std::string num_str;
    char c = *current_;

    // assert that current position is at the start of a number
    assert( is_numeric(c) || c=='.' );
    uint8_t num_point = c=='.' ? 1 : 0;

    // shouldn't we return a string too?
    num_str += c;
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
        else if(is_numeric(c)) {
            num_str += c;
            current_++;
        }
        else if(c=='.') {
            num_point++;
            num_str += c;
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
    return std::stod(num_str);
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
    assert( is_alphanumeric(c) || c=='_' );

    // shouldn't we return a string too?
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
        // a number can be followed by a parenthesis
        else if(c=='(' || c ==')') {
            break;
        }
        else if(is_alphanumeric(c) || c=='_') {
            name += c;
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
    // TODO : add check that name contains something other than underscores
    assert(num_point<2);

    // convert string to double
    return std::stod(num_str);
}
