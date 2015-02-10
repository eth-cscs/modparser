#include <chrono>
#include <iostream>
#include <fstream>

#include <tclap/include/CmdLine.h>

#include "cprinter.hpp"
#include "lexer.h"
#include "module.h"
#include "parser.h"
#include "util.h"

//#define WITH_PROFILING
//#define VERBOSE

struct Options {
    std::string filename;
    std::string outputname;
    bool has_output = false;
    bool verbose = true;
};

int main(int argc, char **argv) {

    Options options;

    // parse command line arguments
    try {
        TCLAP::CmdLine cmd("welcome to mod2c", ' ', "0.1");

        // input file name (to load multiple files we have to use UnlabeledMultiArg
        TCLAP::UnlabeledValueArg<std::string>
            fin_arg("input_file", "the name of the .mod file to compile", true, "", "filename");
        // output filename
        TCLAP::ValueArg<std::string>
            fout_arg("o","output","name of output file", false,"","filename");
        // verbose mode
        TCLAP::SwitchArg verbose_arg("V","verbose","toggle verbose mode", cmd, false);

        cmd.add(fin_arg);
        cmd.add(fout_arg);

        cmd.parse(argc, argv);

        options.outputname = fout_arg.getValue();
        options.has_output = options.outputname.size()>0;
        options.filename = fin_arg.getValue();
        options.verbose = verbose_arg.getValue();
    }
    // catch any exceptions in command line handling
    catch(TCLAP::ArgException &e) {
        std::cerr << "error: " << e.error()
                  << " for arg " << e.argId()
                  << std::endl;
    }

    // load the module from file passed as first argument
    Module m(options.filename.c_str());

    // check that the module is not empty
    if(m.buffer().size()==0) {
        std::cout << red("error: ") << white(argv[1])
                  << " invalid or empty file" << std::endl;
        return 1;
    }

    std::cout << yellow("compiling ") << white(options.filename) << std::endl;

    // initialize the parser
    Parser p(m, false);

    // parse
    if(options.verbose) std::cout << green("[") + "parsing" + green("]") << std::endl;
    p.parse();
    if(p.status() == k_compiler_error) return 1;

    // semantic analysis
    if(options.verbose) std::cout << green("[") + "semantic analysis" + green("]") << std::endl;
    if( m.semantic() == false ) {
        std::cout << m.error_string() << std::endl;
    }

    #ifdef VERBOSE
    std::cout << "====== symbols ======" << std::endl;
    for(auto const &var : m.symbols()) {
        std::cout << var.second.expression->to_string() << std::endl;
    }
    #endif

    if(m.status() == k_compiler_error) return 1;

    #ifdef WITH_PROFILING
    {
        const int N = 10;
        using clock = std::chrono::high_resolution_clock;
        using duration = std::chrono::duration<double>;

        auto start = clock::now();
        for(int i=0; i<N; ++i) {
            Module m(options.filename.c_str());
            Parser p(m);
            if(p.status() != k_compiler_happy) {
                std::cout << "error: unable to parse file" << std::endl;
                return 1;
            }
            m.semantic();
        }
        auto last = clock::now();
        auto time_span = duration(last - start);

        std::cout << "compilation takes an average of " << time_span.count()/N*1000. << " ms";
        std::cout << std::endl;
    }
    #endif

    // generate output
    if(options.verbose) {
        std::cout << green("[") + "output "
                  << (options.has_output ? white(options.outputname) : "stdout")
                  << green("]") << std::endl;
    }

    if(options.has_output) {
        std::ofstream fout(options.outputname);
        fout << CPrinter(m).text();
        fout.close();
    }
    else {
        std::cout << cyan("--------------------------------------") << std::endl;
        std::cout << CPrinter(m).text();
        std::cout << cyan("--------------------------------------") << std::endl;
    }

    return 0;
}
