#include <clpoly/clpoly.hh>
#include "clpoly_test.hh"

using namespace clpoly;

int main()
{
    CLPOLY_TEST("public header: rational sampling");
    CLPOLY_ASSERT_EQ(sample_rational(QQ(1, 3), QQ(1, 2)), QQ(2, 5));
    CLPOLY_ASSERT_EQ(sample_rational(std::nullopt, QQ(0)), QQ(-1));
    return clpoly_test::test_summary();
}
