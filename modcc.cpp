#include <iostream>

#include "lexer.h"
#include "module.h"
#include "parser.h"
#include "util.h"

int main(int argc, char **argv) {
    for(int i=0; i<1000; ++i) {
        Module m("./modfiles/test.mod");
        Parser p(m);
        if(p.status() != ls_happy) {
            std::cout << "ERROR: unable to parse file" << std::endl;
            return 1;
        }
    }
}
