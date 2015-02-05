#pragma once

#include <algorithm>
#include <string>

#include <vector/include/Vector.h>
#include "indexedview.h"

#ifdef PROFILING_PAPI
#include <papi_wrap.h>
#define START_PROFILE  pw_start_collector(handle_);
#define STOP_PROFILE   pw_stop_collector(handle_);
#define INIT_PROFILE   if(handle_<0) handle_ = pw_new_collector(name().c_str());
#define DATA_PROFILE   int handle_=-1;
#else
#define INIT_PROFILE
#define START_PROFILE
#define STOP_PROFILE
#define DATA_PROFILE
#endif

using memory::all;
using memory::end;

using value_type  = double;
using size_type   = int;
using vector_type = memory::HostVector<value_type>;
using view_type   = vector_type::view_type;
using index_type  = memory::HostVector<size_type>;
using index_view  = index_type::view_type;
using indexed_view= IndexedView<value_type, size_type>;

// load an index from file
index_type index_from_file(std::string fname);

// calculates the mapping of mechanism nodes onto ion nodes
index_type index_into(index_view mech, index_view ion);

// print a vector
template <typename Container>
void print(std::string const& name, Container const& c) {
    auto range = std::minmax_element(c.begin(), c.end());
    auto min = *range.first;
    auto max = *range.second;
    std::cout << name << (name.size() ? " " : "");
    if(min==max) {
        std::cout << "[" << c.size() << " * " << min << "]" << std::endl;
    }
    else {
        std::cout << "[" << c.size() << " in "
                  << "[" << min << ":" << max << "]]" << std::endl;
    }
}
