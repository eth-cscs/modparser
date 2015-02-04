#pragma once

#include <string>

#include <vector/include/Vector.h>
#include "indexedview.h"

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

