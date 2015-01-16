#include <limits>

#include "gtest.h"
#include "util.h"

#include "matrix.h"
#include "mechanism.h"
#include "KdShu.h"

#define VERBOSE_TEST

template<typename T>
void printvec(T const& v, std::string name="") {
    #ifdef VERBOSE_TEST
    if(name.size()>0)
        std::cout << yellow(name + " : ");
    for(auto val: v)
        std::cout << val << " ";
    std::cout << std::endl;
    #endif
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

// test that solution of a tri-diagonal system works
TEST(Matrix, solve) {
    using memory::Range;
    using memory::end;
    using memory::all;
    using index_type = Matrix::index_type;

    // create an index vector for a tridiagonal matrix
    index_type p(1023);
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

TEST(Mechanism, simple) {
    using memory::Range;
    using memory::end;
    using memory::all;
    using index_type = Matrix::index_type;

    // create an index vector for a tridiagonal matrix
    // this corresponds to a single cable with no splits
    index_type p(12);
    for(auto i : p.range()(1,end))
        p[i] = i-1;
    p[0] = 0;

    Matrix matrix(p, 1);
    Mechanism_KdShu mech(p, matrix);

    mech.get_v()(all) = 1.0;
    mech.init();

    auto state_names = mech.information.state_names();
    for(auto i=0; i<mech.information.num_state; ++i) {
        printvec(mech.get_state(i), state_names[i]);
    }

    printvec(mech.get_rhs_contribution(), "rhs");
    mech.current();
    printvec(mech.get_rhs_contribution(), "rhs");
}

