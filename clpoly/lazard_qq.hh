/**
 * @file lazard_qq.hh
 * @brief 保留冻结变量的 QQ Lazard 求值
 *
 * 前缀判零与连续求导均使用有理系数多项式。
 * 规格见 docs/cad/design/modular-cad/lazard-and-cad.md。
 */
#ifndef CLPOLY_LAZARD_QQ_HH
#define CLPOLY_LAZARD_QQ_HH

#include <clpoly/real_algebraic.hh>
#include <clpoly/lazard_qq_result.hh>
#include <set>

namespace clpoly{

    // 特定 QQ 字典序核心：活动变量排在冻结变量之前，冻结变量保持链的相对序。
    // p 已规范化且只含这两组坐标；输入合法性由调用者保证。
    inline bool __prefix_zero(const regular_chain::polynomial_type &p,
                              real_algebraic_point &prefix, const std::vector<variable> &active)
    {
        using P = regular_chain::polynomial_type;
        const auto &frozen = prefix.chain().variables();
#ifndef NDEBUG
        std::set<variable> all(frozen.begin(), frozen.end());
        for (const auto &v : active)
        {
            assert(all.insert(v).second);
            for (const auto &w : frozen)
                assert(p.comp()(v, w));
        }
        for (size_t i = 1; i < frozen.size(); ++i)
            assert(p.comp()(frozen[i], frozen[i-1]));
        for (const auto &term : p)
            for (const auto &ve : term.first)
                assert(ve.second >= 0 && all.count(ve.first));
#endif
        std::vector<int64_t> previous;
        P coefficient(prefix.chain().order());
        for (const auto &term : p)
        {
            std::vector<int64_t> powers(active.size(), 0);
            basic_monomial<regular_chain::order_type> m(prefix.chain().order());
            for (const auto &ve : term.first)
            {
                auto it = std::find(active.begin(), active.end(), ve.first);
                if (it == active.end())
                    m.push_back(ve);
                else
                    powers[it - active.begin()] = ve.second;
            }
            // 活动变量具有较高优先级，相同活动单项式的项必定连续。
            // 冻结变量的相对序未变，所以各组系数仍是规范化多项式。
            if (!coefficient.empty() && powers != previous)
            {
                assert(coefficient.is_normal());
                if (!__point_zero(coefficient, prefix))
                    return false;
                coefficient.clear();
            }
            coefficient.push_back({std::move(m), term.second});
            previous = std::move(powers);
        }
        assert(coefficient.is_normal());
        return coefficient.empty() || __point_zero(coefficient, prefix);
    }

    // 一般序入口：完整检查变量和指数，再构造活动变量优先的 QQ 字典序。
    template <class Tc,class comp>
    bool prefix_zero(const polynomial_<Tc,comp> &p, real_algebraic_point &prefix,
                     const std::vector<variable> &active)
    {
        const auto &frozen = prefix.chain().variables();
        std::set<variable> all(frozen.begin(), frozen.end());
        for (const auto &v : active)
            if (!all.insert(v).second)
                throw std::invalid_argument("PREFIX-ZERO: repeated variable");
        for (const auto &term : p)
            for (const auto &ve : term.first)
                if (ve.second < 0 || !all.count(ve.first))
                    throw std::invalid_argument("PREFIX-ZERO: invalid variable");
        std::vector<variable> vars(active.rbegin(), active.rend());
        vars.insert(vars.end(), frozen.rbegin(), frozen.rend());
        const regular_chain::order_type order{custom_var_order(vars)};
        regular_chain::polynomial_type q(&order);
        poly_convert(p, q);
        return __prefix_zero(q, prefix, active);
    }

    // result 的 QQ 代表和比较器由调用者准备；此核心不重复外部检查或序转换。
    // 初始代表非零、valuation 为空，比较器采用 lifting_order 的反序。
    inline void __lazard_evaluate_qq(lazard_qq_result &result,
                                     const std::vector<variable> &lifting_order,
                                     const real_algebraic_point &sample)
    {
        assert(result.order && !result.polynomial.empty() && result.valuation.empty());
        assert(result.polynomial.comp_ptr() == result.order.get());
        assert(lifting_order.size() == sample.size() + 1);
        assert(std::set<variable>(lifting_order.begin(), lifting_order.end()).size() == lifting_order.size());
        assert(std::equal(sample.chain().variables().begin(), sample.chain().variables().end(),
                          lifting_order.begin()));
        assert(xz_detail::compatible_coordinates(lifting_order, result.order.get()));
        for (size_t i = 0; i < sample.size(); ++i)
        {
            auto prefix = sample.prefix(i + 1);
            auto h = result.polynomial;
            size_t r = 0;
            ZZ factorial(1);
            std::vector<variable> active(lifting_order.begin() + i + 1, lifting_order.end());
            // 保留冻结变量的形式代表；不能先模三角列约简再求导。
            while (__prefix_zero(h, prefix, active))
            {
                h = derivative(h, lifting_order[i]);
                ++r;
                factorial *= ZZ(r);
                assert(!h.empty());
            }
            for (auto &term : h)
                term.second /= QQ(factorial);
            result.polynomial = std::move(h);
            result.valuation.push_back(r);
        }
    }

    // PDF 算法 8：求导过程中保留未约简的 QQ 代表。
    // 预先模三角列约简会破坏需要提取的 Taylor 系数。
    // lifting_order 为已有样本坐标加一个新变量，按提升方向排列；f 必须非零。
    // 返回代表及 valuation；sample 按 const 引用保留，局部前缀副本可被精化。
    template <class Tc,class comp>
    lazard_qq_result lazard_evaluate_qq(const polynomial_<Tc,comp> &f,
                                        const std::vector<variable> &lifting_order,
                                        const real_algebraic_point &sample)
    {
        if (lifting_order.size() != sample.size() + 1 ||
            std::set<variable>(lifting_order.begin(), lifting_order.end()).size() !=
                lifting_order.size() ||
            !std::equal(sample.chain().variables().begin(), sample.chain().variables().end(),
                        lifting_order.begin()))
        {
            throw std::invalid_argument("LAZARD-QQ: inconsistent variable order");
        }
        if (f.empty())
        {
            throw std::domain_error("LAZARD-QQ: zero input");
        }
        for (const auto &term : f)
        {
            for (const auto &ve : term.first)
            {
                if (ve.second < 0 || std::find(lifting_order.begin(), lifting_order.end(), ve.first) ==
                                         lifting_order.end())
                {
                    throw std::invalid_argument("LAZARD-QQ: undeclared variable");
                }
            }
        }
        auto order = std::make_shared<const regular_chain::order_type>(
            custom_var_order(std::vector<variable>(lifting_order.rbegin(), lifting_order.rend())));
        lazard_qq_result result(order);
        poly_convert(f, result.polynomial);
        __lazard_evaluate_qq(result, lifting_order, sample);
        return result;
    }
}
#endif
