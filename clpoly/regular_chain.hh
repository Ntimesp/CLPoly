/**
 * @file regular_chain.hh
 * @brief 零维无参数正则链的数据结构与分解接口
 *
 * 分解复用 XY16 算法 2.2（WRSD）和 3.1（平方自由分解）。
 * 规格见 docs/cad/design/modular-cad/regular-chain.md。
 */
#ifndef CLPOLY_REGULAR_CHAIN_HH
#define CLPOLY_REGULAR_CHAIN_HH

#include <clpoly/polynomial.hh>
#include <clpoly/polynomial_gcd.hh>
#include <clpoly/resultant.hh>
#include <clpoly/charset.hh>
#include <clpoly/polynomial_convert.hh>
#include <cassert>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace clpoly{

    // 与多项式一致，只借用外部比较器；调用者保证其存活且变量序不变。
    // 仅保存零维无参数正则链；公开构造拒绝非正则输入，平方自由性另用标记记录。
    // 分解算法定义在类外。
    class regular_chain
    {
    public:
        // 保存 QQ 方程；调用原有整数算法时临时清分母，二者使用相同的字典序类型。
        using order_type = lex_<custom_var_order>;
        using polynomial_type = polynomial_<QQ, order_type>;
        using integer_polynomial = polynomial_<ZZ, order_type>;

    private:
        const order_type *_order = &order_type::init; // 与方程共用外部比较器。
        std::vector<variable> _vars; // 提升顺序，与 _equations 的主变量逐项对应。
        std::vector<polynomial_type> _equations; // 第 i 条方程的主变量是 _vars[i]。
        bool _squarefree = true; // 所有层在各自前缀根处是否均无重根。
        std::vector<bool> _squarefree_prefix; // 第 i 项记录前 i+1 个方程的累计性质。

        // 仅供内部算法装配已知正则的输出；调用者提供累计平方自由标记。
        friend regular_chain __make_regular_chain(
            const regular_chain &, const std::vector<integer_polynomial> &, std::vector<bool>);

    public:
        // 接受 ZZ/QQ 系数与一般多项式序，检查并建立零维正则链。
        // vars 按提升顺序排列，equations[i] 只能依赖 vars[0..i]，且主变量为 vars[i]。
        // vars 必须对应 order 的末尾连续变量，并按反序排列，以满足 WRSD 的 cls 编号。
        // 例如 order={x0,x1,x2}，vars 可为 {x2}、{x2,x1} 或 {x2,x1,x0}。
        // 非法结构或非正则初式抛 invalid_argument；重根允许存在，另用标记记录。
        template <class Tc,class comp>
        regular_chain(std::vector<variable> vars, const std::vector<polynomial_<Tc,comp>> &equations,
                       const order_type *order)
            : _order(order), _vars(std::move(vars))
        {
            // 每个坐标对应一条方程，重复坐标不能表示不同的提升层。
            if (!_order || equations.size() != _vars.size() ||
                std::set<variable>(_vars.begin(), _vars.end()).size() != _vars.size())
            {
                throw std::invalid_argument("regular_chain: invalid coordinate list");
            }
            for (size_t i = 0; i < equations.size(); ++i)
            {
                // 原有 WRSD 用 N+1-v_map[主变量] 定位方程，不能接受缺号的坐标表。
                const auto &positions = _order->comp.v_map;
                auto position = positions.find(_vars[i]);
                if (position == positions.end() || i >= positions.size() ||
                    position->second != static_cast<int64_t>(positions.size() - i))
                {
                    throw std::invalid_argument("regular_chain: incompatible coordinate order");
                }
                // 先检查原输入的变量与指数，再调用要求合法多项式的序转换。
                // 第 i 层仅依赖提升表的前 i+1 个变量。
                for (const auto &term : equations[i])
                {
                    for (const auto &ve : term.first)
                    {
                        if (ve.second < 0 || std::find(_vars.begin(), _vars.begin() + i + 1,
                                                       ve.first) == _vars.begin() + i + 1)
                        {
                            throw std::invalid_argument(
                                "regular_chain: equation uses a later or unbound variable");
                        }
                    }
                }
                polynomial_type q(_order);
                poly_convert(equations[i], q);
                // 每个坐标均有正次数方程，结合下方的初式检查保证零维正则性。
                if (q.empty() || degree(q, _vars[i]) <= 0)
                {
                    throw std::invalid_argument("regular_chain: missing main variable");
                }
                _equations.push_back(std::move(q));
            }
            // 按提升序逐层检查，当前前缀已是正则链。
            // 初式的逐次结式非零，保证它在所有复前缀根处均不消失。
            // 判别式另用于检查平方自由性，并保存每个前缀的累计标记。
            const auto z = integer_equations();
            for (size_t i = 0; i < z.size(); ++i)
            {
                auto initial = leadcoeff(z[i]);
                auto discriminant_i = discriminant(z[i], _vars[i]);
                // 从紧邻的低一层向首层消元，将条件化为可精确判零的常数。
                for (size_t j = i; j-- > 0;)
                {
                    initial = resultant(initial, z[j], _vars[j]);
                    if (!discriminant_i.empty())
                    {
                        discriminant_i = resultant(discriminant_i, z[j], _vars[j]);
                    }
                }
                if (initial.empty())
                {
                    // 此处只拒绝输入；将非正则系统分解为正则链由上层另行完成。
                    throw std::invalid_argument("regular_chain: nonregular initial");
                }
                if (discriminant_i.empty())
                {
                    // 判别式条件失败只影响平方自由性，不否定已通过的正则性。
                    _squarefree = false;
                }
                // 保存累计性质；后续层通过也不能恢复已经失败的前缀标记。
                _squarefree_prefix.push_back(_squarefree);
            }
        }

        // 空链作为逐层构造的起点，平方自由属性约定为 true。
        regular_chain() = default;

        // 创建借用指定序的空链；复制、移动及赋值均使用编译器生成的成员操作。
        explicit regular_chain(const order_type *order)
            : regular_chain(std::vector<variable>{}, std::vector<polynomial_type>{}, order)
        {}

        // 链长同时等于坐标数和方程数。
        size_t size() const
        {
            return _vars.size();
        }

        // 查询整条链的累计缓存，不重新计算判别式；空链返回 true。
        bool is_squarefree() const
        {
            return _squarefree;
        }

        // 借用提升坐标表，与方程主变量逐项对应；原链须保持存活且未移动或覆盖。
        const std::vector<variable> &variables() const
        {
            return _vars;
        }

        // 借用 QQ 方程列表，按提升顺序排列；通过 const 引用保持表示不变量。
        const std::vector<polynomial_type> &equations() const
        {
            return _equations;
        }

        // 返回外部字典序比较器；所有副本、前缀及分解输出共用此地址。
        const order_type *order() const
        {
            return _order;
        }

        // 检查外部 f 的变量覆盖和非负指数，转换为本链字典序下的同值 QQ 多项式。
        // f 可为零或常数；含未声明变量时抛 invalid_argument。
        // 返回多项式借用外部比较器，生命周期不依赖本链对象。
        template <class Tc,class comp>
        polynomial_type convert(const polynomial_<Tc,comp> &f) const
        {
            for (const auto &term : f)
            {
                for (const auto &ve : term.first)
                {
                    if (ve.second < 0 || std::find(_vars.begin(), _vars.end(), ve.first) == _vars.end())
                    {
                        throw std::invalid_argument("regular_chain: unbound variable");
                    }
                }
            }
            polynomial_type q(_order);
            poly_convert(f, q);
            return q;
        }

        // 复用 poly_convert 清正分母，得到供原有算法使用的 ZZ 方程列表。
        // 保持方程零集和符号，通常不保持多项式值；返回方程借用 _order。
        std::vector<integer_polynomial> integer_equations() const
        {
            std::vector<integer_polynomial> out;
            for (const auto &f : _equations)
            {
                integer_polynomial z(_order);
                poly_convert(f, z);
                out.push_back(std::move(z));
            }
            return out;
        }

        // 公开前缀入口：长度越界抛 out_of_range，合法时交给内部截取。
        // n=0 返回空链；方程按值复制，比较器继续借用同一个外部对象。
        regular_chain prefix(size_t n) const
        {
            if (n > size())
            {
                throw std::out_of_range("regular_chain: prefix length");
            }
            return __prefix(n);
        }

        // 内部入口：调用者保证 n 合法；复用已认证的前缀属性，不重算结式。
        regular_chain __prefix(size_t n) const
        {
            assert(n <= size());
            // 正则性和各前缀属性由原链保证，截去后续方程不会改变这些性质。
            // 保留完整比较器，前缀变量的 cls 编号仍从 1 开始连续。
            regular_chain result;
            result._vars.assign(_vars.begin(), _vars.begin() + n);
            result._order = _order;
            // 整链不平方自由时，较短前缀仍可能平方自由，须取该前缀自己的标记。
            result._squarefree_prefix.assign(_squarefree_prefix.begin(),
                                             _squarefree_prefix.begin() + n);
            result._squarefree = n == 0 || result._squarefree_prefix.back();
            result._equations.assign(_equations.begin(), _equations.begin() + n);
            return result;
        }
    };

    // 内部装配：方程按值保存，沿用 model 借用的外部比较器。
    // 正则性由 wrsd 或 RCtoSqrfree 的输出契约保证，flags 正确性由调用者保证。
    // 此处只用断言诊断，不重复公开构造器的检查。
    inline regular_chain __make_regular_chain(
        const regular_chain &model,
        const std::vector<regular_chain::integer_polynomial> &equations,
        std::vector<bool> flags)
    {
        assert(equations.size() == model.size());
        assert(flags.size() == model.size());
        assert(is_regular(equations));
        // 只复制比较器地址，不借用 model 或输入方程的存储。
        regular_chain result;
        result._order = model.order();
        result._vars = model.variables();
        result._squarefree_prefix = std::move(flags);
        result._squarefree = result._squarefree_prefix.empty() || result._squarefree_prefix.back();
        result._equations.reserve(equations.size());
        for (size_t i = 0; i < equations.size(); ++i)
        {
            assert(comp_consistent(equations[i].comp(), *model.order()));
            assert(!equations[i].empty());
            assert(get_first_var(equations[i]) == model.variables()[i]);
            // ZZ→QQ 保持多项式值，各项共用外部比较器。
            regular_chain::polynomial_type q(result.order());
            poly_convert(equations[i], q);
            result._equations.push_back(std::move(q));
        }
        return result;
    }

    // 复零集分解结果：zero 覆盖 f=0 的部分，nonzero 覆盖 f!=0 的部分。
    // 列表内的不同链可以有公共零点；这里保存定义方程，不枚举实根。
    struct regular_chain_split
    {
        std::vector<regular_chain> zero, nonzero;
    };

    // 输入 equations 已是与 t 同坐标、同字典序的正则 ZZ 链。
    // 用判别式建立各前缀的累计标记，检查的是前缀根处的重根，而非普通因子重复。
    inline std::vector<bool> __squarefree_prefixes(
        const regular_chain &t,
        const std::vector<regular_chain::integer_polynomial> &equations)
    {
        assert(equations.size() == t.size());
        std::vector<bool> flags;
        flags.reserve(equations.size());
        bool squarefree = true;
        for (size_t i = 0; i < equations.size(); ++i)
        {
            // 任一前缀不平方自由后，更长前缀的累计标记均为 false。
            if (squarefree)
            {
                auto d = discriminant(equations[i], t.variables()[i]);
                // 逐次结式非零，等价于判别式在全部复前缀根上都不消失。
                for (size_t j = i; j-- > 0 && !d.empty();)
                {
                    d = resultant(d, equations[j], t.variables()[j]);
                }
                squarefree = !d.empty();
            }
            flags.push_back(squarefree);
        }
        return flags;
    }

    // XY16 算法 2.2 的内部入口；q 已是 t 的 QQ 字典序多项式。
    // 输入的变量覆盖、非负指数和规范化由调用者保证，不在此转换一般序。
    // 返回两组借用 t.order() 的正则链，分别定义零与非零分支。
    inline regular_chain_split __weak_split_zero(
        const regular_chain &t, const regular_chain::polynomial_type &q)
    {
        assert(comp_consistent(q.comp(), *t.order()));
#ifndef NDEBUG
        for (const auto &term : q)
        {
            for (const auto &ve : term.first)
            {
                assert(ve.second >= 0 &&
                       std::find(t.variables().begin(), t.variables().end(), ve.first) !=
                           t.variables().end());
            }
        }
#endif
        if (t.size() == 0)
        {
            // 空链上合法的 q 只能是常数，无需调用多项式分解算法。
            if (q.empty())
                return {{t}, {}};
            return {{}, {t}};
        }
        regular_chain::integer_polynomial z(t.order());
        // 清正分母保持零／非零分类，供原有 ZZ 分解算法使用。
        poly_convert(q, z);
        // 坐标和比较器均沿用 t，保证原算法中的 cls 与链下标一致。
        const auto branches = wrsd(t.integer_equations(), z);
        regular_chain_split result;
        // 两类结果共用装配步骤；zero 表示该列表应满足的标签。
        auto append = [&](const auto &chains, auto &out, bool zero)
        {
            for (const auto &chain : chains)
            {
                assert(chain.size() == t.size());
#ifndef NDEBUG
                // 非零标签用逐次结式非零诊断；零分支只保证零集契约，不能强加伪余式条件。
                if (!zero)
                {
                    auto witness = z;
                    for (size_t i = chain.size(); i-- > 0;)
                    {
                        witness = resultant(witness, chain[i], t.variables()[i]);
                    }
                    assert(!witness.empty());
                }
#endif
                // 分支只保证正则，另计算平方自由属性，再装配为独立的数据对象。
                out.push_back(__make_regular_chain(t, chain, __squarefree_prefixes(t, chain)));
            }
        };
        append(branches.first, result.zero, true);
        append(branches.second, result.nonzero, false);
        // 有效零维正则链有复零点，分解不能丢失全部分支。
        assert(!result.zero.empty() || !result.nonzero.empty());
        return result;
    }

    // WRSD 一般入口：接受一般序的 ZZ/QQ 多项式，检查并转换后调用内部入口。
    // 返回零／非零分支，保留 WRSD 的弱契约。
    template <class Tc,class comp>
    regular_chain_split weak_split_zero(const regular_chain &t, const polynomial_<Tc,comp> &f)
    {
        return __weak_split_zero(t, t.convert(f));
    }

    // XY16 算法 3.1：输入类型已保证零维正则性，此处只补足平方自由性。
    // 返回时输出各链仍正则，其零集之并等于原零集。
    // 已知 RCtoSqrfree 对 [x²,y³,z²+xy] 不终止，待修范围见 RSD 独立说明。
    inline std::vector<regular_chain> squarefree_decomposition(const regular_chain &t)
    {
        if (t.size() == 0 || t.is_squarefree())
        {
            // 已知满足条件时直接返回独立副本，保留全部前缀属性。
            return {t};
        }
        std::vector<regular_chain> out;
        // 原算法使用 ZZ 方程，输出仍按原链的提升坐标排列。
        for (const auto &chain : RCtoSqrfree(t.integer_equations()))
        {
            // Algorithm 3.1 保证输出的每个前缀都平方自由，直接保存该结论。
            const std::vector<bool> flags(t.size(), true);
            // 仅在断言启用时复核判别式；Release 使用算法保证的属性。
            assert(__squarefree_prefixes(t, chain) == flags);
            out.push_back(__make_regular_chain(t, chain, flags));
        }
        assert(!out.empty());
        return out;
    }

}
#endif
