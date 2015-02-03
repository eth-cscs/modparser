#pragma once

#include <fstream>
#include <iostream>
#include <limits>

#include "lib/vector/include/Vector.h"

/*******************************************************************************

  ion channels have the following fields :

    ---------------------------------------------------
    label   Ca      Na      K   name
    ---------------------------------------------------
    iX      ica     ina     ik  current
    eX      eca     ena     ek  reversal_potential
    Xi      cai     nai     ki  internal_concentration
    Xo      cao     nao     ko  external_concentration
    gX      gca     gna     gk  conductance
    ---------------------------------------------------
*******************************************************************************/

class IonChannel {
public :
    using value_type  = double;
    using size_type   = int;
    using vector_type = memory::HostVector<value_type>;
    using view_type   = vector_type::view_type;
    using index_type  = memory::HostVector<size_type>;
    using index_view  = index_type::view_type;
    using indexed_view= IndexedView<value_type, size_type>;

    IonChannel(index_view idx) {
        auto n = idx.size();
        node_indices_ = idx;

        data_ = vector_type(5*n);
        data_(memory::all) = std::numeric_limits<value_type>::quiet_NaN();

        iX_ = data_(0*n, 1*n);
        eX_ = data_(1*n, 2*n);
        gX_ = data_(2*n, 3*n);
        Xi_ = data_(3*n, 4*n);
        Xo_ = data_(4*n, 5*n);
    }

    view_type current() {
        return iX_;
    }
    view_type reversal_potential() {
        return eX_;
    }
    view_type internal_concentration() {
        return Xi_;
    }
    view_type external_concentration() {
        return Xo_;
    }
    view_type conductance() {
        return gX_;
    }

    index_type node_indices() {
        return node_indices_;
    }

    size_t size() const {
        return node_indices_.size();
    }

private :

    index_type node_indices_;
    vector_type data_;
    view_type iX_;
    view_type eX_;
    view_type gX_;
    view_type Xi_;
    view_type Xo_;
};
