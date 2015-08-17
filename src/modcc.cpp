#include <chrono>
#include <iostream>
#include <fstream>

#include <tclap/include/CmdLine.h>

#include "cprinter.hpp"
//#include "cudaprinter.hpp"
//#include "cymeprinter.hpp"
#include "lexer.hpp"
#include "module.hpp"
#include "parser.hpp"
#include "util.hpp"

//#define VERBOSE

enum class targetKind {cpu, gpu, cyme};

struct Options {
    std::string filename;
    std::string outputname;
    bool has_output = false;
    bool verbose = true;
    bool optimize = false;
    targetKind target = targetKind::cpu;

    void print() {
        std::cout << cyan("." + std::string(60, '-') + ".") << std::endl;
        std::cout << cyan("| file     ") << filename << std::string(61-11-filename.size(),' ') << cyan("|") << std::endl;
        std::string outname = (outputname.size() ? outputname : "stdout");
        std::cout << cyan("| output   ") << outname << std::string(61-11-outname.size(),' ') << cyan("|") << std::endl;
        std::cout << cyan("| verbose  ") << (verbose  ? "yes" : "no ") << std::string(61-11-3,' ') << cyan("|") << std::endl;
        std::cout << cyan("| optimize ") << (optimize ? "yes" : "no ") << std::string(61-11-3,' ') << cyan("|") << std::endl;
        std::cout << cyan("| target   ") << (target==targetKind::cpu? "cpu" : target==targetKind::cyme ? "cym" : "gpu") << std::string(61-11-3,' ') << cyan("|") << std::endl;
        std::cout << cyan("." + std::string(60, '-') + ".") << std::endl;
    }
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
            fout_arg("o","output","name of output file", false,"","filname");
        // output filename
        TCLAP::ValueArg<std::string>
            target_arg("t","target","backend target={cpu,gpu,cyme}", true,"cpu","cpu/gpu/cyme");
        // verbose mode
        TCLAP::SwitchArg verbose_arg("V","verbose","toggle verbose mode", cmd, false);
        // optimization mode
        TCLAP::SwitchArg opt_arg("O","optimize","turn optimizations on", cmd, false);

        cmd.add(fin_arg);
        cmd.add(fout_arg);
        cmd.add(target_arg);

        cmd.parse(argc, argv);

        options.outputname = fout_arg.getValue();
        options.has_output = options.outputname.size()>0;
        options.filename = fin_arg.getValue();
        options.verbose = verbose_arg.getValue();
        options.optimize = opt_arg.getValue();
        auto targstr = target_arg.getValue();
        if(targstr == "cpu") {
            options.target = targetKind::cpu;
        }
        else if(targstr == "gpu") {
            options.target = targetKind::gpu;
        }
        else if(targstr == "cyme") {
            options.target = targetKind::cyme;
        }
        else {
            std::cerr << red("error") << " target must be one in {cpu, gpu, cyme}" << std::endl;
            return 1;
        }
    }
    // catch any exceptions in command line handling
    catch(TCLAP::ArgException &e) {
        std::cerr << "error: " << e.error()
                  << " for arg " << e.argId()
                  << std::endl;
    }

    try {
        // load the module from file passed as first argument
        Module m(options.filename.c_str());

        // check that the module is not empty
        if(m.buffer().size()==0) {
            std::cout << red("error: ") << white(argv[1])
                      << " invalid or empty file" << std::endl;
            return 1;
        }

        if(options.verbose) {
            options.print();
        }

        ////////////////////////////////////////////////////////////
        // parsing
        ////////////////////////////////////////////////////////////
        if(options.verbose) std::cout << green("[") + "parsing" + green("]") << std::endl;

        // initialize the parser
        Parser p(m, false);

        // parse
        p.parse();
        if(p.status() == lexerStatus::error) return 1;

        ////////////////////////////////////////////////////////////
        // semantic analysis
        ////////////////////////////////////////////////////////////
        if(options.verbose) std::cout << green("[") + "semantic analysis" + green("]") << std::endl;
        m.semantic();

        if( m.has_error() || m.has_warning() ) {
            std::cout << m.error_string() << std::endl;
        }
        if(m.status() == lexerStatus::error) {
            return 1;
        }

        for(auto &symbol : m.symbols()) {
            if(symbol.second->kind()==symbolKind::procedure)
                std::cout << symbol.second->to_string() << std::endl;
        }

        ////////////////////////////////////////////////////////////
        // optimize
        ////////////////////////////////////////////////////////////
        if(options.optimize) {
            if(options.verbose) std::cout << green("[") + "optimize" + green("]") << std::endl;
            m.optimize();
            if(m.status() == lexerStatus::error) {
                return 1;
            }
        }

        ////////////////////////////////////////////////////////////
        // generate output
        ////////////////////////////////////////////////////////////
        if(options.verbose) {
            std::cout << green("[") + "code generation"
                      << green("]") << std::endl;
        }

        std::string text;
        switch(options.target) {
            case targetKind::cpu  :
                text = CPrinter(m, options.optimize).text();
                break;
                /*
            case targetKind::cyme :
                text = CymePrinter(m, options.optimize).text();
                break;
            case targetKind::gpu  :
                text = CUDAPrinter(m, options.optimize).text();
                break;
                */
            default :
                std::cerr << red("error") << ": gpu and cyme printing has been turned off" << std::endl;
                exit(1);
        }

        if(options.has_output) {
            std::ofstream fout(options.outputname);
            fout << text;
            fout.close();
        }
        else {
            std::cout << cyan("--------------------------------------") << std::endl;
            std::cout << text;
            std::cout << cyan("--------------------------------------") << std::endl;
        }

        std::cout << yellow("successfully compiled ") << white(options.filename) << " -> " << white(options.outputname) << std::endl;
    }

    catch(compiler_exception e) {
        std::cerr << red("internal compiler error: ")
                  << white("this means a bug in the compiler,"
                           " please report to modcc developers")
                  << std::endl
                  << e.what() << " @ " << e.location() << std::endl;
        exit(1);
    }
    catch(std::exception e) {
        std::cerr << red("internal compiler error: ")
                  << white("this means a bug in the compiler,"
                           " please report to modcc developers")
                  << std::endl
                  << e.what() << std::endl;
        exit(1);
    }
    catch(...) {
        std::cerr << red("internal compiler error: ")
                  << white("this means a bug in the compiler,"
                           " please report to modcc developers")
                  << std::endl;
        exit(1);
    }

    return 0;
}
