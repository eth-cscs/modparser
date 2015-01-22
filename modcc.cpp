#include <chrono>
#include <iostream>

#include "cprinter.h"
#include "lexer.h"
#include "module.h"
#include "parser.h"
#include "util.h"

//#define VERBOSE

int main(int argc, char **argv) {
    if(argc < 2) {
        std::cout << red("error: ")
                  << "requires file name to parse" << std::endl;
        return 1;
    }

    // load the module from file passed as first argument
    std::string filename = argv[1];
    std::string basename = filename.substr(0, filename.find_last_of("."));

    Module m(filename.c_str());

    // check that the module is not empty
    if(m.buffer().size()==0) {
        std::cout << red("error: ") << white(argv[1])
                  << " invalid or empty file" << std::endl;
        return 1;
    }

    std::cout << "parsing file " << white(filename) << std::endl;

    // initialize the parser
    Parser p(m, false);

    // parse
    std::cout << green("[") + "parsing" + green("]") << std::endl;
    p.parse();
    if(p.status() == k_compiler_error) return 1;

    // semantic analysis
    std::cout << green("[") + "semantic analysis" + green("]") << std::endl;
    if( m.semantic() == false ) {
        std::cout << m.error_string() << std::endl;
    }

    #ifdef VERBOSE
    std::cout << "====== symbols ======" << std::endl;
    for(auto const &var : m.symbols()) {
        std::cout << var.second.expression->to_string() << std::endl;
    }
    #endif

    auto proctest = [] (procedureKind k) {return k == k_proc_normal || k == k_proc_api;};
    for(auto const &var : m.symbols()) {
        if(var.second.kind==k_symbol_procedure && proctest(var.second.expression->is_procedure()->kind())) {
            auto v = new CPrinter();
            var.second.expression->accept(v);
            std::cout << v->text();
        }
    }

    if(m.status() == k_compiler_error) return 1;

    //#define WITH_PROFILING
    #ifdef WITH_PROFILING
    {
        const int N = 10;
        using clock = std::chrono::high_resolution_clock;
        using duration = std::chrono::duration<double>;

        auto start = clock::now();
        for(int i=0; i<N; ++i) {
            Parser p(m);
            if(p.status() != k_compiler_happy) {
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

    // generate output
    auto outputname = basename + ".h";
    std::cout << "output file name " << white(outputname) << std::endl;

    return 0;
}
