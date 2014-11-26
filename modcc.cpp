#include <iostream>

#include "lexer.h"
#include "module.h"
#include "parser.h"
#include "util.h"

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

    #ifdef WITH_PROFILING
    for(int i=0; i<10; ++i) {
        Parser p(m);
        if(p.status() != ls_happy) {
            std::cout << "error: unable to parse file" << std::endl;
            return 1;
        }
    }
    #endif

    std::cout << colorize("succesfully parsed ", kYellow)
              << colorize(argv[1], kGreen)
              << std::endl;
    return 0;
}
