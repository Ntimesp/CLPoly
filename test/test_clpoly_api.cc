#include <clpoly/clpoly.hh>
#include "clpoly_test.hh"

using namespace clpoly;

int main()
{
    CLPOLY_TEST("public header: rational sampling");
    CLPOLY_ASSERT_EQ(sample_rational(QQ(1, 3), QQ(1, 2)), QQ(2, 5));
    CLPOLY_ASSERT_EQ(sample_rational(std::nullopt, QQ(0)), QQ(-1));
    CLPOLY_TEST("public header: regular chains");
    variable x("x"), y("y");
    const regular_chain::order_type order(custom_var_order({y, x}));
    regular_chain t({x, y}, std::vector<polynomial_ZZ>{x*x - 2, y*y - x}, &order);
    CLPOLY_ASSERT(t.is_squarefree() && t.size() == 2);
    CLPOLY_ASSERT_EQ(t.prefix(1).variables(), std::vector<variable>{x});
    CLPOLY_TEST("public header: real algebraic points");
    auto points = isolate_real_points(t);
    CLPOLY_ASSERT_EQ(points.size(), size_t(2));
    CLPOLY_ASSERT(point_zero(polynomial_ZZ(y*y-x), points.front()));
    return clpoly_test::test_summary();
}
