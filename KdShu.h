#pragma once

#include "mechanism.h"

class Mechanism_KdShu : public Mechanism {
public :
    Mechanism_KdShu( IndexVector const& node_indices
                     Matrix &matrix)
    : matrix_(matrix),
      node_indices_(node_indices)
    {
    }

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
    IndexVector const& node_indices() const {
        return node_indices_;
    }
    IndexVector & node_indices() {
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
    DataVector  data_;
    DataRef     v_;
    DataRef     g_;
    DataRef     i_;
    DataRef     m_;
    DataRef     h_;

    // a reference to the matrix system for that this mechanism contributes to
    Matrix     &matrix_;

    // a reference to the node indices that this mechanism is present at
    IndexVector const& node_indices_;
};
