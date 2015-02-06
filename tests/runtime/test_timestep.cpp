#include <gtest.h>

#include "ionchannel.h"
#include "runtime.h"

#include "mechanisms/Ca_HVA.h"
#include "mechanisms/NaTs2_t.h"
#include "mechanisms/Ih.h"

//#include <omp.h>

TEST(Mechanisms, timestep) {
//#pragma omp parallel
//{
    std::cout << "hello " << std::endl;
    // load index data from file
    auto na_index = index_from_file("./nodefiles/na_ion.nodes");
    auto ca_index = index_from_file("./nodefiles/ca_ion.nodes");
    auto  k_index = index_from_file("./nodefiles/k_ion.nodes");

    auto parent_index = index_from_file("./nodefiles/parent_indices.txt");
    auto Ca_HVA_index = index_from_file("./nodefiles/Ca_HVA.nodes");
    auto NaTs2_index  = index_from_file("./nodefiles/NaTs2_t.nodes");
    auto Ih_index     = index_from_file("./nodefiles/Ih.nodes");

    // calculate some cell statistics
    int num_cells = std::count(parent_index.begin(), parent_index.end(), 0);
    int num_nodes = parent_index.size();
    double nodes_per_cell = double(num_nodes)/double(num_cells);

    std::cout << "group size : " << num_nodes  << " nodes, "
              << num_cells << " cells" << std::endl
              << "cell  size : " << int(nodes_per_cell) << " nodes (mean)"
              << std::endl;

    ////////////////////////////////////////////////
    // initialize matrix
    ////////////////////////////////////////////////
    Matrix matrix(parent_index, num_cells);
    matrix.vec_a()(all) = -1.;
    matrix.vec_b()(all) = -1.;
    matrix.vec_d()(all) = 4.;
    matrix.vec_rhs()(all) = 0.;
    matrix.vec_v()(all) = 51.34;

    ////////////////////////////////////////////////
    // initialize ion channels
    ////////////////////////////////////////////////
    IonChannel ion_ca(ca_index);
    IonChannel ion_na(na_index);
    IonChannel ion_k(k_index);

    ion_ca.reversal_potential()(all) = 0.2;
    ion_ca.current()(all) = 0.;
    ion_na.reversal_potential()(all) = 0.2;
    ion_na.current()(all) = 0.;
    ion_k.reversal_potential()(all) = 0.2;
    ion_k.current()(all) = 0.;

    ////////////////////////////////////////////////
    // initialize the mechanisms
    ////////////////////////////////////////////////
    std::vector<Mechanism *> mechanisms;

    const double t0 = 0.0;
    const double dt = 0.1;

    /////////// Ca_HVA ///////////
    Mechanism_Ca_HVA mech_Ca_HVA(matrix, Ca_HVA_index);
    mech_Ca_HVA.set_params(t0, dt);
    mech_Ca_HVA.ion_ca.index = index_into(Ca_HVA_index, ca_index);
    mech_Ca_HVA.ion_ca.eca   = ion_ca.reversal_potential();
    mech_Ca_HVA.ion_ca.ica   = ion_ca.current();
    mechanisms.push_back(&mech_Ca_HVA);

    /////////// NaTs2_t ///////////
    Mechanism_NaTs2_t mech_NaTs2_t(matrix, NaTs2_index);
    mech_NaTs2_t.set_params(t0, dt);
    mech_NaTs2_t.ion_na.index = index_into(NaTs2_index, na_index);
    mech_NaTs2_t.ion_na.ena   = ion_na.reversal_potential();
    mech_NaTs2_t.ion_na.ina   = ion_na.current();
    mechanisms.push_back(&mech_NaTs2_t);

    /////////// Ih ///////////
    Mechanism_Ih mech_Ih(matrix, Ih_index);
    mech_Ih.set_params(t0, dt);
    // no ion channel dependencies
    mechanisms.push_back(&mech_Ih);

    // initialize the mechanism
    for(auto m : mechanisms) {
        m->nrn_init();
    }

    #ifdef PROFILING_PAPI
    int handle_ = pw_new_collector("matrix");
    #endif

    auto VEC_A   = matrix.vec_a();
    auto VEC_B   = matrix.vec_b();
    auto VEC_V   = matrix.vec_v();
    auto VEC_RHS = matrix.vec_rhs();
    auto VEC_D   = matrix.vec_d();
    auto p       = parent_index;

    const int nt = 1000;
    for(auto i=0; i<nt; ++i) {
        START_PROFILE;
        VEC_RHS(all) = 0.;
        VEC_D  (all) = 0.;
        STOP_PROFILE;
        ///////////////////////////
        //   setup_tree_matrix   //
        ///////////////////////////
        // rhs
        for(auto m : mechanisms) {
            m->nrn_current();
        }
        START_PROFILE;
        for(auto i=num_cells; i<num_nodes; ++i) {
            auto dv        = VEC_V[p[i]] - VEC_V[i];
            VEC_RHS[i]    -= dv * VEC_B[i];
            VEC_RHS[p[i]] += dv * VEC_A[i];
        }
        STOP_PROFILE;

        // lhs (add conductance to diagonal)
        for(auto m : mechanisms) {
            m->nrn_jacob();
        }
        START_PROFILE;
        for(auto i=num_cells; i<num_nodes; ++i) {
            VEC_D[i]    -= VEC_B[i];
            VEC_D[p[i]] -= VEC_A[i];
        }
        STOP_PROFILE;

        ///////////////////////////
        //   nrn_solve_minimal   //
        ///////////////////////////
        START_PROFILE;
        matrix.solve();

        // second_order_current (missing)

        ///////////////////////////
        //        update         //
        ///////////////////////////
        for(auto i=0; i<num_nodes; ++i) {
            VEC_V[i] += 2.0*VEC_RHS[i];
        }
        STOP_PROFILE;

        ///////////////////////////
        //        nonvint        //
        ///////////////////////////
        // update state
        for(auto m : mechanisms) {
            m->nrn_state();
        }
    }
//}// end omp parallel
}

