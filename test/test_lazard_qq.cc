#include <clpoly/lazard_qq.hh>
#include "clpoly_test.hh"
using namespace clpoly;
using P = polynomial_ZZ;
int main()
{
    variable x("x"), y("y"), z("z");
    const regular_chain::order_type order(custom_var_order({z, y, x}));
    auto roots = isolate_real_points(regular_chain({x}, std::vector<P>{x * x - 2}, &order));
    auto a = roots.back();
    CLPOLY_TEST("QQ Taylor representatives and valuation");
    // 在 x=sqrt(2) 上，(x²-2)^n(y+1) 的首次非零阶数为 n，系数为 (2x)^n(y+1)。
    for (int n = 1; n <= 5; ++n)
    {
        auto result = lazard_evaluate_qq(P(pow(x * x - 2, n) * (y + 1)), {x, y}, a);
        CLPOLY_ASSERT(result.valuation == std::vector<size_t>{size_t(n)});
        polynomial_QQ actual, expected;
        poly_convert(result.polynomial, actual);
        poly_convert(P(pow(2 * x, n) * (y + 1)), expected);
        auto prefix = a;
        CLPOLY_ASSERT(prefix_zero(actual - expected, prefix, {y}));
    }
    CLPOLY_TEST("coefficient grouping before exact point-zero");
    // x²*y 与 -2*y 必须先合并为同一活动单项式 y 的系数，才能正确判为零。
    auto prefix = a;
    CLPOLY_ASSERT(prefix_zero(P(x * x * y - 2 * y), prefix, {y}));
    CLPOLY_ASSERT(!prefix_zero(P(x * y - y), prefix, {y}));
    auto origin = append_rational(append_rational(real_algebraic_point{}, x, QQ(0), &order), y, QQ(0), &order);
    auto result = lazard_evaluate_qq(P(x * z - y), {x, y, z}, origin);
    CLPOLY_ASSERT(result.valuation == std::vector<size_t>({0, 1}));
    polynomial_QQ f;
    poly_convert(result.polynomial, f);
    CLPOLY_ASSERT(f == polynomial_QQ(QQ(-1)));
    CLPOLY_TEST("Lazard representative is interpreted with the caller's point");
    // 同一个 QQ 代表 2x 在两个共轭点上符号相反，结果不能脱离原点上下文解释。
    auto representative = lazard_evaluate_qq(P(x * x - 2), {x, y}, a);
    auto positive = a, negative = roots.front();
    CLPOLY_ASSERT(representative.valuation == std::vector<size_t>{1});
    CLPOLY_ASSERT_EQ(point_sign(representative.polynomial, positive), 1);
    CLPOLY_ASSERT_EQ(point_sign(representative.polynomial, negative), -1);
    CLPOLY_TEST("validation in release as well as debug");
    // 零多项式没有有限的首次非零阶数；未声明变量不属于本次活动／冻结坐标。
    bool rejected = false;
    try
    {
        lazard_evaluate_qq(P(0), {x, y}, a);
    }
    catch (const std::domain_error &)
    {
        rejected = true;
    }
    CLPOLY_ASSERT(rejected);
    rejected = false;
    try
    {
        lazard_evaluate_qq(P(z), {x, y}, a);
    }
    catch (const std::invalid_argument &)
    {
        rejected = true;
    }
    CLPOLY_ASSERT(rejected);
    CLPOLY_TEST("general input order and direct lexicographic coefficient groups");
    const grlex_<custom_var_order> graded(custom_var_order({x, z, y}));
    polynomial_<QQ, grlex_<custom_var_order>> general(&graded);
    poly_convert(P((x*x-2)*y*y + (x*x-2)*z), general);
    auto checked_prefix = a;
    CLPOLY_ASSERT(prefix_zero(general, checked_prefix, {y,z}));
    auto lex_order = std::make_shared<const regular_chain::order_type>(custom_var_order({z,y,x}));
    regular_chain::polynomial_type lex(lex_order.get());
    poly_convert(general, lex);
    CLPOLY_ASSERT(__prefix_zero(lex, checked_prefix, {y,z}));
    lazard_qq_result direct(std::make_shared<const regular_chain::order_type>(custom_var_order({y,x})));
    poly_convert(P((x*x-2)*(y+1)), direct.polynomial);
    __lazard_evaluate_qq(direct, {x,y}, a);
    CLPOLY_ASSERT(direct.valuation == std::vector<size_t>{1});
    CLPOLY_ASSERT(prefix_zero(direct.polynomial -
                             lazard_evaluate_qq(P((x*x-2)*(y+1)), {x,y}, a).polynomial,
                             checked_prefix, {y}));
    bool invalid_active = false;
    try
    {
        prefix_zero(P(x), checked_prefix, {x});
    }
    catch (const std::invalid_argument &)
    {
        invalid_active = true;
    }
    CLPOLY_ASSERT(invalid_active);
    bool invalid_result = false;
    try
    {
        lazard_qq_result invalid(nullptr);
    }
    catch (const std::invalid_argument &)
    {
        invalid_result = true;
    }
    CLPOLY_ASSERT(invalid_result);
    return clpoly_test::test_summary();
}
