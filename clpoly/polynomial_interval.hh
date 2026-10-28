/** 多项式区间求值，独立于三角列与实根算法。 */
#ifndef CLPOLY_POLYNOMIAL_INTERVAL_HH
#define CLPOLY_POLYNOMIAL_INTERVAL_HH

#include <clpoly/polynomial.hh>
#include <clpoly/rational_interval.hh>
#include <cassert>
#include <stdexcept>
#include <vector>

namespace clpoly
{
// PI-HORNER：[first,last) 是按字典序排列的非空连续项段，
// 各项共享前 offset 个变量与指数对。递归直接读取原项段，不复制多项式。
template<class Iterator>
rational_interval __assign_interval_horner(
    Iterator first, Iterator last, std::size_t offset, std::size_t var_begin,
    const std::vector<variable>& vars,
    const std::vector<rational_interval>& box)
{
    if (first->first.size() == offset)
        return rational_interval(QQ(first->second));

    const variable v = first->first[offset].first;
    // 后缀中的变量均排在已处理变量之后。核心接口的前置条件
    // 保证 v 出现在这个有序变量表中。
    std::size_t index = var_begin;
    while (index < vars.size() && vars[index] != v)
        ++index;
    assert(index < vars.size());
    const rational_interval& x = box[index];
    const auto exponent = [offset, v](Iterator term) -> int64_t
    {
        const auto& monomial = term->first;
        if (monomial.size() > offset && monomial[offset].first == v)
            return monomial[offset].second;
        return 0;
    };

    rational_interval result;
    int64_t previous = 0;
    auto group = first;
    while (group != last)
    {
        const int64_t degree = exponent(group);
        auto end = group + 1;
        while (end != last && exponent(end) == degree)
            ++end;

        // 零次组中没有变量 v 对应的项，因此保持单项式后缀偏移不变。
        auto coefficient = __assign_interval_horner(
            group, end, offset + (degree > 0 ? 1 : 0), index + 1, vars, box);
        if (group == first)
        {
            result = std::move(coefficient);
        }
        else
        {
            // 字典序保证 previous > degree >= 0，直接计算整个指数间隙的幂。
            // 当区间 x 跨过零时，pow(x,2) 比 x*x 给出的包络更窄。
            result *= pow(x, previous - degree);
            result += coefficient;
        }
        previous = degree;
        group = end;
    }
    if (previous > 0)
        result *= pow(x, previous);
    return result;
}

// 字典序核心，沿用 __project 的接口风格：调用者保证 f 已规范化，
// vars 与 box 的坐标一一对应，且 vars 按 f.comp() 严格递增。
// 仅断言坐标尺寸、变量顺序和访问下标；多项式自身的不变量由调用者保证。
template<class Tc, class VarOrder>
rational_interval __assign_interval(
    const polynomial_<Tc, lex_<VarOrder>>& f,
    const std::vector<variable>& vars,
    const std::vector<rational_interval>& box)
{
    assert(vars.size() == box.size());
#ifndef NDEBUG
    for (std::size_t i = 1; i < vars.size(); ++i)
        assert(f.comp()(vars[i - 1], vars[i]));
#endif
    if (f.empty())
        return rational_interval();
    return __assign_interval_horner(f.begin(), f.end(), 0, 0, vars, box);
}

// 一般入口，沿用 project 的接口风格：vars 同时指定盒坐标与目标字典序的
// 变量优先级。已满足字典序核心前置条件时，直接调用 __assign_interval。
template<class Tc, class To>
rational_interval assign_interval(
    const polynomial_<Tc, To>& f,
    const std::vector<variable>& vars,
    const std::vector<rational_interval>& box)
{
    if (vars.size() != box.size())
        throw std::invalid_argument("assign_interval: inconsistent variable box");
    // 目标变量序本身已有变量映射，复用它检查坐标，不另建集合。
    const lex_<custom_var_order> order(vars);
    if (order.comp.v_map.size() != vars.size())
        throw std::invalid_argument("assign_interval: duplicate variable");
    for (const auto& vp : get_variables(f))
    {
        if (order.comp.v_map.find(vp.first) == order.comp.v_map.end())
            throw std::invalid_argument("assign_interval: unbound variable");
    }
    polynomial_<Tc, lex_<custom_var_order>> sorted(&order);
    poly_convert(f, sorted);
    return __assign_interval(sorted, vars, box);
}
} // 命名空间 clpoly
#endif
