#pragma once

#include <limits>

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

        auto const alignment = 64; // alignment in bytes (TODO: get this info from allocator)
        auto padding = memory::impl::get_padding<value_type>(alignment, n);
        auto n_alloc = n + padding;
        auto total_size = 4 * n_alloc;

        data_= vector_type(total_size);       // allocate total storage
        data_(memory::all) = std::numeric_limits<value_type>::quiet_NaN(); // set to NaN

        // point sub-vectors into storage
        a_   = data_(        0,   n);
        d_   = data_(  n_alloc,   n_alloc + n);
        b_   = data_(2*n_alloc, 2*n_alloc + n);
        rhs_ = data_(3*n_alloc, 3*n_alloc + n);
    }

    size_type size() const {
        return parent_indices_.size();
    }

    /// solve the linear system, solution is in rhs[] on return
    void solve() {
        auto n = size();
        auto const& p = parent_indices_;

        ////////////////////////////////
        // backward sweep
        ////////////////////////////////
        for(auto i=n-1; i>=num_cells_; --i) {
            auto factor = a_[i] / d_[i];
            d_[p[i]]   -= factor * b_[i];
            rhs_[p[i]] -= factor * rhs_[i];
        }

        for(auto i=0; i<num_cells_; ++i) {
            rhs_[i] /= d_[i];
        }

        ////////////////////////////////
        // forward sweep
        ////////////////////////////////
        for(auto i=num_cells_; i<n; ++i) {
            rhs_[i] -= b_[i] * rhs_[p[i]];
            rhs_[i] /= d_[i];
        }
    }

    // mutiply by a vector
    vector_type gemv(view_type x) {
        auto n = size();
        auto const& p = parent_indices_;
        #ifndef NDEBUG
        assert(n == x.size());
        #endif

        vector_type y(n);
        if(size()==0) {
            return y;
        }
        y[0] = d_[0] * x[0];
        if(size()==1) {
            return y;
        }

        for(int i=1; i<n; ++i) {
            y[i] = a_[i]*x[p[i]] + d_[i]*x[i];  // y_i  = a_i*x_k + d_i*x_i
            y[p[i]] += b_[i]*x[i];              // y_k += b_i*x_i
        }

        return y;
    }

    view_type const& vec_a() const { return a_; }
    view_type&       vec_a()       { return a_; }

    view_type const& vec_b() const { return b_; }
    view_type&       vec_b()       { return b_; }

    view_type const& vec_d() const { return d_; }
    view_type&       vec_d()       { return d_; }

    view_type const& vec_rhs() const { return rhs_; }
    view_type&       vec_rhs()       { return rhs_; }

    view_type const& vec_v() const { return v_; }
    view_type&       vec_v()       { return v_; }

    view_type const& data() const { return data_; }
    view_type&       data()       { return data_; }

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

