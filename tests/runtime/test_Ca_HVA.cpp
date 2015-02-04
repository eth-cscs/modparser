#include <gtest.h>

#include "ionchannel.h"
#include "runtime.h"

#include "mechanisms/Ca_HVA.h"

TEST(Mechanisms, Ca_HVA) {
    // load index data from file
    auto na_index = index_from_file("./nodefiles/na_ion.nodes");
    auto ca_index = index_from_file("./nodefiles/ca_ion.nodes");
    auto  k_index = index_from_file("./nodefiles/k_ion.nodes");

    auto parent_index = index_from_file("./nodefiles/parent_indices.txt");
    auto Ca_HVA_index  = index_from_file("./nodefiles/Ca_HVA.nodes");

    // calculate some cell statistics
    int num_cells = std::count(parent_index.begin(), parent_index.end(), 0);
    int num_nodes = parent_index.size();
    double nodes_per_cell = double(num_nodes)/double(num_cells);

    std::cout << "group size : " << num_nodes  << " nodes, "
              << num_cells << " cells" << std::endl
              << "cell  size : " << int(nodes_per_cell) << " nodes (mean)" << std::endl
              << "mech  size : " << Ca_HVA_index.size() << " instances"
              << std::endl;

    // initialize matrix
    Matrix matrix(parent_index, num_cells);
    // initialize ion channels
    IonChannel ion_ca(ca_index);

    // initialize the Nats2_t mechanism
    Mechanism_Ca_HVA mech(Ca_HVA_index, matrix);
    mech.set_params(0.0, 0.1);

    // configure the mapping from mechanism onto the sodium ion
    // channel by hand
    index_type ion_map = index_into(Ca_HVA_index, ca_index);
    print("ion map ", ion_map);
    mech.ion_ca.index = ion_map;
    mech.ion_ca.eca   = ion_ca.reversal_potential();
    mech.ion_ca.ica   = ion_ca.current();
    ion_ca.reversal_potential()(all) = 0.2;
    ion_ca.current()(all) = 0.;

    mech.gCa_HVAbar(all) = 2.;

    matrix.vec_a()(all) = -1.;
    matrix.vec_b()(all) = -1.;
    matrix.vec_d()(all) = 4.;
    matrix.vec_rhs()(all) = 0.;
    matrix.vec_v()(all) = 128.;

    // initialize the mechanism
    mech.nrn_init();
    std::cout << yellow("init") << std::endl;
    print("m   ", mech.m);
    print("h   ", mech.h);
    print("minf", mech.mInf);
    print("hinf", mech.hInf);
    print("mtau", mech.mTau);
    print("htau", mech.hTau);
    print("malpha", mech.mAlpha);
    print("mbeta", mech.mBeta);

    // update state
    mech.nrn_state();
    std::cout << yellow("state") << std::endl;
    print("m   ", mech.m);
    print("h   ", mech.h);
    print("minf", mech.mInf);
    print("hinf", mech.hInf);
    print("mtau", mech.mTau);
    print("htau", mech.hTau);
    print("malpha", mech.mAlpha);
    print("mbeta", mech.mBeta);

    // calculate current contribution
    mech.nrn_current();
    std::cout << yellow("current") << std::endl;
    print("gCa ", mech.gCa);
    print("ica ", mech.ica);
    print("rhs ", matrix.vec_rhs());
    print("d   ", matrix.vec_d());


    // add contribution back to diagonal
    mech.nrn_jacob();
    std::cout << yellow("jacob") << std::endl;
    print("g_  ", mech.g_);
    print("d   ", matrix.vec_d());

    matrix.solve();
    std::cout << yellow("solve") << std::endl;
    print("rhs ", matrix.vec_rhs());

    // print the result
    //print("ica ", ion_ca.current());
}


