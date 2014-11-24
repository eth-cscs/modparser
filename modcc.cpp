#include <iostream>

#include "lexer.h"
#include "module.h"
#include "parser.h"
#include "util.h"

int main(int argc, char **argv) {
    Module m("./modfiles/test.mod");
    for(int i=0; i<100000; ++i) {
        Parser p(m);
        if(p.status() != ls_happy) {
            std::cout << "ERROR: unable to parse file" << std::endl;
            return 1;
        }
    }
}
