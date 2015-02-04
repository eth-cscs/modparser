#include "runtime.h"

#include <cstdio>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>

// load an index from file
index_type index_from_file(std::string fname) {
    std::ifstream fid(fname);
    if(!fid.is_open()) {
        std::cerr << "errror: unable to open index file "
                  << fname
                  << std::endl;
        return index_type();
    }

    size_type n;
    fid >> n;
    printf( "%-6d indexes in %s\n", n, fname.c_str());

    index_type index(n);
    for(size_type i=0; i<n; ++i) {
        fid >> index[i];
    }

    return index;
}

// calculates the mapping of mechanism nodes onto ion nodes
index_type index_into(index_view mech, index_view ion) {
    using size_type = index_view::size_type;
    std::map<size_type, size_type> ion_map;
    for(size_type i=0; i<ion.size(); ++i) {
        ion_map[ion[i]] = i;
    }
    index_type map(mech.size());
    for(size_type i=0; i<mech.size(); ++i) {
        map[i] = ion_map[mech[i]];
    }
    return map;
}

