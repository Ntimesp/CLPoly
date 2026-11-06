/**
 * @file full_cad.hh
 * @brief 完整 Lazard CAD 的投影与提升算法
 *
 * 复用原有投影算子与 cad_tree，实根运算由实代数点模块提供。
 * 规格见 docs/cad/design/modular-cad/lazard-and-cad.md。
 */
#ifndef CLPOLY_FULL_CAD_HH
#define CLPOLY_FULL_CAD_HH

#include <clpoly/cad.hh>
#include <clpoly/full_cad_result.hh>
#include <clpoly/lazard_qq.hh>
#include <clpoly/rational_sample.hh>

namespace clpoly{

    // CAD 层沿用 lazard_evaluate_qq 的结果与输入契约，变量表采用提升顺序。
    // 返回值仍须相对于同一个父样本 a 解释。
    template <class Tc,class comp>
    lazard_evaluation_result lazard_evaluate(const polynomial_<Tc,comp> &f,
                                             const std::vector<variable> &vars, const cad_sample &a)
    {
        return lazard_evaluate_qq(f, vars, a);
    }

    // PDF 算法 12（LIFT-STACK）：alpha 是父样本，所有求值均相对于该点解释，
    // 父节点按存储顺序访问。根证书描述样本纤维上的根，来源身份引用投影定理
    // 所保证的有序连续根函数。
    // evaluations 与本层投影列表逐项对应，alpha 为上一层完整前缀；level=0 使用空点。
    inline void __lift_stack(full_cad_result &result, size_t level, size_t parent,
                           const cad_sample &alpha,
                           const std::vector<lazard_evaluation_result> &evaluations)
    {
        using P = regular_chain::polynomial_type;
        assert(level < result.num_levels() && alpha.size() == level);
        assert(level ? parent < result.level_size(level-1) : parent == SIZE_MAX);
        assert(evaluations.size() == result.level_polys(level).size());
        // 在 alpha 上专门化与 QQ 乘积交换；乘积的不同根就是全部因子根的并，
        // 其中共享根只保留一次。
        P product(result.order());
        product.push_back({basic_monomial<full_cad_result::order_type>(result.order()), QQ(1)});
        std::vector<P> factors;
        factors.reserve(evaluations.size());
        for (const auto &e : evaluations)
        {
            assert(e.valuation.size() == level && !e.polynomial.empty());
            P p(result.order());
            poly_convert(e.polynomial, p);
            product = product * p;
            factors.push_back(std::move(p));
        }
        cad_stack st;
        st.evaluations = evaluations;
        st.roots = __roots_over(product, alpha, result.level_var(level));
        // 每个来源分别计算不同根序号，共享根对所有相关来源都计数。
        // 选第一个来源作为规范身份，保持既有 cad_root(poly_idx,root_idx) 的含义。
        std::vector<size_t> root_counts(evaluations.size(), 0);
        std::vector<cad_root> boundaries;
        for (auto &root : st.roots)
        {
            // 根在乘积中只出现一次，但必须精确识别它属于哪些来源多项式。
            std::vector<size_t> sources;
            for (size_t j = 0; j < evaluations.size(); ++j)
            {
                if (__point_zero(factors[j], root))
                {
                    sources.push_back(j);
                }
            }
            assert(!sources.empty());
            const size_t source = sources.front();
            // 先读取该来源当前序号构造身份，再推进全部相关来源的计数。
            boundaries.emplace_back(source, root_counts[source]);
            for (size_t index : sources)
            {
                ++root_counts[index];
            }
            st.sources.push_back(std::move(sources));
        }
        const size_t dim = level ? result.dimension(level - 1, parent) : 0;
        // 子节点按 Sector、Section、…、Sector 连续存储。
        // 无根时生成一个圆柱胞腔，样本取零；其余 Sector 在认证的 QQ
        // 开间隙中选择最简有理数，不改变 Section 的根证书。
        for (size_t i = 0; i <= st.roots.size(); ++i)
        {
            QQ q(0);
            if (!st.roots.empty())
            {
                if (i == 0)
                {
                    q = sample_rational(std::nullopt, st.roots.front().box().back().left());
                }
                else if (i == st.roots.size())
                {
                    q = sample_rational(st.roots.back().box().back().right(), std::nullopt);
                }
                else
                {
                    // 区间端点只是有理内界，先分离两侧根盒，才能给出合法的开间隙。
                    while (st.roots[i - 1].box().back().right() >= st.roots[i].box().back().left())
                    {
                        st.roots[i - 1].shrink();
                        st.roots[i].shrink();
                    }
                    q = sample_rational(st.roots[i - 1].box().back().right(),
                                        st.roots[i].box().back().left());
                }
            }
            auto left = i ? boundaries[i - 1] : cad_root::neginf();
            auto right = i < st.roots.size() ? boundaries[i] : cad_root::inf();
            result.add_sector(level, parent, left, right, q,
                              append_rational(alpha, result.level_var(level), q, result.order()), dim + 1);
            if (i < st.roots.size())
            {
                result.add_section(level, parent, boundaries[i], st.roots[i], dim);
            }
        }
        result.add_stack(level, std::move(st));
    }

