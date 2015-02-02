#include "NaTs2_t.h"
#include "../ionchannel.h"

#include <fstream>
#include <iostream>

using value_type  = double;
using size_type   = int;
using vector_type = memory::HostVector<value_type>;
using view_type   = memory::HostView<value_type>;
using index_type  = memory::HostVector<size_type>;
using indexed_view= IndexedView<value_type, size_type>;

index_type index_from_file() {
        std::ifstream fid(fname);
        if(!fid.is_open()) {
            std::cerr << "errror: unable to open index file "
                      << fname
                      << std::endl;
            return index_type();
        }

        size_type n;
        fid >> n;
        std::cout << "index file " << fname << " has size " << n << std::endl;

        index_type index(n);
        for(size_type i=0; i<n; ++i) {
            fid >> index[i];
        }
        for(auto idx : node_indices_) {
            std::cout << idx << " ";
        }
        std::cout << std::endl << std::endl;
        return index;
}

int main(void) {
    auto na_index = index_from_file("./nodefiles/na_ion.nodes");
    std::cout << (na_index.size() ? "success" : "fail") << std::endl;
}

