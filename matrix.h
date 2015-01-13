#pragma once

#include "util.h"
#include "lib/vector/include/Vector.h"

/*
   let's define the matrix storage class here, for starters
   don't bother templating much
    - hard-code template parameters for double precision host compute
*/

// the system matrix contains the following
//      a   : matrix sub diagonal
//      b   : matrix super-diagonal
//      d   : matrix diagonal
//      rhs : rhs
//      v   : solution (the voltage)
// the structure is described by
//      parent_indices
//      size        : number of nodes
//      num_cells   : number of cells in the cell group
class Matrix {
public:
    using value_type  = double;
    using size_type   = int;
    using vector_type = memory::HostVector<value_type>;
    using view_type   = memory::HostView<value_type>;
    using index_type  = memory::HostVector<size_type>;

    // constructor
    Matrix (index_type const& indices, size_type num_cells)
    :   parent_indices_(indices),
        num_cells_(num_cells)
    {
        // allocate memory for internal storage
        auto n = size(); // the number of grid points

        // Determine the memory required to store a single vector
        // where a vector is used to store a matrix diagonal or solution.
        // This will be size()+padding, where padding is added to ensure
        // that the start of each vector is properly alligned in data[].
        //auto alignment = vector_type::::alignment; // allignment used by the storage class
        auto alignment = 64; // allignment used by the storage class
        auto padding = n%alignment;
        auto n_alloc = n + padding;
        auto total_size = 5 * n_alloc;
        std::cerr << yellow("allocating data_") << std::endl;
        data_= vector_type(total_size); // time for an rvalue reference constructor
        std::cerr << yellow("allocating a_") << std::endl;
        a_   = data_(        0,   n_alloc);
        std::cerr << yellow("done") << std::endl;
        d_   = data_(  n_alloc, 2*n_alloc);
        b_   = data_(2*n_alloc, 3*n_alloc);
        rhs_ = data_(3*n_alloc, 4*n_alloc);
        v_   = data_(4*n_alloc, 5*n_alloc);
        std::cerr << yellow("done") << std::endl;
    }

    size_type size() const {
        return parent_indices_.size();
    }

    /*
    /// solve the linear system, solution is in rhs[] on return
    void solve() {
        auto n = size();
        auto p = parent_indices;

        ////////////////////////////////
        // backward sweep
        ////////////////////////////////
        for(auto i=n-1; i>=num_cells; --i) {
            auto factor = a[i] / d[i];
            d[p[i]]   -= factor * b[i];
            rhs[p[i]] -= factor * rhs[i];
        }

        for(auto i=0; i<num_cells; ++i) {
            rhs[i] /= d[i];
        }

        ////////////////////////////////
        // forward sweep
        ////////////////////////////////
        for(auto i=num_cells; i<n; ++i) {
            rhs[i] -= b[i] * rhs[p[i]];
            rhs[i] /= d[i];
        }
    }
    */

    view_type const& vec_a() const {
        return a_;
    }

    view_type const& vec_b() const {
        return b_;
    }

    view_type const& vec_d() const {
        return d_;
    }

    view_type const& vec_v() const {
        return v_;
    }

    view_type const& vec_rhs() const {
        return rhs_;
    }

private:
    // the parent indices are stored as an index array
    index_type parent_indices_;

    size_type num_cells_;

    // all the data fields point into a single block of storage
    vector_type data_;

    // the individual data files that point into block storage
    view_type   a_;
    view_type   d_;
    view_type   b_;
    view_type   rhs_;
    view_type   v_;
};

