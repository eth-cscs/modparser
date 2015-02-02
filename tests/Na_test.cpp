#include "NaTs2_t.h"
#include "../ionchannel.h"

#include <cstdio>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>

using value_type  = double;
using size_type   = int;
using vector_type = memory::HostVector<value_type>;
using view_type   = vector_type::view_type;
using index_type  = memory::HostVector<size_type>;
using index_view  = index_type::view_type;
using indexed_view= IndexedView<value_type, size_type>;

// load an index from file
index_type index_from_file(std::string fname) {
        std::ifstream fid(fname);
        if(!fid.is_open()) {
            std::cerr << "errror: unable to open index file "
                      << fname
                      << std::endl;
            return index_type();
        }

        size_type n;
        fid >> n;
        printf( "%-6d indexes in %s\n", n, fname.c_str());

        index_type index(n);
        for(size_type i=0; i<n; ++i) {
            fid >> index[i];
        }

        return index;
}

// calculates the mapping of mechanism nodes onto ion nodes
index_type index_into(index_view mech, index_view ion) {
    using size_type = index_view::size_type;
    std::map<size_type, size_type> ion_map;
    for(size_type i=0; i<ion.size(); ++i) {
        ion_map[ion[i]] = i;
    }
    index_type map(mech.size());
    for(size_type i=0; i<mech.size(); ++i) {
        map[i] = ion_map[mech[i]];
    }
    return map;
}

int main(void) {
    // load index data from file
    auto na_index = index_from_file("./nodefiles/na_ion.nodes");
    auto ca_index = index_from_file("./nodefiles/ca_ion.nodes");
    auto  k_index = index_from_file("./nodefiles/k_ion.nodes");

    auto parent_index = index_from_file("./nodefiles/parent_indices.txt");
    auto NaTs2_index  = index_from_file("./nodefiles/NaTs2_t.nodes");

    // calculate some cell statistics
    int num_cells = std::count(parent_index.begin(), parent_index.end(), 0);
    int num_nodes = parent_index.size();
    double nodes_per_cell = double(num_nodes)/double(num_cells);

    std::cout << "there are " << num_cells
              << " cells in the cell group, with an average "
              << nodes_per_cell << " nodes per cell"
              << std::endl;

    // initialize matrix
    Matrix matrix(parent_index, num_cells);
    // initialize ion channels
    IonChannel ion_na(na_index);
    IonChannel ion_ca(ca_index);
    IonChannel ion_k (k_index);

    // initialize the Nats2_t mechanism
    Mechanism_NaTs2_t mech(NaTs2_index, matrix);
    mech.set_params(0.0, 0.1);

    // configure the mapping from mechanism onto the sodium ion
    // channel by hand
    index_type ion_map = index_into(NaTs2_index, na_index);
    mech.ion_na.index = ion_map;
    mech.ion_na.ena   = ion_na.reversal_potential();
    mech.ion_na.ina   = ion_na.current();
    ion_na.reversal_potential()(memory::all) = 0.2;
    ion_na.current()(memory::all) = 0.;

    matrix.vec_a()(memory::all) = -1.;
    matrix.vec_b()(memory::all) = -1.;
    matrix.vec_d()(memory::all) = 4.;
    matrix.vec_rhs()(memory::all) = 0.;
    matrix.vec_v()(memory::all) = 128.;

    // initialize the mechanism
    mech.nrn_init();

    // calculate current contribution
    mech.nrn_current();

    // update state
    mech.nrn_state();

    // add contribution back to rhs
    mech.nrn_jacob();

    //matrix.solve();

    // print the result
    //for(auto val : matrix.vec_rhs())
    //for(auto val : ion_na.reversal_potential())
    for(auto val : ion_na.current())
        std::cout << val << " ";
    std::cout << std::endl;
}


