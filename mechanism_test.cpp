#include "gtest.h"
#include "util.h"

#include "mechanism.h"
#include "KdShu.h"

TEST(Matrix, init) {
    using memory::Range;
    using memory::end;
    using index_type = Matrix::index_type;

    // create an index vector for a tridiagonal matrix
    index_type p(10);
    for(auto i : p.range()(1,end)) {
        p[i] = i-1;
    }
    p[0] = 0;

    std::cout << red("construct Matrix") << std::endl;
    Matrix A(p, 1);
}
/*
TEST(Mechanism, simple) {
    Mechanism_KdShu();
}
*/

