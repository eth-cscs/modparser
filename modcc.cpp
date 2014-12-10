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

    // parse the module data
    Parser p(m);
    if(p.status() != ls_happy) {
        // return with error
        return 1;
    }

    #ifdef VERBOSE
    std::string twiz = colorize("=", kGreen) + colorize("=", kYellow) + colorize("=", kRed);
    twiz += twiz;
    std::cout << twiz + " variables " + twiz << std::endl;
    for(auto const &var : p.identifiers()) {
        std::cout << *dynamic_cast<Variable*>(var.second) << std::endl;
    }
    std::cout << twiz + " procedures " + twiz << std::endl;
    for(auto const &proc: p.procedures()) {
        std::cout << proc->to_string() << std::endl << std::endl;
    }
    std::cout << twiz + " functions " + twiz << std::endl;
    for(auto const &fun: p.functions()) {
        std::cout << fun->to_string() << std::endl << std::endl;
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

    std::cout << "succesfully parsed " + colorize(argv[1], kGreen)
              << std::endl;
    return 0;
}
