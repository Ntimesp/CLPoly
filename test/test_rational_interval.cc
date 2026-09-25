#include <clpoly/rational_interval.hh>
#include "clpoly_test.hh"

using namespace clpoly;
using I = rational_interval;

// Check the exception type without hiding the operation being tested.
template<class Exception, class Function>
bool throws(Function operation)
{
    try
    {
        operation();
    }
    catch (const Exception&)
    {
        return true;
    }
    catch (...)
    {
        return false;
    }
    return false;
}

int main()
{
    CLPOLY_TEST("interval construction and endpoints");
    {
        CLPOLY_ASSERT_EQ(I(), I(QQ(0)));
        CLPOLY_ASSERT(I(QQ(2)).is_point());
        const I x(QQ(1, 2), QQ(3, 2));
        CLPOLY_ASSERT_EQ(x.left(), QQ(1, 2));
        CLPOLY_ASSERT_EQ(x.right(), QQ(3, 2));
        CLPOLY_ASSERT(!x.is_point());
        CLPOLY_ASSERT(throws<std::invalid_argument>([] { I x(QQ(2), QQ(1)); }));
    }

    CLPOLY_TEST("interval addition and subtraction");
    {
        const I x(QQ(1), QQ(3));
        const I y(QQ(-2), QQ(4));
        CLPOLY_ASSERT_EQ(x + y, I(QQ(-1), QQ(7)));
        CLPOLY_ASSERT_EQ(x - y, I(QQ(-3), QQ(5)));
        CLPOLY_ASSERT_EQ(-x, I(QQ(-3), QQ(-1)));
        CLPOLY_ASSERT_EQ(x + QQ(2), I(QQ(3), QQ(5)));
        CLPOLY_ASSERT_EQ(QQ(-2) * x, I(QQ(-6), QQ(-2)));
        CLPOLY_ASSERT_EQ(I(QQ(1, 3), QQ(2, 3)) + I(QQ(1, 6)), I(QQ(1, 2), QQ(5, 6)));
    }

    CLPOLY_TEST("multiplication: nine sign combinations");
    {
        const I positive(QQ(2), QQ(3));
        const I negative(QQ(-5), QQ(-4));
        const I mixed(QQ(-2), QQ(7));

        CLPOLY_ASSERT_EQ(positive * positive, I(QQ(4), QQ(9)));
        CLPOLY_ASSERT_EQ(positive * negative, I(QQ(-15), QQ(-8)));
        CLPOLY_ASSERT_EQ(positive * mixed, I(QQ(-6), QQ(21)));
        CLPOLY_ASSERT_EQ(negative * positive, I(QQ(-15), QQ(-8)));
        CLPOLY_ASSERT_EQ(negative * negative, I(QQ(16), QQ(25)));
        CLPOLY_ASSERT_EQ(negative * mixed, I(QQ(-35), QQ(10)));
        CLPOLY_ASSERT_EQ(mixed * positive, I(QQ(-6), QQ(21)));
        CLPOLY_ASSERT_EQ(mixed * negative, I(QQ(-35), QQ(10)));
        CLPOLY_ASSERT_EQ(mixed * mixed, I(QQ(-14), QQ(49)));

        // Asymmetric mixed intervals exercise both choices of extreme corner.
        CLPOLY_ASSERT_EQ(I(QQ(-7), QQ(2)) * I(QQ(-3), QQ(5)), I(QQ(-35), QQ(21)));
        CLPOLY_ASSERT_EQ(mixed * I(QQ(-5), QQ(3)), I(QQ(-35), QQ(21)));
    }

    CLPOLY_TEST("multiplication: zero endpoints and self assignment");
    {
        const I mixed(QQ(-2), QQ(7));
        CLPOLY_ASSERT_EQ(I(QQ(0)) * mixed, I(QQ(0)));
        CLPOLY_ASSERT_EQ(mixed * I(QQ(0)), I(QQ(0)));
        CLPOLY_ASSERT_EQ(I(QQ(0), QQ(3)) * mixed, I(QQ(-6), QQ(21)));
        CLPOLY_ASSERT_EQ(I(QQ(-3), QQ(0)) * mixed, I(QQ(-21), QQ(6)));
        CLPOLY_ASSERT_EQ(mixed * I(QQ(0), QQ(3)), I(QQ(-6), QQ(21)));
        CLPOLY_ASSERT_EQ(mixed * I(QQ(-3), QQ(0)), I(QQ(-21), QQ(6)));
        I self(QQ(-2), QQ(7));
        self *= self;
        CLPOLY_ASSERT_EQ(self, I(QQ(-14), QQ(49)));
    }

    CLPOLY_TEST("interval reciprocal and division");
    {
        CLPOLY_ASSERT_EQ(reciprocal(I(QQ(2), QQ(4))), I(QQ(1, 4), QQ(1, 2)));
        CLPOLY_ASSERT_EQ(reciprocal(I(QQ(-4), QQ(-2))), I(QQ(-1, 2), QQ(-1, 4)));
        CLPOLY_ASSERT_EQ(I(QQ(2), QQ(6)) / I(QQ(2), QQ(4)), I(QQ(1, 2), QQ(3)));
        CLPOLY_ASSERT_EQ(I(QQ(2), QQ(6)) / I(QQ(-4), QQ(-2)), I(QQ(-3), QQ(-1, 2)));
        // A zero anywhere in the divisor, including either endpoint, is rejected.
        CLPOLY_ASSERT(throws<std::domain_error>([] { reciprocal(I(QQ(-1), QQ(1))); }));
        CLPOLY_ASSERT(throws<std::domain_error>([] { reciprocal(I(QQ(0), QQ(1))); }));
        CLPOLY_ASSERT(throws<std::domain_error>([] { reciprocal(I(QQ(-1), QQ(0))); }));
        CLPOLY_ASSERT(throws<std::domain_error>([] { reciprocal(I(QQ(0))); }));
    }

    CLPOLY_TEST("interval compound assignment");
    {
        I x(QQ(1), QQ(2));
        x += I(QQ(3), QQ(4));
        CLPOLY_ASSERT_EQ(x, I(QQ(4), QQ(6)));
        x -= I(QQ(1), QQ(2));
        CLPOLY_ASSERT_EQ(x, I(QQ(2), QQ(5)));

        I self(QQ(1), QQ(2));
        self += self;
        CLPOLY_ASSERT_EQ(self, I(QQ(2), QQ(4)));
        self = I(QQ(1), QQ(2));
        self -= self;
        CLPOLY_ASSERT_EQ(self, I(QQ(-1), QQ(1)));
        self = I(QQ(1), QQ(2));
        self /= self;
        CLPOLY_ASSERT_EQ(self, I(QQ(1, 2), QQ(2)));

        I unchanged(QQ(2), QQ(3));
        CLPOLY_ASSERT(throws<std::domain_error>([&] { unchanged /= I(QQ(0), QQ(1)); }));
        CLPOLY_ASSERT_EQ(unchanged, I(QQ(2), QQ(3)));
    }

    CLPOLY_TEST("interval powers");
    {
        CLPOLY_ASSERT(throws<std::domain_error>([] { pow(I(QQ(1)), -1); }));
        CLPOLY_ASSERT_EQ(pow(I(QQ(0)), 0), I(QQ(1)));
        CLPOLY_ASSERT_EQ(pow(I(QQ(-2), QQ(7)), 1), I(QQ(-2), QQ(7)));
        CLPOLY_ASSERT_EQ(pow(I(QQ(-2)), 3), I(QQ(-8)));
        CLPOLY_ASSERT_EQ(pow(I(QQ(-2)), 4), I(QQ(16)));
        CLPOLY_ASSERT_EQ(pow(I(QQ(0)), 3), I(QQ(0)));
        CLPOLY_ASSERT_EQ(pow(I(QQ(2, 3)), 3), I(QQ(8, 27)));
        CLPOLY_ASSERT_EQ(pow(I(QQ(-2), QQ(3)), 3), I(QQ(-8), QQ(27)));
        CLPOLY_ASSERT_EQ(pow(I(QQ(2), QQ(3)), 2), I(QQ(4), QQ(9)));
        CLPOLY_ASSERT_EQ(pow(I(QQ(-3), QQ(-2)), 2), I(QQ(4), QQ(9)));
        CLPOLY_ASSERT_EQ(pow(I(QQ(-3), QQ(2)), 2), I(QQ(0), QQ(9)));
        CLPOLY_ASSERT_EQ(pow(I(QQ(-2), QQ(3)), 2), I(QQ(0), QQ(9)));
    }

    CLPOLY_TEST("interval containment");
    {
        const I x(QQ(1), QQ(3));
        CLPOLY_ASSERT(x.contains(QQ(1)));
        CLPOLY_ASSERT(!x.contains(QQ(0)));
        CLPOLY_ASSERT(!x.contains(QQ(4)));
        CLPOLY_ASSERT(x.contains(I(QQ(1), QQ(2))));
        CLPOLY_ASSERT(!x.contains(I(QQ(2), QQ(4))));
        CLPOLY_ASSERT(x.interior_contains(QQ(2)));
        CLPOLY_ASSERT(!x.interior_contains(QQ(1)));
        CLPOLY_ASSERT(x.interior_contains(I(QQ(3, 2), QQ(5, 2))));
        CLPOLY_ASSERT(!x.interior_contains(x));
    }

    CLPOLY_TEST("interval intersection and hull");
    {
        const I x(QQ(0), QQ(1));
        const I touching(QQ(1), QQ(2));
        CLPOLY_ASSERT(x.overlaps(touching));
        CLPOLY_ASSERT(!x.overlaps(I(QQ(2), QQ(3))));
        const auto point = intersection(x, touching);
        CLPOLY_ASSERT(point && *point == I(QQ(1)));
        const auto overlap = intersection(I(QQ(0), QQ(3)), I(QQ(1), QQ(4)));
        CLPOLY_ASSERT(overlap && *overlap == I(QQ(1), QQ(3)));
        CLPOLY_ASSERT(!intersection(x, I(QQ(2), QQ(3))));
        CLPOLY_ASSERT_EQ(hull(x, I(QQ(2), QQ(3))), I(QQ(0), QQ(3)));
        CLPOLY_ASSERT(x != touching);
    }

    CLPOLY_TEST("interval midpoint and bisection");
    {
        const I x(QQ(1), QQ(4));
        CLPOLY_ASSERT_EQ(x.width(), QQ(3));
        CLPOLY_ASSERT_EQ(x.midpoint(), QQ(5, 2));
        const auto halves = x.bisect();
        CLPOLY_ASSERT_EQ(halves.first, I(QQ(1), QQ(5, 2)));
        CLPOLY_ASSERT_EQ(halves.second, I(QQ(5, 2), QQ(4)));
        const auto point = I(QQ(2)).bisect();
        CLPOLY_ASSERT_EQ(point.first, I(QQ(2)));
        CLPOLY_ASSERT_EQ(point.second, I(QQ(2)));
    }

    CLPOLY_TEST("interval sign");
    {
        CLPOLY_ASSERT_EQ(I(QQ(2), QQ(3)).sgn(), 1);
        CLPOLY_ASSERT_EQ(I(QQ(-3), QQ(-2)).sgn(), -1);
        CLPOLY_ASSERT_EQ(I(QQ(0)).sgn(), 0);
        CLPOLY_ASSERT_EQ(I(QQ(-1), QQ(1)).sgn(), 2);
        CLPOLY_ASSERT_EQ(I(QQ(0), QQ(3)).sgn(), 2);
        CLPOLY_ASSERT_EQ(I(QQ(-3), QQ(0)).sgn(), 2);
        CLPOLY_ASSERT(!I(QQ(2), QQ(3)).contains_zero());
        CLPOLY_ASSERT(!I(QQ(-3), QQ(-2)).contains_zero());
        CLPOLY_ASSERT(I(QQ(-1), QQ(1)).contains_zero());
        CLPOLY_ASSERT(I(QQ(0), QQ(1)).contains_zero());
        CLPOLY_ASSERT(I(QQ(-1), QQ(0)).contains_zero());
        CLPOLY_ASSERT(I(QQ(0)).contains_zero());
    }

    CLPOLY_TEST("interval endpoints beyond int64");
    {
        const QQ large(ZZ("18446744073709551616")); // 2^64
        const I x(large, QQ(ZZ("18446744073709551617")));
        CLPOLY_ASSERT_EQ(x.width(), QQ(1));
        CLPOLY_ASSERT_EQ(x + I(QQ(1)),
                         I(QQ(ZZ("18446744073709551617")), QQ(ZZ("18446744073709551618"))));
        CLPOLY_ASSERT_EQ(x * I(QQ(2)),
                         I(QQ(ZZ("36893488147419103232")), QQ(ZZ("36893488147419103234"))));
    }

    return clpoly_test::test_summary();
}
