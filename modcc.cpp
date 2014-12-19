#include <chrono>
#include <iostream>

#include "lexer.h"
#include "module.h"
#include "parser.h"
#include "util.h"

#define VERBOSE

int main(int argc, char **argv) {
    if(argc < 2) {
        std::cout << red("error: ")
                  << "requires file name to parse" << std::endl;
        return 1;
    }

    // load the module from file passed as first argument
    Module m(argv[1]);

    // check that the module is not empty
    if(m.buffer().size()==0) {
        std::cout << red("error: ") << white(argv[1])
                  << " invalid or empty file" << std::endl;
        return 1;
    }

    // initialize the parser
    Parser p(m, false);

    // parse
    std::cout << green("[") + "parsing" + green("]") << std::endl;
    p.parse();
    if(p.status() == ls_error) return 1;

    // semantic analysis
    std::cout << green("[") + "semantic analysis" + green("]") << std::endl;
    p.semantic();

    #ifdef VERBOSE
    std::cout << "====== symbols ======" << std::endl;
    for(auto const &var : p.symbols()) {
        std::cout << var.second.expression->to_string() << std::endl;
    }
    #endif

    if(p.status() == ls_error) return 1;

    #define WITH_PROFILING
    #ifdef WITH_PROFILING
    {
        const int N = 10;
        using clock = std::chrono::high_resolution_clock;
        using duration = std::chrono::duration<double>;

        auto start = clock::now();
        for(int i=0; i<N; ++i) {
            Parser p(m);
            if(p.status() != ls_happy) {
                std::cout << "error: unable to parse file" << std::endl;
                return 1;
            }
        }
        auto last = clock::now();
        auto time_span = duration(last - start);

        std::cout << "compilation takes an average of " << time_span.count()/N*1000. << " ms";
        std::cout << std::endl;
    }
    #endif

    return 0;
}
