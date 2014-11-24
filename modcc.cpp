#include <iostream>

#include "lexer.h"
#include "module.h"
#include "parser.h"
#include "util.h"

int main(int argc, char **argv) {
    Module m("./modfiles/test.mod");

    Parser p(m);
    if(p.status() != ls_happy) {
        std::cout << "ERROR: unable to parse file" << std::endl;
        return 1;
    }

    #ifdef WITH_PROFILING
    for(int i=0; i<10; ++i) {
        Parser p(m);
        if(p.status() != ls_happy) {
            std::cout << "ERROR: unable to parse file" << std::endl;
            return 1;
        }
    }
    #endif
}
