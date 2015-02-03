#pragma once

#include <vector/include/Vector.h>

template <typename T, typename I>
struct IndexedView {
    using value_type  = T;
    using size_type   = I;
    using vector_type = memory::HostVector<value_type>;
    using view_type   = memory::HostView<value_type>;
    using index_type  = memory::HostVector<size_type>;

    view_type  & view;
    index_type const& index;

    IndexedView(view_type& v, index_type const& i)
    :   view(v),
        index(i)
    {}

    size_type size() const {
        return index.size();
    }

    value_type&
    operator[] (const size_type i) {
        return view[index[i]];
    }

    value_type const&
    operator[] (const size_type i) const {
        return view[index[i]];
    }
};

