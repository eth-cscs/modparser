#pragma once

#include "mechanism.h"
#include "matrix.h"

class Mechanism_KdShu : public Mechanism {
public :
    using value_type  = double;
    using size_type   = int;
    using vector_type = memory::HostVector<value_type>;
    using view_type   = memory::HostView<value_type>;
    using index_type  = memory::HostVector<size_type>;

    Mechanism_KdShu( index_type const& node_indices,
                     Matrix &matrix)
    :   Mechanism("KdShu2007"),
        matrix_(matrix),
        node_indices_(node_indices)
    {
    }
/*
    ////////////////////
    // helpers
    // could these, and the corresponding data be put in the base class?
    ////////////////////
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
    index_type & node_indices() {
        return node_indices_;
    }

    //INITIAL {
    //  trates(v)
    //  m=minf
    //  h=hinf
    //}
    //PROCEDURE trates(v) {
    //  LOCAL qt
    //  qt=q10^((celsius-22)/10)
    //  minf=1-1/(1+exp((v-vhalfm)/km))
    //  hinf=1/(1+exp((v-vhalfh)/kh))
    //  mtau = 0.6
    //  htau = 1500
    //}
    void init() override {
        auto n = size();
        for(int i=0; i<n; ++i) {
            // all the values used below are actually constant (parameters that aren't global)
            auto minf = 1. - 1./(1 + exp((v[i] - vhalfm)/km);
            auto hinf = 1./(1 + exp((v[i] - vhalfm)/km);
            m[i] = minf;
            h[i] = minf;
        }
    }
private :
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
    // the data fields
    vector_type data_;
    view_type   v_;
    view_type   g_;
    view_type   i_;
    view_type   m_;
    view_type   h_;
*/

    // a reference to the matrix system for that this mechanism contributes to
    Matrix     &matrix_;

    // a reference to the node indices that this mechanism is present at
    index_type const& node_indices_;
};
