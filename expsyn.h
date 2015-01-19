#pragma once

#include "mechanism.h"
#include "matrix.h"

/// Type that encapsulates runtime information about kdShu mechanism.
/// Used because this information can't be embedded directly in MechanismKdShu,
/// because non-aggregate classes can't have constexpr members
struct InfoExpsyn {
    // dimensions of mechanism
    static constexpr int num_fields = 9;
    static constexpr int num_state  = 2; // number of state variables

    static std::vector<std::string>
    state_names() {
        return {"m", "h"};
    }

    // constants
    static constexpr double vhalfm = -43;
    static constexpr double vhalfh = -67;
    static constexpr double km     = 8;
    static constexpr double kh     = 7.3;

    // scalar values
    double q10    = 2.3;
};

class Mechanism_Expsyn : public Mechanism {
public :
    using info = InfoExpsyn;
    InfoExpsyn information;

    using value_type  = double;
    using size_type   = int;
    using vector_type = memory::HostVector<value_type>;
    using view_type   = memory::HostView<value_type>;
    using index_type  = memory::HostVector<size_type>;

    Mechanism_Expsyn( index_type const& node_indices,
                     Matrix &matrix)
    :   Mechanism("expsyn"),
        matrix_(matrix),
        node_indices_(node_indices)
    {
        auto n = size();
        auto const alignment = 64; // alignment in bytes (TODO: get this info from allocator)
        auto padding = memory::impl::get_padding<value_type>(alignment, n);
        auto n_alloc = n + padding;
        auto total_size = info::num_fields * n_alloc;

        data_= vector_type(total_size);       // allocate total storage
        data_(memory::all) = std::numeric_limits<value_type>::quiet_NaN(); // set to NaN

        // point sub-vectors into storage
        v = data_(0*n_alloc, 0*n_alloc + n);
        g = data_(1*n_alloc, 1*n_alloc + n);
        i = data_(2*n_alloc, 2*n_alloc + n);
        rhs_contribution = data_(5*n_alloc, 5*n_alloc + n);

        // initialize initial values for parameters from PARAMETER block
        // these were declared as both RANGE + PARAMETER
        //      it stinks that ghey are not declared GLOBAL, and yet still need
        gkbar(memory::all) = 0.1;
        ek(memory::all) = -100;
    }

    ////////////////////////////////////////////////////////////////////////////
    // access to intrinsic properties
    ////////////////////////////////////////////////////////////////////////////
    view_type get_v() {
        return v;
    }
    view_type get_g() {
        return g;
    }
    view_type get_i() {
        return i;
    }
    view_type get_rhs_contribution() {
        return rhs_contribution;
    }
    view_type get_state(int i) {
        assert(i<info::num_state);
        switch(i) {
            case 0:
                return m;
            case 1:
                return h;
            default:
                return view_type();
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // helpers
    // could these, and the corresponding data be put in the base class?
    ////////////////////////////////////////////////////////////////////////////
    int size() const {
        return node_indices_.size();
    }

    Matrix const& matrix() const {
        return matrix_;
    }
    Matrix & matrix() {
        return matrix_;
    }
    index_type const& node_indices() const {
        return node_indices_;
    }

    //INITIAL {
    //  g=0
    //}
    void init() override {
        // TODO : load v
        auto n = size();
        for(int i=0; i<n; ++i) {
            g_state[i] = 0.;
        }
    }

    //BREAKPOINT {
    //    SOLVE state METHOD cnexp
    //    i = g*(v - e)
    //}
    void current() override {
        // low arithmetic intensity:
        //      5 loads
        //      2 stores
        //      6 flops
        auto n = size();
        // TODO : load v
        for(int i=0; i<n; ++i) {
            i_rng[i] = g_state[i] * (v[i] - e[i]);
            g[i]  = g_state[i];
            rhs_contribution[i] = i_rng[i];
        }
        // TODO : flush rhs
        // TODO : flush currents
    }

private :

    void flush_rhs() {
        auto n = size();
        auto ni = node_indices();
        auto vec_rhs = matrix_.vec_rhs();
        for(int i=0; i<n; ++i) {
            vec_rhs[ni[i]] += rhs_contribution[i];
        }
    }

    // ----- what to store? ---- //
    //  - state variables
    //      - number of state variables (2)
    //      - names of state variables (m,h)
    //      - DataRefs for state variables
    //  - local store
    //      - voltage       v
    //      - external current ghost: ik, ica, etc
    //      - nonspecific current: i
    //      - capacitance   g
    //      - scratch variables (e.g. minf... not that we store minf in this case)
    //      - parameters (mtau, vhalfm, km, etc)
    //  - externally referenced data
    //      * these could be wrapped in a single type, that is referenced from the cell group
    //      - a reference to the cellgroup matrix
    //      - the voltage vector
    //      - celcius
    //      - area
    //      - ionic current variables
    //  - index data
    //      - nodeindices

    ////////////////////////////////////
    // the data fields
    ////////////////////////////////////
    vector_type data_;

    // general (all mechanisms have these)
    view_type   v;
    view_type   g;
    view_type   i;
    view_type   m;
    view_type   h;
    view_type   rhs_contribution;    // for saving update to the RHS == -g=-di/dv

    // mechanism-specific (unique to this mechanism)
    view_type   ik;
    view_type   gkbar;
    view_type   ek;

    // a reference to the matrix system for that this mechanism contributes to
    Matrix     &matrix_;

    // a reference to the node indices that this mechanism is present at
    index_type const& node_indices_;
};


