#pragma once

#include "../gtest.h"

#include "../src/parser.h"
#include "../src/util.h"

//#define VERBOSE_TEST
#ifdef VERBOSE_TEST
#define VERBOSE_PRINT(x) std::cout << (x) << std::endl;
#else
#define VERBOSE_PRINT(x)
#endif

static Expression* parse_line_expression(std::string const& s) {
    return Parser(s).parse_line_expression();
}

static Expression* parse_expression(std::string const& s) {
    return Parser(s).parse_expression();
}

static Expression* parse_function(std::string const& s) {
    return Parser(s).parse_function();
}

static Expression* parse_procedure(std::string const& s) {
    return Parser(s).parse_procedure();
}

