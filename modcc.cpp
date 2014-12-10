#include <iostream>

#include "lexer.h"
#include "module.h"
#include "parser.h"
#include "util.h"

#define VERBOSE

int main(int argc, char **argv) {
    if(argc < 2) {
        std::cout << colorize("error: ", kRed)
                  << "requires file name to parse" << std::endl;
        return 1;
    }

    // load the module from file passed as first argument
    Module m(argv[1]);

    // check that the module is not empty
    if(m.buffer().size()==0) {
        std::cout << colorize("error: ", kRed)
                  << colorize(argv[1], kGreen)
                  << " invalid or empty file" << std::endl;
        return 1;
    }

    // initialize the parser
    Parser p(m, false);

    std::cout << "parsing " + colorize(argv[1], kGreen) + "..." << std::endl;

    // parse
    p.parse();
    if(p.status() != ls_happy) {
        std::cerr << colorize("error: ", kRed) << p.error_message() << std::endl;
        return 1;
    }
    std::cout << "... parsed" << std::endl;

    // semantic analysis
    p.semantic();
    if(p.status() != ls_happy) {
        std::cerr << colorize("error: ", kRed) << p.error_message() << std::endl;
        return 1;
    }
    std::cout << "... semantic analysis " << std::endl;

    #ifdef VERBOSE
    std::string twiz = colorize("=", kGreen) + colorize("=", kYellow) + colorize("=", kRed);
    twiz += twiz;
    std::cout << twiz + " symbols " + twiz << std::endl;
    for(auto const &var : p.symbols()) {
        if(var.second->is_variable())
            std::cout << *dynamic_cast<Variable*>(var.second) << std::endl;
        else
            std::cout << var.second->expression()->to_string() << std::endl;
    }
    #endif
    #ifdef WITH_PROFILING
    for(int i=0; i<10; ++i) {
        Parser p(m);
        if(p.status() != ls_happy) {
            std::cout << "error: unable to parse file" << std::endl;
            return 1;
        }
    }
    #endif

    return 0;
}
