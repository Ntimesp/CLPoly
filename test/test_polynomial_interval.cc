#include <clpoly/polynomial_interval.hh>
#include "clpoly_test.hh"

using namespace clpoly;
using I = rational_interval;
using P = polynomial_ZZ;

template<class Function>
bool rejects(Function operation)
{
    try
    {
        operation();
    }
    catch (const std::invalid_argument&)
    {
        return true;
    }
    return false;
}

int main()
{
    variable x("x"), y("y"), z("z");
    const lex_<custom_var_order> xy({x, y, z});
    using L = polynomial_<ZZ, lex_<custom_var_order>>;

    CLPOLY_TEST("interval assignment: zero, constants and rational coefficients");
    {
        CLPOLY_ASSERT_EQ(assign_interval(P(0), {}, {}), I(QQ(0)));
        CLPOLY_ASSERT_EQ(assign_interval(P(-3), {}, {}), I(QQ(-3)));
        CLPOLY_ASSERT_EQ(assign_interval(P(2), {x}, {I(QQ(-1), QQ(1))}), I(QQ(2)));
        polynomial_QQ f = QQ(1, 2) * x + QQ(1, 3);
        CLPOLY_ASSERT_EQ(assign_interval(f, {x}, {I(QQ(1), QQ(2))}),
                         I(QQ(5, 6), QQ(4, 3)));
    }

    CLPOLY_TEST("interval assignment: sparse powers and crossing zero");
    {
        L f(&xy);
        poly_convert(P(x*x + y), f);
        CLPOLY_ASSERT_EQ(__assign_interval(f, {x, y}, {I(QQ(-1), QQ(1)), I(QQ(2), QQ(3))}),
                         I(QQ(2), QQ(4)));
        // 一般入口会改变变量优先级；这个多项式在两种顺序下得到相同包络。
        CLPOLY_ASSERT_EQ(assign_interval(f, {y, x}, {I(QQ(2), QQ(3)), I(QQ(-1), QQ(1))}),
                         I(QQ(2), QQ(4)));
        poly_convert(P(pow(x, 6) + 2*pow(x, 2)), f);
        CLPOLY_ASSERT_EQ(__assign_interval(f, {x}, {I(QQ(-1), QQ(1))}), I(QQ(0), QQ(3)));
        poly_convert(P(pow(x, 1000000) + 1), f);
        CLPOLY_ASSERT_EQ(__assign_interval(f, {x}, {I(QQ(-1), QQ(1))}), I(QQ(1), QQ(2)));
    }

    CLPOLY_TEST("interval assignment: recursive coefficient groups");
    {
        L f(&xy);
        // (y+1)*x^3 + y^2*x + z：递归中的零次组不含 x 或 y，检查后缀偏移。
        poly_convert(P((y+1)*pow(x, 3) + y*y*x + z), f);
        CLPOLY_ASSERT_EQ(__assign_interval(f, {x, y, z},
                         {I(QQ(1), QQ(2)), I(QQ(2), QQ(3)), I(QQ(-1), QQ(1))}),
                         I(QQ(6), QQ(51)));
        CLPOLY_ASSERT_EQ(__assign_interval(f, {x, y, z},
                         {I(QQ(2)), I(QQ(3)), I(QQ(-1))}), I(QQ(49)));
        // 跳过未使用的坐标时，变量与盒区间的对应关系应保持不变。
        poly_convert(P(z*z), f);
        CLPOLY_ASSERT_EQ(__assign_interval(f, {x, y, z},
                         {I(QQ(10)), I(QQ(20)), I(QQ(-1), QQ(2))}), I(QQ(0), QQ(4)));
        poly_convert(P(x + y + z + 1), f);
        CLPOLY_ASSERT_EQ(__assign_interval(f, {x, y, z},
                         {I(QQ(1), QQ(2)), I(QQ(2), QQ(3)), I(QQ(3), QQ(4))}),
                         I(QQ(7), QQ(10)));
    }

    CLPOLY_TEST("interval assignment: variable priority and order conversion");
    {
        const P f = x*x*y + x*y*y;
        L lx(&xy);
        poly_convert(f, lx);
        const lex_<custom_var_order> yx({y, x});
        L ly(&yx);
        poly_convert(f, ly);
        const std::vector<variable> vars{x, y};
        const std::vector<I> box{I(QQ(-1), QQ(1)), I(QQ(1), QQ(2))};
        // 比较两种变量顺序对应的表达式：x*(y*x+y^2) 与 y*(x*y+x^2)。
        CLPOLY_ASSERT_EQ(__assign_interval(lx, vars, box), I(QQ(-6), QQ(6)));
        CLPOLY_ASSERT_EQ(__assign_interval(ly, {y, x}, {box[1], box[0]}), I(QQ(-4), QQ(6)));
        // 即使输入字典序的比较器不同，一般入口仍按 vars 指定的顺序计算。
        CLPOLY_ASSERT_EQ(assign_interval(ly, vars, box), I(QQ(-6), QQ(6)));
        const grlex_<custom_var_order> graded({y, x});
        polynomial_<ZZ, grlex_<custom_var_order>> g(&graded);
        poly_convert(f, g);
        CLPOLY_ASSERT_EQ(assign_interval(g, vars, box), I(QQ(-6), QQ(6)));
        polynomial_<ZZ, lex> default_lex;
        poly_convert(P(x*x + 1), default_lex);
        CLPOLY_ASSERT_EQ(__assign_interval(default_lex, {x}, {I(QQ(-1), QQ(1))}),
                         I(QQ(1), QQ(2)));
    }

    CLPOLY_TEST("interval assignment: Horner enclosure and shrinking boxes");
    {
        L f(&xy);
        poly_convert(P(x*x + x), f);
        CLPOLY_ASSERT_EQ(__assign_interval(f, {x}, {I(QQ(-1), QQ(1))}), I(QQ(-2), QQ(2)));
        poly_convert(P(x*x - 2*x + 1), f);
        CLPOLY_ASSERT_EQ(__assign_interval(f, {x}, {I(QQ(1), QQ(2))}), I(QQ(-1), QQ(1)));
        CLPOLY_ASSERT_EQ(__assign_interval(f, {x}, {I(QQ(1), QQ(3, 2))}), I(QQ(-1, 2), QQ(1, 2)));
        CLPOLY_ASSERT_EQ(__assign_interval(f, {x}, {I(QQ(1))}), I(QQ(0)));
    }

    CLPOLY_TEST("interval assignment: invalid bindings");
    {
        CLPOLY_ASSERT(rejects([&] { assign_interval(P(0), {x}, {}); }));
        CLPOLY_ASSERT(rejects([&] { assign_interval(P(0), {x, x}, {I(), I()}); }));
        CLPOLY_ASSERT(rejects([&] { assign_interval(P(x), {y}, {I()}); }));
        L f(&xy);
        poly_convert(P(x), f);
        CLPOLY_ASSERT(rejects([&] { assign_interval(f, {}, {}); }));

    }
    return clpoly_test::test_summary();
}
