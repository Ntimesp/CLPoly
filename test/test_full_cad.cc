#include "clpoly_test.hh"
#include <clpoly/full_cad.hh>
using namespace clpoly;
// 检查父子连续布局、栈的交替结构、来源身份及样本前缀，不重复实现投影算法。
static void check_tree(const full_cad_result &tree)
{
    for (size_t level = 0; level < tree.num_levels(); ++level)
    {
        size_t parents = level ? tree.level_size(level - 1) : 1;
        size_t next = 0;
        for (size_t parent = 0; parent < parents; ++parent)
        {
            const auto &stack = tree.stack(level, level ? parent : SIZE_MAX);
            size_t begin = level ? tree.node(level - 1, parent).child_begin() : 0;
            size_t count = level ? tree.node(level - 1, parent).child_count() : tree.level_size(0);
            CLPOLY_ASSERT_EQ(begin, next);
            // r 个不同根产生 r 个 Section 和 r+1 个 Sector。
            CLPOLY_ASSERT_EQ(count, 2 * stack.roots.size() + 1);
            for (const auto &source : stack.sources)
                CLPOLY_ASSERT(!source.empty());
            for (size_t i = 0; i < count; ++i)
            {
                const auto &node = tree.node(level, begin + i);
                const auto &sample = tree.sample(level, begin + i);
                CLPOLY_ASSERT_EQ(sample.size(), level + 1);
                CLPOLY_ASSERT_EQ(node.parent(), level ? parent : SIZE_MAX);
                CLPOLY_ASSERT_EQ(node.is_section(), bool(i % 2));
                size_t pd = level ? tree.dimension(level - 1, parent) : 0;
                CLPOLY_ASSERT_EQ(tree.dimension(level, begin + i), pd + (i % 2 ? 0 : 1));
                if (level)
                    CLPOLY_ASSERT(
                        same_real_point(sample.prefix(level), tree.sample(level - 1, parent)));
                auto selected = sample;
                const auto v = tree.level_var(level);
                if (i % 2)
                {
                    CLPOLY_ASSERT(same_real_point(selected, stack.roots[i / 2]));
                    const auto &resolved =
                        tree.root(level, level ? parent : SIZE_MAX, node.roots()[0]);
                    CLPOLY_ASSERT(&resolved == &stack.roots[i / 2]);
                }
                else
                {
                    // Sector 新增坐标等于 cad_node 中保存的有理样本。
                    const QQ q = tree.node(level, begin + i).sample_point();
                    polynomial_QQ linear;
                    poly_convert(polynomial_ZZ(v), linear);
                    linear = linear - q;
                    CLPOLY_ASSERT(point_zero(linear, selected));
                    if (i > 0)
                        CLPOLY_ASSERT(stack.roots[i / 2 - 1].box().back().right() < q);
                    if (i / 2 < stack.roots.size())
                        CLPOLY_ASSERT(q < stack.roots[i / 2].box().back().left());
                }
            }
            next += count;
        }
        CLPOLY_ASSERT_EQ(next, tree.level_size(level));
    }
}
int main()
{
    variable x("x"), y("y"), z("z");
    const regular_chain::order_type order(custom_var_order({z, y, x}));
    using P = polynomial_ZZ;
    CLPOLY_TEST("Lazard rational nullification and order");
    // 在 (x,y)=(0,0) 处，xz-y 的 valuation 为 (0,1)，最终代表为 -1。
    cad_sample a = append_rational(append_rational(cad_sample{}, x, QQ(0), &order), y, QQ(0), &order);
    auto l = lazard_evaluate(P(x * z - y), {x, y, z}, a);
    CLPOLY_ASSERT(l.valuation == std::vector<size_t>({0, 1}));
    polynomial_QQ lm;
    poly_convert(l.polynomial, lm);
    CLPOLY_ASSERT(lm == polynomial_QQ(QQ(-1)));
    auto sr = isolate_real_points(regular_chain({x}, std::vector<P>{x * x - 2}, &order));
    cad_sample b = sr.back();
    auto l2 = lazard_evaluate(P(pow(x * x - 2, 2)), {x, y}, b);
    CLPOLY_ASSERT(l2.valuation == std::vector<size_t>({2}));
    polynomial_QQ l2p;
    poly_convert(l2.polynomial, l2p);
    CLPOLY_ASSERT(point_zero(l2p - QQ(8), b));
    CLPOLY_TEST("one-dimensional complete CAD");
    // 两个实根分出五个胞腔；中间 Sector 为负，根处为零，其余 Sector 为正。
    auto c = full_cad(std::vector<P>{x * x - 2}, {x});
    CLPOLY_ASSERT_EQ(c.level_size(0), size_t(5));
    for (size_t i = 0; i < 5; ++i)
    {
        CLPOLY_ASSERT_EQ(c.node(0, i).is_section(), bool(i % 2));
        CLPOLY_ASSERT_EQ(c.signs(i)[0], i % 2 ? 0 : (i == 2 ? -1 : 1));
    }
    CLPOLY_TEST("root references keep their source polynomial identity");
    // 来源 x 与 x-1 各自只有第 0 个根，不能用合并根表下标代替来源根序号。
    auto boundaries = full_cad(std::vector<P>{P(x), P(x - 1)}, {x});
    // 两个根之间使用 1/2；无界 Sector 优先取整数。
    CLPOLY_ASSERT_EQ(boundaries.node(0, 0).sample_point(), QQ(-1));
    CLPOLY_ASSERT_EQ(boundaries.node(0, 2).sample_point(), QQ(1, 2));
    CLPOLY_ASSERT_EQ(boundaries.node(0, 4).sample_point(), QQ(2));
    auto left_ref = boundaries.node(0, 1).roots()[0];
    auto right_ref = boundaries.node(0, 3).roots()[0];
    CLPOLY_ASSERT_EQ(left_ref.root_idx(), size_t(0));
    CLPOLY_ASSERT_EQ(right_ref.root_idx(), size_t(0));
    CLPOLY_ASSERT(left_ref.poly_idx() != right_ref.poly_idx());
    auto right_point = boundaries.root(0, SIZE_MAX, right_ref);
    CLPOLY_ASSERT(point_zero(P(x - 1), right_point));

    CLPOLY_TEST("shared roots advance the ordinals of every source");
    // 两个来源在父点 x=0 共享 y=0，后续 y=1 在其来源中仍须编号为 1。
    auto coincident = full_cad(std::vector<P>{P(y - x), P(y * y - y - x)}, {y, x});
    bool shared_parent = false;
    for (size_t parent = 0; parent < coincident.level_size(0); ++parent)
    {
        auto point = coincident.sample(0, parent);
        if (!point_zero(P(x), point))
            continue;
        shared_parent = true;
        const auto &stack = coincident.stack(1, parent);
        CLPOLY_ASSERT_EQ(stack.roots.size(), size_t(2));
        CLPOLY_ASSERT_EQ(stack.sources[0].size(), size_t(2));
        const size_t source = stack.sources[1][0];
        const size_t section = coincident.node(0, parent).child_begin() + 3;
        const auto reference = coincident.node(1, section).roots()[0];
        CLPOLY_ASSERT_EQ(reference.poly_idx(), source);
        CLPOLY_ASSERT_EQ(reference.root_idx(), size_t(1));
        const auto &common = coincident.root(1, parent, cad_root(source, 0));
        CLPOLY_ASSERT(&common == &stack.roots[0]);
        auto one = coincident.root(1, parent, reference);
        CLPOLY_ASSERT(point_zero(P(y - 1), one));
    }
    CLPOLY_ASSERT(shared_parent);

    CLPOLY_TEST("coordinate axes");
    // 两坐标轴给出九个胞腔，维数分布为一个点、四条射线和四个二维区域。
    auto axes = full_cad(std::vector<P>{P(x), P(y)}, {y, x});
    CLPOLY_ASSERT_EQ(axes.level_size(0), size_t(3));
    CLPOLY_ASSERT_EQ(axes.leaf_count(), size_t(9));
    size_t dims[3] = {0, 0, 0};
    for (size_t i = 0; i < axes.leaf_count(); ++i)
        ++dims[axes.dimension(1, i)];
    CLPOLY_ASSERT_EQ(dims[0], size_t(1));
    CLPOLY_ASSERT_EQ(dims[1], size_t(4));
    CLPOLY_ASSERT_EQ(dims[2], size_t(4));
    CLPOLY_TEST("projection order x,z,y lifts coordinates y,z,x");
    // 非默认排列也必须反向提升，并使定义方程、根盒和层变量保持同一绑定。
    auto reordered = full_cad(std::vector<P>{P(x - 1), P(y - 2), P(z - 3)}, {x, z, y});
    std::vector<variable> level_variables;
    for (size_t level = 0; level < reordered.num_levels(); ++level)
        level_variables.push_back(reordered.level_var(level));
    CLPOLY_ASSERT(level_variables == std::vector<variable>({y, z, x}));
    // 每层只有一个根，沿三个 Section 到达点 (y,z,x)=(2,3,1)。
    size_t point_index = 1;
    for (size_t level = 1; level < reordered.num_levels(); ++level)
        point_index = reordered.node(level - 1, point_index).child_begin() + 1;
    const auto &ordered_point = reordered.leaf_sample(point_index);
    CLPOLY_ASSERT(ordered_point.chain().variables() == level_variables);
    CLPOLY_ASSERT(ordered_point.box()[0].interior_contains(QQ(2)) &&
                   ordered_point.box()[1].interior_contains(QQ(3)) &&
                   ordered_point.box()[2].interior_contains(QQ(1)));
    CLPOLY_ASSERT(reordered.signs(point_index) == std::vector<int>({0, 0, 0}));
    CLPOLY_TEST("circle lower-dimensional cells");
    // 圆的切点会改变提升栈的根数，必须保留这些低维父胞腔及其子栈。
    auto circle = full_cad(std::vector<P>{x * x + y * y - 1}, {y, x});
    CLPOLY_ASSERT_EQ(circle.level_size(0), size_t(5));
    CLPOLY_ASSERT_EQ(circle.leaf_count(), size_t(13));
    CLPOLY_TEST("empty, constants and zero-dimensional ambient space");
    // 只有零和常数时每层形成一个 Sector；零变量空间仍有一个空样本叶和常数符号行。
    auto empty = full_cad(std::vector<P>{P(0), P(-3)}, {y, x});
    CLPOLY_ASSERT_EQ(empty.leaf_count(), size_t(1));
    CLPOLY_ASSERT(empty.signs(0) == std::vector<int>({0, -1}));
    auto zero = full_cad(std::vector<P>{P(4)}, {});
    CLPOLY_ASSERT_EQ(zero.leaf_count(), size_t(1));
    CLPOLY_ASSERT_EQ(zero.signs(0)[0], 1);
    CLPOLY_ASSERT(zero.leaf_sample(0).size() == 0);
    CLPOLY_TEST("copy retains comparator ownership");
    // 原结果销毁后，副本中的投影多项式仍须持有有效比较器。
    auto copied = [&]()
    {
        auto temporary = full_cad(std::vector<P>{x * x - 2}, {x});
        auto copy = temporary;
        return copy;
    }();
    auto pp = copied.level_polys(0)[0] + copied.level_polys(0)[0];
    CLPOLY_ASSERT(!pp.empty());
    CLPOLY_TEST("three-variable nullification vertical line");
    // 在 x=y=0 的父胞腔上，xz-y 恒零；最后坐标应形成一个完整 Sector。
    auto nul = full_cad(std::vector<P>{x * z - y}, {z, y, x});
    bool found = false;
    for (size_t j = 0; j < nul.level_size(1); ++j)
    {
        auto p = nul.sample(1, j);
        if (point_zero(P(x), p) && point_zero(P(y), p))
        {
            found = true;
            CLPOLY_ASSERT_EQ(nul.node(1, j).child_count(), size_t(1));
            auto child = nul.node(1, j).child_begin();
            CLPOLY_ASSERT(nul.node(2, child).is_sector());
            CLPOLY_ASSERT_EQ(nul.signs(child)[0], 0);
        }
    }
    CLPOLY_ASSERT(found);
    CLPOLY_TEST("irrational section, rational sector, algebraic section");
    // 验证代数点、QQ 采样和再次代数提升可以交替，完整前缀证书保持一致。
    auto mix = full_cad(std::vector<P>{x * x - 2, P(y), z * z - x - y}, {z, y, x});
    found = false;
    for (size_t j = 0; j < mix.leaf_count(); ++j)
    {
        auto p1 = mix.node(2, j).parent();
        auto p0 = mix.node(1, p1).parent();
        if (mix.node(0, p0).is_section() && mix.node(1, p1).is_sector() &&
            mix.node(2, j).is_section())
        {
            auto p = mix.sample(2, j);
            if (point_zero(P(x * x - 2), p))
            {
                found = true;
                CLPOLY_ASSERT(point_zero(P(z * z - x - y), p));
                CLPOLY_ASSERT_EQ(mix.signs(j)[2], 0);
            }
        }
    }
    CLPOLY_ASSERT(found);
    CLPOLY_TEST("shared roots, rational coefficients and input validation");
    // 共享根只计一次；QQ 系数保留原值符号；非法变量表在公开入口报错。
    auto shared = full_cad(std::vector<P>{x * x - 2, (x * x - 2) * (x - 3)}, {x});
    CLPOLY_ASSERT_EQ(shared.leaf_count(), size_t(7));
    using QP = polynomial_QQ;
    QP q;
    poly_convert(P(2 * x - 1), q);
    q = q * QQ(1, 3);
    auto rational = full_cad(std::vector<QP>{q}, {x});
    CLPOLY_ASSERT_EQ(rational.leaf_count(), size_t(3));
    auto rational_sample = rational.sample(0, 1);
    CLPOLY_ASSERT(point_zero(P(2 * x - 1), rational_sample));
    bool threw = false;
    try
    {
        full_cad(std::vector<P>{P(x)}, {x, x});
    }
    catch (const std::invalid_argument &)
    {
        threw = true;
    }
    CLPOLY_ASSERT(threw);
    threw = false;
    try
    {
        full_cad(std::vector<P>{P(y)}, {x});
    }
    catch (const std::invalid_argument &)
    {
        threw = true;
    }
    CLPOLY_ASSERT(threw);
    CLPOLY_TEST("independent sign oracle for repeated rational grid factors");
    // 偶次幂第一因子非负，奇次幂第二因子保留符号，答案由已知根序直接确定。
    auto grid = full_cad(
        std::vector<P>{pow((x + 2) * (2 * x - 1), 2), pow((3 * y + 1) * (y - 2), 3)}, {y, x});
    CLPOLY_ASSERT_EQ(grid.leaf_count(), size_t(25));
    for (size_t j = 0; j < grid.leaf_count(); ++j)
    {
        const size_t ix = grid.node(1, j).parent(), iy = j % 5;
        const int sx = ix % 2 ? 0 : 1, sy = iy % 2 ? 0 : (iy == 2 ? -1 : 1);
        CLPOLY_ASSERT(grid.signs(j) == std::vector<int>({sx, sy}));
    }
    check_tree(grid);
    CLPOLY_TEST("four-dimensional coordinate decomposition");
    // 四条坐标超平面的积结构共有 3^4 个胞腔，维数计数为 1、8、24、32、16。
    variable w("w");
    auto four = full_cad(std::vector<P>{P(x), P(y), P(z), P(w)}, {w, z, y, x});
    CLPOLY_ASSERT_EQ(four.leaf_count(), size_t(81));
    size_t counts[5] = {0, 0, 0, 0, 0};
    for (size_t j = 0; j < four.leaf_count(); ++j)
        ++counts[four.dimension(3, j)];
    const size_t expected[5] = {1, 8, 24, 32, 16};
    for (size_t j = 0; j < 5; ++j)
        CLPOLY_ASSERT_EQ(counts[j], expected[j]);
    check_tree(four);
    CLPOLY_TEST("general QQ variable order and direct lexicographic CAD driver");
    const grlex_<custom_var_order> graded(custom_var_order({y,x}));
    polynomial_<QQ, grlex_<custom_var_order>> general(&graded);
    poly_convert(P(x*x+y*y-1), general);
    auto negative_circle = full_cad(std::vector<decltype(general)>{general * QQ(-1,3)}, {y,x});
    CLPOLY_ASSERT_EQ(negative_circle.leaf_count(), circle.leaf_count());
    for (size_t j = 0; j < circle.leaf_count(); ++j)
        CLPOLY_ASSERT_EQ(negative_circle.signs(j)[0], -circle.signs(j)[0]);
    auto direct_order = std::make_shared<const full_cad_result::order_type>(custom_var_order({y,x}));
    full_cad_result::poly_type integer(direct_order.get());
    regular_chain::polynomial_type rational_input(direct_order.get());
    poly_convert(P(x*x+y*y-1), integer);
    poly_convert(P(x*x+y*y-1), rational_input);
    auto direct = __full_cad({integer}, {rational_input}, {y,x}, direct_order);
    CLPOLY_ASSERT_EQ(direct.leaf_count(), circle.leaf_count());
    check_tree(direct);
    check_tree(negative_circle);
    bool invalid_result = false;
    try
    {
        full_cad_result invalid(nullptr, {}, {});
    }
    catch (const std::invalid_argument &)
    {
        invalid_result = true;
    }
    CLPOLY_ASSERT(invalid_result);
    CLPOLY_TEST("result construction checks shape, order ownership and variable coverage");
    for (int invalid_case = 0; invalid_case < 4; ++invalid_case)
    {
        bool invalid = false;
        try
        {
            if (invalid_case == 0)
                full_cad_result bad(direct_order, {y,x}, {{}});
            else if (invalid_case == 1)
                full_cad_result bad(direct_order, {x,y}, {{},{}});
            else if (invalid_case == 2)
            {
                full_cad_result::order_type foreign(custom_var_order({y,x}));
                full_cad_result::poly_type p(&foreign);
                poly_convert(P(y), p);
                full_cad_result bad(direct_order, {y,x}, {{p},{}});
            }
            else
            {
                full_cad_result::poly_type p(direct_order.get());
                poly_convert(P(y+z), p);
                full_cad_result bad(direct_order, {y,x}, {{p},{}});
            }
        }
        catch (const std::invalid_argument &)
        {
            invalid = true;
        }
        CLPOLY_ASSERT(invalid);
    }
    CLPOLY_TEST("certified stack structure and sample membership");
    for (const auto *tree :
         {&c, &axes, &circle, &empty, &zero, &copied, &nul, &mix, &shared, &rational})
        check_tree(*tree);
    return clpoly_test::test_summary();
}
