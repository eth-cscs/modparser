#include <mutex>

#include "token.hpp"

// lookup table used for checking if an identifier matches a keyword
std::unordered_map<std::string, TOK> keyword_map;

// for stringifying a token type
std::map<TOK, std::string> token_map;

std::mutex mutex;

struct Keyword {
    const char *name;
    TOK type;
};

struct TokenString {
    const char *name;
    TOK token;
};

static Keyword keywords[] = {
    {"TITLE",       tok_title},
    {"NEURON",      tok_neuron},
    {"UNITS",       tok_units},
    {"PARAMETER",   tok_parameter},
    {"ASSIGNED",    tok_assigned},
    {"STATE",       tok_state},
    {"BREAKPOINT",  tok_breakpoint},
    {"DERIVATIVE",  tok_derivative},
    {"PROCEDURE",   tok_procedure},
    {"FUNCTION",    tok_function},
    {"INITIAL",     tok_initial},
    {"NET_RECEIVE", tok_net_receive},
    {"UNITSOFF",    tok_unitsoff},
    {"UNITSON",     tok_unitson},
    {"SUFFIX",      tok_suffix},
    {"NONSPECIFIC_CURRENT", tok_nonspecific_current},
    {"USEION",      tok_useion},
    {"READ",        tok_read},
    {"WRITE",       tok_write},
    {"RANGE",       tok_range},
    {"LOCAL",       tok_local},
    {"SOLVE",       tok_solve},
    {"THREADSAFE",  tok_threadsafe},
    {"GLOBAL",      tok_global},
    {"POINT_PROCESS", tok_point_process},
    {"METHOD",      tok_method},
    {"if",          tok_if},
    {"else",        tok_else},
    {"cnexp",       tok_cnexp},
    {"exp",         tok_exp},
    {"sin",         tok_sin},
    {"cos",         tok_cos},
    {"log",         tok_log},
    {nullptr,       tok_reserved},
};

static TokenString token_strings[] = {
    {"=",           tok_eq},
    {"+",           tok_plus},
    {"-",           tok_minus},
    {"*",           tok_times},
    {"/",           tok_divide},
    {"^",           tok_pow},
    {"<",           tok_lt},
    {"<=",          tok_lte},
    {">",           tok_gt},
    {">=",          tok_gte},
    {"==",          tok_EQ},
    {"!=",          tok_ne},
    {",",           tok_comma},
    {"'",           tok_prime},
    {"{",           tok_lbrace},
    {"}",           tok_rbrace},
    {"(",           tok_lparen},
    {")",           tok_rparen},
    {"identifier",  tok_identifier},
    {"number",      tok_number},
    {"TITLE",       tok_title},
    {"NEURON",      tok_neuron},
    {"UNITS",       tok_units},
    {"PARAMETER",   tok_parameter},
    {"ASSIGNED",    tok_assigned},
    {"STATE",       tok_state},
    {"BREAKPOINT",  tok_breakpoint},
    {"DERIVATIVE",  tok_derivative},
    {"PROCEDURE",   tok_procedure},
    {"FUNCTION",    tok_function},
    {"INITIAL",     tok_initial},
    {"NET_RECEIVE", tok_net_receive},
    {"UNITSOFF",    tok_unitsoff},
    {"UNITSON",     tok_unitson},
    {"SUFFIX",      tok_suffix},
    {"NONSPECIFIC_CURRENT", tok_nonspecific_current},
    {"USEION",      tok_useion},
    {"READ",        tok_read},
    {"WRITE",       tok_write},
    {"RANGE",       tok_range},
    {"LOCAL",       tok_local},
    {"SOLVE",       tok_solve},
    {"THREADSAFE",  tok_threadsafe},
    {"GLOBAL",      tok_global},
    {"POINT_PROCESS", tok_point_process},
    {"METHOD",      tok_method},
    {"if",          tok_if},
    {"else",        tok_else},
    {"eof",         tok_eof},
    {"exp",         tok_exp},
    {"log",         tok_log},
    {"cos",         tok_cos},
    {"sin",         tok_sin},
    {"cnexp",       tok_cnexp},
    {"error",       tok_reserved},
};

/// set up lookup tables for converting between tokens and their
/// string representations
void initialize_token_maps() {
    // ensure that tables are initialized only once
    std::lock_guard<std::mutex> g(mutex);

    if(keyword_map.size()==0) {
        //////////////////////
        /// keyword map
        //////////////////////
        for(int i = 0; keywords[i].name!=nullptr; ++i) {
            keyword_map.insert( {keywords[i].name, keywords[i].type} );
        }

        //////////////////////
        // token map
        //////////////////////
        int i;
        for(i = 0; token_strings[i].token!=tok_reserved; ++i) {
            token_map.insert( {token_strings[i].token, token_strings[i].name} );
        }
        // insert the last token: tok_reserved
        token_map.insert( {token_strings[i].token, token_strings[i].name} );
    }
}

std::string token_string(TOK token) {
    auto pos = token_map.find(token);
    return pos==token_map.end() ? std::string("<unknown token>") : pos->second;
}

bool is_keyword(Token const& t) {
    for(Keyword *k=keywords; k->name!=nullptr; ++k)
        if(t.type == k->type)
            return true;
    return false;
}

std::ostream& operator<< (std::ostream& os, Token const& t) {
    return os << "<<" << token_string(t.type) << ", " << t.spelling << ", " << t.location << ">>";
}
