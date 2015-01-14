#include <limits>

#include "gtest.h"
#include "util.h"

#include "matrix.h"
#include "mechanism.h"
#include "KdShu.h"

template<typename T>
void printvec(T const& v) {
    for(auto val: v)
        std::cout << val << " ";
    std::cout << std::endl;
}

TEST(Matrix, init) {
    using memory::Range;
    using memory::end;
    using memory::all;
    using index_type = Matrix::index_type;

    // create an index vector for a tridiagonal matrix
    index_type p(32);
    for(auto i : p.range()(1,end)) {
        p[i] = i-1;
    }
    p[0] = 0;

    Matrix A(p, 1);

    EXPECT_EQ(A.size(), p.size());
    EXPECT_EQ(A.vec_d().size(), p.size());
    EXPECT_EQ(A.vec_a().size(), p.size());
    EXPECT_EQ(A.vec_b().size(), p.size());
    EXPECT_EQ(A.vec_rhs().size(), p.size());
}

TEST(Matrix, solve) {
    using memory::Range;
    using memory::end;
    using memory::all;
    using index_type = Matrix::index_type;

    // create an index vector for a tridiagonal matrix
    index_type p(32);
    for(auto i : p.range()(1,end)) {
        p[i] = i-1;
    }
    p[0] = 0;

    Matrix A(p, 1);

    A.vec_a()(all) = 1.;
    A.vec_b()(all) = 1.;
    A.vec_d()(all) = -4;
    A.vec_rhs()(all) = 1;


    A.solve();

    A.vec_a()(all) = 1.;
    A.vec_b()(all) = 1.;
    A.vec_d()(all) = -4;

    Matrix::vector_type v = A.gemv(A.vec_rhs());
    for(auto val : v)
        ASSERT_NEAR(val, 1, 1e-15);
}

/*
TEST(Mechanism, simple) {
    Mechanism_KdShu();
}
*/