    // 同一字典序下的投影、提升核心。polys[i] 是 originals[i] 的正倍数；
    // 全部输入规范化、变量受 vars 覆盖，并借用 order 所拥有的比较器。
    inline full_cad_result __full_cad(
        const std::vector<full_cad_result::poly_type> &polys,
        const std::vector<regular_chain::polynomial_type> &originals,
        const std::vector<variable> &vars,
        std::shared_ptr<const full_cad_result::order_type> order)
    {
        assert(order && polys.size() == originals.size());
        assert(order->comp.v_map.size() == vars.size());
#ifndef NDEBUG
        for (size_t i = 0; i < vars.size(); ++i)
            assert(order->comp.v_map.at(vars[i]) == static_cast<int64_t>(i + 1));
        for (const auto &f : polys)
        {
            assert(comp_consistent(f.comp(), *order));
            for (const auto &term : f)
                for (const auto &ve : term.first)
                    assert(ve.second >= 0 && order->comp.v_map.count(ve.first));
        }
        for (const auto &f : originals)
            assert(comp_consistent(f.comp(), *order));
#endif
        // 使用已审核的 Lazard/FACTOR 投影契约，见 PDF 第 9 节。
        auto projection =
            __project_full(polys, vars, projection_method::LAZARD, basis_computation_method::FACTOR);
        // 多项式借用比较器指针，统一重新绑定到结果所持有的比较器。
        for (auto &layer : projection)
        {
            for (auto &p : layer)
            {
                full_cad_result::poly_type q(order.get());
                poly_convert(p, q);
                p = std::move(q);
            }
        }
        full_cad_result result(order, vars, std::move(projection));
        std::vector<variable> lifting(vars.rbegin(), vars.rend());
        // 每层先处理全部父胞腔，同一父栈一次完整追加，保持树的连续子节点布局。
        for (size_t level = 0; level < vars.size(); ++level)
        {
            std::vector<variable> prefix(lifting.begin(), lifting.begin() + level + 1);
            const size_t parents = level ? result.level_size(level - 1) : 1;
            for (size_t j = 0; j < parents; ++j)
            {
                auto a = level ? result.sample(level - 1, j) : cad_sample{};
                std::vector<lazard_evaluation_result> gs;
                for (const auto &f : result.level_polys(level))
                {
                    // 每个父样本分别计算 valuation，不能在不同父样本之间复用代表的解释。
                    lazard_evaluation_result e(order);
                    poly_convert(f, e.polynomial);
                    __lazard_evaluate_qq(e, prefix, a);
                    gs.push_back(std::move(e));
                }
                __lift_stack(result, level, level ? j : SIZE_MAX, a, gs);
            }
        }

        // 因子归一化不保证保留原符号，因此叶符号从原 QQ 输入计算。
        std::vector<std::vector<int>> signs;
        for (size_t j = 0; j < result.leaf_count(); ++j)
        {
            auto a = result.leaf_sample(j);
            std::vector<int> row;
            for (const auto &f : originals)
            {
                row.push_back(__point_sign(f, a));
            }
            signs.push_back(std::move(row));
        }
        result.set_signs(std::move(signs));
        return result;
    }
    // PDF 算法 13（FULL-CAD）：先投影，再逐层提升，最后计算原输入的叶符号。
    // 公开 vars 为投影顺序：{x0,x1,x2} 先消去 x0，再消去 x1。
    // 提升及样本坐标采用 {x2,x1,x0}；三角列内部比较器恢复为投影优先级。
    // 接受 ZZ/QQ 输入，零与常数允许；重复坐标、未声明变量或负指数抛 invalid_argument。
    // 返回所有胞腔的树、实点样本和原输入符号；完整性依据投影及逐父完整栈契约。
    template <class Tc,class comp>
    full_cad_result full_cad(const std::vector<polynomial_<Tc,comp>> &input,
                             const std::vector<variable> &vars)
    {
        const std::set<variable> unique(vars.begin(), vars.end());
        if (unique.size() != vars.size())
        {
            throw std::invalid_argument("full_cad: duplicate variables");
        }
        auto order = std::make_shared<const full_cad_result::order_type>(custom_var_order(vars));
        std::vector<full_cad_result::poly_type> polys;
        std::vector<regular_chain::polynomial_type> originals;
        for (const auto &f : input)
        {
            for (const auto &term : f)
            {
                for (const auto &ve : term.first)
                {
                    if (ve.second < 0 || !unique.count(ve.first))
                    {
                        throw std::invalid_argument("full_cad: undeclared variable");
                    }
                }
            }
            full_cad_result::poly_type p(order.get());
            // 投影使用清正分母后的整数方程，叶符号另保留原值的 QQ 表达式。
            poly_convert(f, p);
            polys.push_back(std::move(p));
            regular_chain::polynomial_type q(order.get());
            poly_convert(f, q);
            originals.push_back(std::move(q));
        }

        return __full_cad(polys, originals, vars, std::move(order));
    }

}
#endif
