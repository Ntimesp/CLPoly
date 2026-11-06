#include <clpoly/real_algebraic.hh>
#include "clpoly_test.hh"
using namespace clpoly;
using P = polynomial_ZZ;
int main()
{
    variable x("x"), y("y"), z("z");
    const regular_chain::order_type order(custom_var_order({z, y, x}));
    CLPOLY_TEST("UNI adapter: zero root, strict brackets and checked domain");
    // 三组平方自由一元式都含一个简单零根，检查适配器补零、括区间和导数证书。
    for (const auto &f : std::vector<xz_detail::U>{
             {QQ(0), QQ(1)}, {QQ(0), QQ(-1), QQ(1)}, {QQ(0), QQ(-2), QQ(0), QQ(1)}})
    {
        auto roots = xz_detail::univariate(f, QQ(1, 64));
        CLPOLY_ASSERT_EQ(roots.size(), f.size() - 1);
        size_t zeros = 0;
        for (size_t j = 0; j < roots.size(); ++j)
        {
            const auto &b = roots[j];
            if (b.interior_contains(QQ(0)))
                ++zeros;
            CLPOLY_ASSERT(b.width() > QQ(0) && b.width() <= QQ(1, 64));
            CLPOLY_ASSERT(sgn(xz_detail::eval(f, b.left())) * sgn(xz_detail::eval(f, b.right())) ==
                          -1);
            CLPOLY_ASSERT(
                xz_detail::strict_sign(xz_detail::eval(xz_detail::derivative_u(f), b).sgn()));
            if (j)
                CLPOLY_ASSERT(roots[j - 1].right() < b.left());
        }
        CLPOLY_ASSERT_EQ(zeros, size_t(1));
    }
    bool uni_rejected = false;
    // 分别拒绝重根输入、非正精度和零多项式；非零常数应直接返回空表。
    try
    {
        xz_detail::univariate({QQ(1), QQ(-2), QQ(1)}, QQ(1, 16));
    }
    catch (const std::invalid_argument &)
    {
        uni_rejected = true;
    }
    CLPOLY_ASSERT(uni_rejected);
    uni_rejected = false;
    try
    {
        xz_detail::univariate({QQ(1)}, QQ(0));
    }
    catch (const std::invalid_argument &)
    {
        uni_rejected = true;
    }
    CLPOLY_ASSERT(uni_rejected);
    uni_rejected = false;
    try
    {
        xz_detail::univariate({}, QQ(1));
    }
    catch (const std::domain_error &)
    {
        uni_rejected = true;
    }
    CLPOLY_ASSERT(uni_rejected);
    CLPOLY_ASSERT(xz_detail::univariate({QQ(2)}, QQ(1, 16)).empty());
    CLPOLY_TEST("XZ06 isolation, zero and negative roots");
    // 根为 -sqrt(2)、0、sqrt(2)、3；精化必须嵌套，并保持所选根及其符号。
    regular_chain univariate({x}, std::vector<P>{x * (x * x - 2) * (x - 3)}, &order);
    auto points = isolate_real_points(univariate);
    CLPOLY_ASSERT_EQ(points.size(), size_t(4));
    const int signs[] = {-1, 0, 1, 1};
    for (size_t i = 0; i < points.size(); ++i)
    {
        auto &a = points[i];
        CLPOLY_ASSERT(point_zero(univariate.equations()[0], a));
        CLPOLY_ASSERT_EQ(point_sign(P(x), a), signs[i]);
        auto before = a.box()[0];
        a.refine(QQ(1, 10000));
        CLPOLY_ASSERT(before.contains(a.box()[0]));
        CLPOLY_ASSERT(a.box()[0].width() < QQ(1, 10000));
    }
    CLPOLY_TEST("nested triangular point and NSHR");
    // y²=x 排除负的 x 前缀，两个实点都满足 x=sqrt(2)。
    auto nested = isolate_real_points(regular_chain({x, y}, std::vector<P>{x * x - 2, y * y - x}, &order));
    CLPOLY_ASSERT_EQ(nested.size(), size_t(2));
    for (auto &a : nested)
    {
        CLPOLY_ASSERT_EQ(point_sign(P(x), a), 1);
        CLPOLY_ASSERT(point_zero(P(y * y - x), a));
        auto before = a.box();
        a.refine(QQ(1, 1000));
        for (size_t i = 0; i < 2; ++i)
            CLPOLY_ASSERT(before[i].contains(a.box()[i]));
    }
    CLPOLY_TEST("conjugate membership cannot use norm roots");
    // 同一条可约定义链有四个实点，判零结果必须区分 y=x 与 y=-x。
    auto conjugates =
        isolate_real_points(regular_chain({x, y}, std::vector<P>{x * x - 2, y * y - 2}, &order));
    CLPOLY_ASSERT_EQ(conjugates.size(), size_t(4));
    for (auto &a : conjugates)
    {
        int sx = point_sign(P(x), a), sy = point_sign(P(y), a);
        CLPOLY_ASSERT(point_zero(P(y - x), a) == (sx == sy));
        CLPOLY_ASSERT(point_zero(P(y + x), a) == (sx != sy));
    }
    CLPOLY_TEST("point-zero on reducible chain");
    // 不能从某个共轭点为零推断所有共轭点为零；另覆盖无实根和空点。
    auto rational = isolate_real_points(regular_chain({x}, std::vector<P>{(x - 1) * (x - 2)}, &order));
    CLPOLY_ASSERT(point_zero(P(x - 1), rational[0]));
    CLPOLY_ASSERT(!point_zero(P(x - 1), rational[1]));
    auto no_real = isolate_real_points(regular_chain({x}, std::vector<P>{x * x + 1}, &order));
    CLPOLY_ASSERT(no_real.empty());
    auto empty = isolate_real_points(regular_chain());
    CLPOLY_ASSERT(empty.size() == 1 && empty[0].size() == 0);
    CLPOLY_ASSERT_EQ(point_sign(P(-2), empty[0]), -1);
    CLPOLY_TEST("ROOTS-QQ degree branch, repeated roots and mixed coordinates");
    // 求指定前缀上的纤维根，包含系数消失、重根去重及有理坐标追加。
    auto fibre = roots_over(P((x - 2) * y), rational[0], y, &order);
    CLPOLY_ASSERT(fibre.size() == 1 && point_zero(P(y), fibre[0]));
    auto repeated = roots_over(P(pow(y * y - x, 2)), nested[0].prefix(1), y, &order);
    CLPOLY_ASSERT(repeated.size() == 2);
    for (auto &a : repeated)
        CLPOLY_ASSERT(point_zero(P(y * y - x), a));
    auto sector = append_rational(nested[0].prefix(1), y, QQ(2, 3), &order);
    CLPOLY_ASSERT(point_zero(P(3 * y - 2), sector));
    CLPOLY_ASSERT_EQ(point_sign(P(x), sector), 1);
    bool rejected = false;
    try
    {
        roots_over(P((x - 1) * y), rational[0], y, &order);
    }
    catch (const std::domain_error &)
    {
        rejected = true;
    }
    CLPOLY_ASSERT(rejected);
    auto constants = roots_over(P(2), rational[0], y, &order);
    CLPOLY_ASSERT(constants.empty());
    CLPOLY_TEST("degree drop and empty fibre on selected component");
    // 在 x=1 分支上二次项消失，实际变为 y-2；非零常数特化没有根。
    auto dropped = roots_over(P((x - 1) * y * y + y - 2), rational[0], y, &order);
    CLPOLY_ASSERT_EQ(dropped.size(), size_t(1));
    CLPOLY_ASSERT(point_zero(P(y - 2), dropped[0]));
    CLPOLY_ASSERT(roots_over(P((x - 1) * y + 1), rational[0], y, &order).empty());
    CLPOLY_TEST("deterministic generated rational and nested fibres");
    // 使用已知有理根的生成族，逐点检查纤维而不依赖隔离器自己的输出作答案。
    for (int i = 1; i <= 8; ++i)
    {
        const int lo = -i, hi = i + 1;
        auto generated =
            isolate_real_points(regular_chain({x}, std::vector<P>{-(x - lo) * (2 * x - hi)}, &order));
        CLPOLY_ASSERT_EQ(generated.size(), size_t(2));
        CLPOLY_ASSERT(point_zero(P(x - lo), generated[0]));
        CLPOLY_ASSERT(point_zero(P(2 * x - hi), generated[1]));
        for (auto &a : generated)
        {
            auto roots = roots_over(P((y - x) * (3 * y - 3 * x - 1)), a, y, &order);
            CLPOLY_ASSERT_EQ(roots.size(), size_t(2));
            CLPOLY_ASSERT(point_zero(P(y - x), roots[0]));
            CLPOLY_ASSERT(point_zero(P(3 * y - 3 * x - 1), roots[1]));
        }
    }
    CLPOLY_TEST("reencode on another defining chain");
    // 新定义可含重根或其他根，但重编码必须仍选回原来的 x=1。
    auto recoded = reencode_point(rational[0], regular_chain({x}, std::vector<P>{pow(x - 1, 2)}, &order));
    CLPOLY_ASSERT(same_real_point(recoded, rational[0]));
    CLPOLY_TEST("reencode retains the point when the new chain has other roots");
    auto different =
        reencode_point(rational[0], regular_chain({x}, std::vector<P>{(x - 1) * (x - 3)}, &order));
    CLPOLY_ASSERT(point_zero(P(x - 1), different));
    CLPOLY_ASSERT(same_real_point(different, rational[0]));

    CLPOLY_TEST("recursive reencoding selects the same two-dimensional point");
    // 新链包含额外的前缀根，递归重编码应保持指定二维实点。
    auto recoded_pair = reencode_point(
        conjugates.front(),
        regular_chain({x, y}, std::vector<P>{(x * x - 2) * (x - 3), y * y - x * x}, &order));
    CLPOLY_ASSERT(same_real_point(recoded_pair, conjugates.front()));
    CLPOLY_ASSERT(point_zero(P(y - x), recoded_pair));

    CLPOLY_TEST("squarefree branches select only the requested prefix");
    // y²=x 在 x=0 处有一个互异根，在 x=1 处有两个；分解后不能串用前缀。
    auto base_points = isolate_real_points(regular_chain({x}, std::vector<P>{x * (x - 1)}, &order));
    auto zero_fibre = roots_over(P(y * y - x), base_points.front(), y, &order);
    CLPOLY_ASSERT_EQ(zero_fibre.size(), size_t(1));
    CLPOLY_ASSERT(point_zero(P(y), zero_fibre[0]));
    auto unit_fibre = roots_over(P(y * y - x), base_points.back(), y, &order);
    CLPOLY_ASSERT_EQ(unit_fibre.size(), size_t(2));
    CLPOLY_ASSERT(point_zero(P(y + 1), unit_fibre[0]));
    CLPOLY_ASSERT(point_zero(P(y - 1), unit_fibre[1]));

    CLPOLY_TEST("external construction still checks the full certificate");
    // 宽盒 [-2,2] 含两个根，不能作为 x²-2 的单根证书直接构造实点。
    bool invalid_box = false;
    try
    {
        real_algebraic_point invalid(regular_chain({x}, std::vector<P>{x * x - 2}, &order),
                                     {rational_interval(QQ(-2), QQ(2))});
    }
    catch (const std::invalid_argument &)
    {
        invalid_box = true;
    }
    CLPOLY_ASSERT(invalid_box);

    CLPOLY_TEST("third level and close irrational roots");
    // 三级提升及距离为 1/4096 的两个代数纤维根，检查完整性和精确去重。
    auto third = roots_over(P(z * z - y), nested.back(), z, &order);
    CLPOLY_ASSERT_EQ(third.size(), size_t(2));
    for (auto &a : third)
        CLPOLY_ASSERT(point_zero(P(z * z - y), a));
    auto close = roots_over(P((y - x) * (4096 * y - 4096 * x - 1)), nested[0].prefix(1), y, &order);
    CLPOLY_ASSERT_EQ(close.size(), size_t(2));
    CLPOLY_ASSERT(point_zero(P(y - x), close[0]));
    CLPOLY_ASSERT(point_zero(P(4096 * y - 4096 * x - 1), close[1]));

    CLPOLY_TEST("checked general orders and direct QQ lexicographic queries");
    const grlex_<custom_var_order> graded(custom_var_order({x, y, z}));
    polynomial_<QQ, grlex_<custom_var_order>> general(&graded);
    poly_convert(P(y*y-x), general);
    general = general * QQ(-1, 7);
    auto selected = nested.back();
    CLPOLY_ASSERT(point_zero(general, selected));
    const auto lex_zero = selected.chain().convert(general);
    CLPOLY_ASSERT(__point_zero(lex_zero, selected));
    const auto lex_sign = selected.chain().convert(P(x));
    CLPOLY_ASSERT_EQ(__point_sign(lex_sign, selected), point_sign(P(x), selected));
    regular_chain::polynomial_type lex_fibre(&order);
    poly_convert(P(z*z-y), lex_fibre);
    const auto direct = __roots_over(lex_fibre, nested.back(), z);
    CLPOLY_ASSERT_EQ(direct.size(), third.size());
    for (size_t i = 0; i < direct.size(); ++i)
        CLPOLY_ASSERT(same_real_point(direct[i], third[i]));

    CLPOLY_TEST("public queries reject missing coordinates and incompatible target order");
    for (int query = 0; query < 3; ++query)
    {
        bool invalid = false;
        try
        {
            if (query == 0)
                point_zero(P(z), selected);
            else if (query == 1)
                point_sign(P(z), selected);
            else
                roots_over(P(z*z-y), selected, z, nullptr);
        }
        catch (const std::invalid_argument &)
        {
            invalid = true;
        }
        CLPOLY_ASSERT(invalid);
    }
    const regular_chain::order_type incompatible(custom_var_order({x,y,z}));
    bool bad_order = false;
    try
    {
        roots_over(P(z*z-y), selected, z, &incompatible);
    }
    catch (const std::invalid_argument &)
    {
        bad_order = true;
    }
    CLPOLY_ASSERT(bad_order);
    return clpoly_test::test_summary();
}
