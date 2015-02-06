#pragma once

#include <matrix.h>
#include <indexedview.h>

class Mechanism {
public:

    using value_type  = double;
    using size_type   = int;
    using vector_type = memory::HostVector<value_type>;
    using view_type   = memory::HostView<value_type>;
    using index_type  = memory::HostVector<size_type>;
    using indexed_view= IndexedView<value_type, size_type>;

    Mechanism(Matrix &matrix, index_type const& node_indices)
    :   matrix_(matrix), node_indices_(node_indices)
    {}

    size_type size() const {
        return node_indices_.size();
    }

    virtual void set_params(value_type t_, value_type dt_) = 0;
    virtual std::string name() const = 0;
    virtual void nrn_init()     = 0;
    virtual void nrn_state()    = 0;
    virtual void nrn_current()  = 0;
    virtual void nrn_jacob()    = 0;

    Matrix &matrix_;
    index_type const& node_indices_;
};

