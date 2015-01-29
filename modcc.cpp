#include <chrono>
#include <iostream>
#include <fstream>

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

    if(m.status() == k_compiler_error) return 1;

    //#define WITH_PROFILING
    #ifdef WITH_PROFILING
    {
        const int N = 10;
        using clock = std::chrono::high_resolution_clock;
        using duration = std::chrono::duration<double>;

        auto start = clock::now();
        for(int i=0; i<N; ++i) {
            Module m(filename.c_str());
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
    auto outputname = basename + ".h";
    std::cout << "output file name " << white(outputname) << std::endl;

    std::ofstream fout(outputname);

    //////////////////////////////////////////////
    //////////////////////////////////////////////
    fout << "#pragma once\n\n";
    fout << "#include <cmath>\n\n";
    //fout << "#include \"../mechanism.h\"\n";
    fout << "#include \"../matrix.h\"\n";
    fout << "#include \"../indexedview.h\"\n\n";

    //////////////////////////////////////////////
    //////////////////////////////////////////////

    fout << "class Mechanism {\n";
    fout << "public:\n\n";
    fout << "    using value_type  = double;\n";
    fout << "    using size_type   = int;\n";
    fout << "    using vector_type = memory::HostVector<value_type>;\n";
    fout << "    using view_type   = memory::HostView<value_type>;\n";
    fout << "    using index_type  = memory::HostVector<size_type>;\n";
    fout << "    using indexed_view= IndexedView<value_type, size_type>;\n\n";

    //////////////////////////////////////////////
    //////////////////////////////////////////////
    for(auto& ion: m.neuron_block().ions) {
        auto tname = "Ion" + ion.name;
        fout << "    struct " + tname + " {\n";
        for(auto& field : ion.read) {
            fout << "        view_type " + field + ";\n";
        }
        for(auto& field : ion.write) {
            fout << "        view_type " + field + ";\n";
        }
        fout << "        index_type index;\n";
        fout << "    };\n";
        fout << "    " + tname + " ion_" + ion.name + ";\n\n";
    }

    //////////////////////////////////////////////
    //////////////////////////////////////////////
    fout << "    Mechanism( index_type const& node_indices,\n";
    fout << "               Matrix &matrix)\n";
    fout << "    :  matrix_(matrix), node_indices_(node_indices)\n";
    fout << "    {}\n\n";

    //////////////////////////////////////////////
    //////////////////////////////////////////////

    auto v = new CPrinter();
    v->increase_indentation();
    auto proctest = [] (procedureKind k) {return k == k_proc_normal || k == k_proc_api;};
    for(auto const &var : m.symbols()) {
        if(   var.second.kind==k_symbol_procedure
           && proctest(var.second.expression->is_procedure()->kind()))
        {
            var.second.expression->accept(v);
        }
    }
    fout << v->text();

    //////////////////////////////////////////////
    //////////////////////////////////////////////

    fout << "private:\n\n";
    fout << "    vector_type data_;" << std::endl;
    for(auto sym: m.symbols()) {
        if(sym.second.kind==k_symbol_variable) {
            auto var = sym.second.expression->is_variable();
            if(var->is_range()) {
                fout << "    view_type " << var->name() << ";\n";
            }
            else {
                fout << "    double " << var->name() << ";\n";
            }
        }
        else if(sym.second.kind==k_symbol_indexed_variable) {
            auto var = sym.second.expression->is_indexed_variable();
            fout << "    view_type " << var->name() << ";\n";
        }
    }

    fout << "    Matrix &matrix_;\n";
    fout << "    index_type const& node_indices_;\n";

    fout << "};\n\n";

    //////////////////////////////////////////////
    //////////////////////////////////////////////

    fout.close();

    return 0;
}
