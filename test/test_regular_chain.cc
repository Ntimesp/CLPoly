#include <clpoly/regular_chain.hh>
#include "clpoly_test.hh"
using namespace clpoly;
using P = polynomial_ZZ;

// 独立在给定有理点代入，验证包装器的变量绑定和分支标签。
static bool contains(const regular_chain &t, const variable &x, const variable &y,
                     const QQ &a, const QQ &b)
{
    for (const auto &f : t.equations())
    {
        QQ value(0);
        for (const auto &term : f)
            value += term.second * pow(a, term.first.deg(x)) * pow(b, term.first.deg(y));
        if (value != QQ(0))
            return false;
    }
    return true;
}
int main()
{
    variable x("x"), y("y"), z("z");
    const regular_chain::order_type order(custom_var_order({z, y, x}));
    CLPOLY_TEST("regular chains, prefixes, borrowed order and multiplicities");
    // 正则平方自由链及其一层前缀，主变量按提升方向依次为 x、y。
    regular_chain t({x, y}, std::vector<P>{x * x - 2, y * y - x}, &order);
    CLPOLY_ASSERT(t.size() == 2 && t.is_squarefree());
    auto prefix = t.prefix(1);
    CLPOLY_ASSERT(prefix.variables() == std::vector<variable>{x});
    CLPOLY_ASSERT(prefix.is_squarefree());
    // 源链销毁后，前缀继续借用 main 中仍存活的比较器。
    auto borrowed = [&]
    {
        regular_chain source = t;
        return source.prefix(1);
    }();
    CLPOLY_ASSERT((borrowed.equations()[0] - borrowed.convert(P(x*x - 2))).empty());
    // 首层含重根；平方自由分解保留两个不同根，并将首层次数降为 2。
    regular_chain repeated({x, y}, std::vector<P>{pow(x - 1, 3) * pow(x + 2, 2), y - x}, &order);
    CLPOLY_ASSERT(!repeated.is_squarefree());
    auto sf = squarefree_decomposition(repeated);
    CLPOLY_ASSERT(sf.size() == 1 && sf[0].is_squarefree());
    CLPOLY_ASSERT(degree(sf[0].equations()[0], x) == 2);
    bool rejected = false;
    try
    {
        // 前缀根 x=0 会使下一层初式 x 消失，应拒绝构造非正则链。
        regular_chain invalid({x, y}, std::vector<P>{x * (x - 1), x * y - 1}, &order);
    }
    catch (const std::invalid_argument &)
    {
        rejected = true;
    }
    CLPOLY_ASSERT(rejected);

    CLPOLY_TEST("projection priority and lifting coordinates");
    // 比较器优先级为 x、y、z，但求解及方程排列顺序为 z、y、x。
    const regular_chain::order_type reverse_order(custom_var_order({x, y, z}));
    regular_chain ordered({z, y, x}, std::vector<P>{z * z - 2, y - z, x - y}, &reverse_order);
    std::vector<variable> main_variables;
    for (const auto &equation : ordered.equations())
        main_variables.push_back(get_first_var(equation));
    CLPOLY_ASSERT(main_variables == std::vector<variable>({z, y, x}));
    auto priority = ordered.variables();
    std::sort(priority.begin(), priority.end(), *ordered.order());
    CLPOLY_ASSERT(priority == std::vector<variable>({x, y, z}));
    auto ordered_prefix = ordered.prefix(2);
    CLPOLY_ASSERT(ordered_prefix.variables() == std::vector<variable>({z, y}));
    CLPOLY_ASSERT_EQ(get_first_var(ordered_prefix.equations().back()), y);

    CLPOLY_TEST("copy and move preserve the borrowed order");
    // 通过实际多项式运算检查复制／移动后的引用，而非比较内部指针地址。
    auto copied = [&]
    {
        regular_chain source = t;
        regular_chain copy(source);
        return copy;
    }();
    CLPOLY_ASSERT((copied.equations()[1] - copied.convert(P(y * y - x))).empty());
    const regular_chain::order_type assigned_order(custom_var_order({x, y}));
    regular_chain assigned({y, x}, std::vector<P>{y * y - 3, x - y}, &assigned_order);
    assigned = copied;
    CLPOLY_ASSERT((assigned.equations()[1] - assigned.convert(P(y * y - x))).empty());
    regular_chain moved(std::move(copied));
    CLPOLY_ASSERT((moved.equations()[1] - moved.convert(P(y * y - x))).empty());
    assigned = std::move(ordered_prefix);
    CLPOLY_ASSERT((assigned.equations()[1] - assigned.convert(P(y - z))).empty());
    // 第二次插入触发扩容，原对象被移动到新的地址。
    std::vector<regular_chain> relocated;
    relocated.reserve(1);
    relocated.push_back(std::move(moved));
    relocated.push_back(ordered);
    CLPOLY_ASSERT((relocated[0].equations()[1] -
                   relocated[0].convert(P(y * y - x))).empty());
    // 副本、前缀和容器扩容后的对象均借用原来的外部序。
    CLPOLY_ASSERT(t.order() == &order && prefix.order() == &order &&
                  relocated[0].order() == &order && assigned.order() == &reverse_order);

    CLPOLY_TEST("prefix inherits its own squarefree certificate");
    // 重根只出现在第二层，第一层前缀及空前缀仍应平方自由。
    regular_chain higher_repeated({x, y}, std::vector<P>{x * x - 2, pow(y - x, 2)}, &order);
    CLPOLY_ASSERT(!higher_repeated.is_squarefree());
    CLPOLY_ASSERT(higher_repeated.prefix(1).is_squarefree());
    CLPOLY_ASSERT(higher_repeated.prefix(0).is_squarefree());

    CLPOLY_TEST("WRSD splits selected-point ambiguity without intervals");
    // 四个实点中，y=x 的两个点属于零分支，y=-x 的两个点属于非零分支。
    regular_chain product({x, y}, std::vector<P>{x * x - 2, y * y - 2}, &order);
    auto split = weak_split_zero(product, P(y - x));
    CLPOLY_ASSERT(!split.zero.empty() && !split.nonzero.empty());
    // 非零分支用逐次结式断言诊断；零分支直接使用 WRSD 的零集标签。
    size_t zero_at_positive = 0, nonzero_at_opposite = 0;
    for (const auto &branch : split.zero)
        if (branch.size() == 2)
            ++zero_at_positive;
    for (const auto &branch : split.nonzero)
        if (branch.size() == 2)
            ++nonzero_at_opposite;
    CLPOLY_ASSERT(zero_at_positive > 0 && nonzero_at_opposite > 0);
    auto all = weak_split_zero(product, P(x * x - 2));
    CLPOLY_ASSERT(!all.zero.empty() && all.nonzero.empty());
    auto none = weak_split_zero(product, P(x * x + 1));
    CLPOLY_ASSERT(none.zero.empty() && !none.nonzero.empty());
    regular_chain empty;
    CLPOLY_ASSERT(weak_split_zero(empty, P(0)).zero.size() == 1);
    CLPOLY_ASSERT(weak_split_zero(empty, P(1)).nonzero.size() == 1);

    CLPOLY_TEST("WRSD keeps the zero set of a repeated prefix");
    // 唯一零点为 (0,0)，xy 在该点为零；保留原链也是合法的 WRSD 零分支。
    regular_chain repeated_prefix({x, y}, std::vector<P>{x*x, y*y*y}, &order);
    const auto repeated_split = weak_split_zero(repeated_prefix, P(x*y));
    CLPOLY_ASSERT(repeated_split.zero.size() == 1 && repeated_split.nonzero.empty());
    CLPOLY_ASSERT_EQ(repeated_split.zero.front().equations(), repeated_prefix.equations());

    CLPOLY_TEST("squarefree decomposition of empty chain");
    // 空链表示唯一空元组，分解结果是一条空链，而非空结果列表。
    const auto empty_decomposition = squarefree_decomposition(empty);
    CLPOLY_ASSERT(empty_decomposition.size() == 1 && empty_decomposition[0].size() == 0);

    CLPOLY_TEST("checked conversion and internal lexicographic entries");
    // 输入比较器夹有未使用变量 z，公开入口转换到调用者指定的连续目标序。
    lex_<custom_var_order> wider_order(custom_var_order({y, z, x}));
    using wider_polynomial = polynomial_<ZZ, lex_<custom_var_order>>;
    wider_polynomial first(&wider_order), second(&wider_order);
    first = x*x - 2;
    second = y*y - 2;
    regular_chain rebound({x, y}, std::vector<wider_polynomial>{first, second}, &order);
    polynomial_<QQ, lex_<custom_var_order>> f(&wider_order);
    f = y - x;
    for (auto &term : f)
        term.second /= QQ(2);
    const auto q = rebound.convert(f);
    const auto checked = weak_split_zero(rebound, f);
    const auto direct = __weak_split_zero(rebound, q);
    CLPOLY_ASSERT_EQ(checked.zero.front().equations(), direct.zero.front().equations());
    CLPOLY_ASSERT_EQ(checked.nonzero.front().equations(), direct.nonzero.front().equations());
    const auto checked_prefix = t.prefix(1);
    const auto direct_prefix = t.__prefix(1);
    CLPOLY_ASSERT_EQ(checked_prefix.equations(), direct_prefix.equations());

    CLPOLY_TEST("public entries reject invalid input");
    // 公开入口必须在 Release 中同样拒绝未声明变量和越界前缀长度。
    rejected = false;
    try
    {
        weak_split_zero(product, P(z));
    }
    catch (const std::invalid_argument &)
    {
        rejected = true;
    }
    CLPOLY_ASSERT(rejected);
    rejected = false;
    try
    {
        // WRSD 的 cls 用于方程下标，目标序不能在链坐标之间夹入其他变量。
        regular_chain invalid({x, y}, std::vector<P>{x*x - 2, y*y - 2}, &wider_order);
    }
    catch (const std::invalid_argument &)
    {
        rejected = true;
    }
    CLPOLY_ASSERT(rejected);
    rejected = false;
    try
    {
        regular_chain invalid(nullptr);
    }
    catch (const std::invalid_argument &)
    {
        rejected = true;
    }
    CLPOLY_ASSERT(rejected);
    rejected = false;
    try
    {
        product.prefix(3);
    }
    catch (const std::out_of_range &)
    {
        rejected = true;
    }
    CLPOLY_ASSERT(rejected);

    CLPOLY_TEST("internal branches borrow the caller's order");
    // 源链销毁后，分解分支继续使用外部序，仍能做精确运算。
    auto detached = [&]
    {
        regular_chain source({x, y}, std::vector<P>{x*x - 2, y*y - 2}, &order);
        return __weak_split_zero(source, source.convert(P(y - x)));
    }();
    const auto &zero_branch = detached.zero.front();
    CLPOLY_ASSERT((zero_branch.equations().back() - zero_branch.convert(P(y - x))).empty());
    CLPOLY_ASSERT(zero_branch.order() == &order && sf.front().order() == &order);

    CLPOLY_TEST("WRSD labels under independent rational substitution");
    regular_chain rational({x, y}, std::vector<P>{x*x - 1, y*y - 1}, &order);
    polynomial_QQ query;
    poly_convert(P(y-x), query);
    const auto labelled = weak_split_zero(rational, query * QQ(-2, 3));
    for (int a : {-1, 1})
        for (int b : {-1, 1})
        {
            bool zero = false, nonzero = false;
            for (const auto &branch : labelled.zero)
                zero = zero || contains(branch, x, y, QQ(a), QQ(b));
            for (const auto &branch : labelled.nonzero)
                nonzero = nonzero || contains(branch, x, y, QQ(a), QQ(b));
            CLPOLY_ASSERT_EQ(zero, a == b);
            CLPOLY_ASSERT_EQ(nonzero, a != b);
        }
    CLPOLY_TEST("constructor rejects invalid coordinate and equation structure");
    const auto rejects = [&](std::vector<variable> vars, std::vector<P> equations)
    {
        try
        {
            regular_chain invalid(std::move(vars), equations, &order);
        }
        catch (const std::invalid_argument &)
        {
            return true;
        }
        return false;
    };
    CLPOLY_ASSERT(rejects({x,x}, {P(x),P(x)}));
    CLPOLY_ASSERT(rejects({x,y}, {P(x)}));
    CLPOLY_ASSERT(rejects({x}, {P(1)}));
    CLPOLY_ASSERT(rejects({x}, {P(y)}));
    CLPOLY_ASSERT(rejects({x,y}, {P(x),P(z+y)}));

    return clpoly_test::test_summary();
}
