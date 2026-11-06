/**
 * @file real_algebraic.hh
 * @brief 基于 QQ 三角列与严格有理根盒的实代数点
 *
 * 隔离、精化与点查询依据 XZ06，三角分解复用 XY16。
 * 规格见 docs/cad/design/modular-cad/real-algebraic.md。
 */
#ifndef CLPOLY_REAL_ALGEBRAIC_HH
#define CLPOLY_REAL_ALGEBRAIC_HH

#include <clpoly/regular_chain.hh>
#include <clpoly/polynomial_interval.hh>
#include <clpoly/realroot.hh>

namespace clpoly{

    namespace xz_detail
    {
        // P 是实际定义方程；U 仅表示有理系数的一元界多项式，不能替代定义三角列。
        using P = regular_chain::polynomial_type;
        using U = std::vector<QQ>; // 按次数递增存放的稠密 QQ 系数。
        using I = rational_interval;
        // 提升坐标必须是目标比较器的反向连续后缀。
        inline bool compatible_coordinates(const std::vector<variable> &vars,
                                           const regular_chain::order_type *order)
        {
            if (!order || vars.size() > order->comp.v_map.size())
                return false;
            for (size_t i = 0; i < vars.size(); ++i)
            {
                const auto it = order->comp.v_map.find(vars[i]);
                if (it == order->comp.v_map.end() ||
                    it->second != static_cast<int64_t>(order->comp.v_map.size() - i))
                    return false;
            }
            return true;
        }

        // 有理点上的稠密 Horner 求值；空系数表表示零多项式。
        inline QQ eval(const U &f, const QQ &x)
        {
            QQ v(0);
            for (size_t j = f.size(); j-- > 0;)
            {
                v = v * x + f[j];
            }
            return v;
        }

        // 同一表达式的精确区间包络；包络含零并不意味着存在零点。
        inline I eval(const U &f, const I &x)
        {
            I v;
            for (size_t j = f.size(); j-- > 0;)
            {
                v = v * x + I(f[j]);
            }
            return v;
        }

        // 一元形式求导，输出仍按次数递增排列；常数的导数为空表。
        inline U derivative_u(const U &f)
        {
            U d(f.empty() ? 0 : f.size() - 1);
            for (size_t j = 1; j < f.size(); ++j)
            {
                d[j - 1] = f[j] * QQ(ZZ(j));
            }
            return d;
        }

        // 按 v 的指数收集 QQ 系数多项式，c[j] 对应 v^j；缺失次数保留零项。
        // 各系数不含 v，仍借用 f 的比较器；输入指数须非负且多项式已规范化。
        inline std::vector<P> coefficients(const P &f, const variable &v)
        {
            assert(f.empty() || is_number(f) || get_first_var(f) == v ||
                   f.comp()(v, get_first_var(f)));
            std::vector<P> c(static_cast<size_t>(std::max<int64_t>(0, degree(f, v))) + 1, P(f.comp_ptr()));
            for (const auto &term : f)
            {
                auto d = term.first.deg(v);
                basic_monomial<regular_chain::order_type> power(f.comp_ptr());
                if (d)
                {
                    power = {{v, d}};
                }
                c[d].push_back({d ? term.first / power : term.first, term.second});
            }
            // v 的优先级高于系数变量；各次数组本来连续且有序。
            // 删除相同的 v 幂不会引入同类项，无需重新排序。
#ifndef NDEBUG
            for (const auto &coefficient : c)
                assert(coefficient.is_normal());
#endif
            return c;
        }

        // 展开 f(...,v+offset)；只做 QQ 多项式运算，返回结果借用 f 的比较器。
        inline P translate(const P &f, const variable &v, const QQ &offset)
        {
            auto c = coefficients(f, v);
            P result(f.comp_ptr()), linear(f.comp_ptr());
            basic_monomial<regular_chain::order_type> vm(f.comp_ptr()), one(f.comp_ptr());
            vm = {{v, 1}};
            linear.push_back({vm, QQ(1)});
            if (offset != QQ(0))
            {
                linear.push_back({one, offset});
            }
            for (size_t i = c.size(); i-- > 0;)
            {
                result = result * linear + c[i];
            }
            return result;
        }

        // vars、box 是已认证的提升前缀，f 只含前缀变量和新的主变量 v。
        // 返回有理一元下界／上界的系数表；对 v>0 同时夹住多项式及各阶导数。
        // 调用方先把全部根平移到正半轴，再使用该性质。
        inline std::pair<U, U> bounds(const P &f, const variable &v, const std::vector<variable> &vars,
                                      const std::vector<I> &box)
        {
            auto c = coefficients(f, v);
            U l(c.size()), u(c.size());
            // vars 与根盒均按提升顺序存放；同步反转为比较器的投影优先级。
            // 不能只反转变量表，否则区间会绑定到错误的变量。
            const std::vector<variable> ordered_vars(vars.rbegin(), vars.rend());
            const std::vector<I> ordered_box(box.rbegin(), box.rend());
            for (size_t j = 0; j < c.size(); ++j)
            {
                auto b = __assign_interval(c[j], ordered_vars, ordered_box);
                l[j] = b.left();
                u[j] = b.right();
            }
            return {l, u};
        }

        // 清除全体系数的正分母，转为原有稀疏 ZZ 一元多项式。
        // 乘以正常数保持根集、重数和符号；不保持原多项式值。
        inline upolynomial_ZZ integer(const U &f)
        {
            ZZ denominator(1);
            for (const auto &q : f)
            {
                denominator = (denominator / gcd(denominator, q.den())) * q.den();
            }
            upolynomial_ZZ z;
            for (size_t j = f.size(); j-- > 0;)
            {
                if (f[j] != QQ(0))
                {
                    z.push_back({umonomial(j), f[j].num() * (denominator / f[j].den())});
                }
            }
            return z;
        }

        // 判别一元多项式是否无重根：gcd(f,f') 必须为非零常数。
        // 零返回 false，非零常数返回 true；使用二参数 GCD，不计算 Bézout 系数。
        inline bool squarefree(const U &f)
        {
            auto z = integer(f);
            if (z.empty())
            {
                return false;
            }
            if (z.front().first.deg() == 0)
            {
                return true;
            }
            auto g = polynomial_GCD(z, derivative(z));
            return !g.empty() && g.front().first.deg() == 0;
        }

        // 区间符号中的 unknown 和零均不是严格符号，只接受 -1 或 1。
        inline bool strict_sign(int s)
        {
            return s == -1 || s == 1;
        }

        // 精化包含唯一单根的严格括区间。中点恰为根时，在其两侧保留更小的
        // 严格区间，避免把精化证书退化成单点。
        // 调用者须保证唯一性、正精度和端点异号；断言仅用于诊断。
        inline I bracket(const U &f, I b, const QQ &epsilon)
        {
            assert(epsilon > QQ(0));
            int left = sgn(eval(f, b.left())), right = sgn(eval(f, b.right()));
            assert(left && right && left != right);
            while (b.width() > epsilon)
            {
                const QQ m = b.midpoint();
                const int middle = sgn(eval(f, m));
                if (!middle)
                {
                    // 新区间仍位于旧括区间内部，已知的唯一根继续被严格包含。
                    const QQ delta = std::min(b.width() / QQ(4), epsilon / QQ(4));
                    b = I(m - delta, m + delta);
                }
                else if (middle == left)
                {
                    b = I(m, b.right());
                }
                else
                {
                    b = I(b.left(), m);
                }
            }
            return b;
        }

        // PDF 算法 11（UNI-QQ）：适配既有非零根隔离器，返回全部根的有序列表，
        // 每个根都有严格括区间和导数符号证书。
        // 要求 epsilon>0、f 非零且平方自由；非零常数返回空根表。
        inline std::vector<I> __univariate(const U &f, const QQ &epsilon)
        {
            assert(epsilon > QQ(0));
            auto z = integer(f);
            assert(!z.empty());
            if (z.front().first.deg() == 0)
            {
                return {};
            }
            assert(squarefree(f));
            std::vector<I> out;
            const U df = derivative_u(f);
            // 已审核的标量 Uspensky 接口只返回非零根；先补入可能存在的简单零根，
            // 再统一转换成严格括区间。
            auto roots = uspensky(z);
            if (f.front() == QQ(0))
            {
                roots.emplace_back(QQ(0), QQ(0));
            }
            for (auto endpoints : roots)
            {
                I b(endpoints.first, endpoints.second);
                // Uspensky 可以返回精确有理根或端点根，先扩为严格的异号括区间。
                if (eval(f, b.left()) == QQ(0) || eval(f, b.right()) == QQ(0))
                {
                    const QQ root = eval(f, b.left()) == QQ(0) ? b.left() : b.right();
                    QQ delta = epsilon / QQ(4);
                    do
                    {
                        b = I(root - delta, root + delta);
                        delta /= QQ(2);
                    } while (!strict_sign(eval(df, b).sgn()) ||
                             sgn(eval(f, b.left())) * sgn(eval(f, b.right())) != -1);
                }
                b = bracket(f, b, epsilon);
                while (!strict_sign(eval(df, b).sgn()))
                {
                    b = bracket(f, b, b.width() / QQ(2));
                }
                out.push_back(std::move(b));
            }
            std::sort(out.begin(), out.end(), [](const I &a, const I &b) { return a.left() < b.left(); });
            // 宽度条件不足以区分相邻根，还需把各严格括区间收缩至互不接触。
            for (size_t j = 1; j < out.size(); ++j)
            {
                while (out[j - 1].overlaps(out[j]))
                {
                    out[j - 1] = bracket(f, out[j - 1], out[j - 1].width() / QQ(2));
                    out[j] = bracket(f, out[j], out[j].width() / QQ(2));
                }
            }
            return out;
        }

        // 有检查的适配入口，供独立一元调用使用。
        // XZ-LIFT 已认证平方自由，直接使用上面的内部入口。
        inline std::vector<I> univariate(const U &f, const QQ &epsilon)
        {
            if (epsilon <= QQ(0))
                throw std::invalid_argument("UNI: nonpositive precision");
            if (integer(f).empty())
                throw std::domain_error("UNI: zero polynomial");
            if (!squarefree(f))
                throw std::invalid_argument("UNI: squarefree polynomial required");
            return __univariate(f, epsilon);
        }

        // 正半轴上，两界导数严格同号、端点严格异号，保证前缀盒中每个点的
        // 专门化在该区间内恰有一个根。
        // b 是同一 QQ 表达式的上下界，j 必须处于正半轴；这里只检查证书，不精化。
        inline bool certified(const std::pair<U, U> &b, const I &j)
        {
            const int dl = eval(derivative_u(b.first), j).sgn(), du = eval(derivative_u(b.second), j).sgn();
            return strict_sign(dl) && dl == du && sgn(eval(b.first, j.left())) == -dl &&
                   sgn(eval(b.second, j.left())) == -dl && sgn(eval(b.first, j.right())) == dl &&
                   sgn(eval(b.second, j.right())) == dl;
        }
    } // 命名空间 xz_detail

    // 实点保存平方自由正则链、严格 QQ 根盒和固定平移。公开构造验证证书，
    // 根隔离与精化由独立函数实现。
    class real_algebraic_point
    {
        regular_chain _chain; // 平方自由正则定义链，借用调用者保持存活的比较器。
        std::vector<rational_interval> _box; // 原坐标严格隔离盒，与提升坐标一一对应。
        std::vector<QQ> _shifts; // 每层固定平移；_box[i]+_shifts[i] 位于正半轴。

    public:
        // 零坐标的唯一空点，作为逐层提升及无变量公式的样本。
        real_algebraic_point() = default;

        // 公开构造：验证整条链、严格根盒及所有前缀上的上下界证书。
        // shifts 为空时逐层选取正半轴平移；显式提供时沿用并验证该平移。
        // 宽盒即使包含目标根，也可能不满足证书，非法输入抛 invalid_argument。
        real_algebraic_point(regular_chain chain, std::vector<rational_interval> box,
                             std::vector<QQ> shifts = {})
            : _chain(std::move(chain)), _box(std::move(box))
        {
            using namespace xz_detail;
            if (!_chain.is_squarefree() || _chain.size() != _box.size() ||
                (!shifts.empty() && shifts.size() != _box.size()))
            {
                throw std::invalid_argument(
                    "real_algebraic_point: requires squarefree regular chain and matching box");
            }
            for (size_t i = 0; i < _box.size(); ++i)
            {
                if (_box[i].is_point())
                {
                    throw std::invalid_argument("real_algebraic_point: strict brackets required");
                }
                QQ shift = shifts.empty() ? (_box[i].left() > QQ(0) ? QQ(0) : QQ(1) - _box[i].left())
                                          : shifts[i];
                if (_box[i].left() + shift <= QQ(0))
                {
                    throw std::invalid_argument("real_algebraic_point: nonpositive shifted bracket");
                }

                // 令新坐标 t=v+shift，以 f(...,t-shift) 在正半轴上构造界多项式。
                auto f = translate(_chain.equations()[i], _chain.variables()[i], -shift);
                auto b = bounds(
                    f, _chain.variables()[i],
                    std::vector<variable>(_chain.variables().begin(), _chain.variables().begin() + i),
                    std::vector<I>(_box.begin(), _box.begin() + i));
                if (!certified(b, _box[i] + I(shift)))
                {
                    throw std::invalid_argument("real_algebraic_point: uncertified root box");
                }
                _shifts.push_back(shift);
            }
        }

        // 只读借用定义链；求值和精化不能把界多项式替换为定义方程。
        const regular_chain &chain() const
        {
            return _chain;
        }

        // 只读借用原坐标隔离盒；下标对应 chain().variables() 的提升顺序。
        const std::vector<rational_interval> &box() const
        {
            return _box;
        }

        // 只读借用已固定的平移；后续精化使用同一组平移保持证书条件。
        const std::vector<QQ> &shifts() const
        {
            return _shifts;
        }

        // 实点的坐标数，等于定义链长和根盒维数。
        size_t size() const
        {
            return _box.size();
        }

        // 各维区间宽度的最大值；空点为零，用于控制整体精化精度。
        QQ max_width() const
        {
            QQ m(0);
            for (const auto &b : _box)
            {
                m = std::max(m, b.width());
            }
            return m;
        }

        // 截取前 n 个提升坐标及其已认证证书；n 越界时抛 out_of_range。
        real_algebraic_point prefix(size_t n) const
        {
            if (n > size())
            {
                throw std::out_of_range("real_algebraic_point: prefix");
            }

            // 前缀继承已经验证的证书，只截取数据，不重新平移或认证。
            real_algebraic_point result;
            result._chain = _chain.prefix(n);
            result._box.assign(_box.begin(), _box.begin() + n);
            result._shifts.assign(_shifts.begin(), _shifts.begin() + n);
            return result;
        }

        // 成员函数仅转发，算法定义在类型之外。
        void shrink();
        void refine(const QQ &epsilon);
        friend void shrink(real_algebraic_point &a);
        friend std::vector<real_algebraic_point> __lift_real_points(const regular_chain &chain,
                                                                    real_algebraic_point prefix);
    };

    // PDF 算法 2（NSHR-QQ）：从第一坐标向上精化，每层使用已经缩小的前缀
    // 和原有固定平移。定义链与所选实点保持不变。
    inline void shrink(real_algebraic_point &a)
    {
        using namespace xz_detail;
        for (size_t i = 0; i < a.size(); ++i)
        {
            auto q = translate(a._chain.equations()[i], a._chain.variables()[i], -a._shifts[i]);
            auto b = bounds(
                q, a._chain.variables()[i],
                std::vector<variable>(a._chain.variables().begin(), a._chain.variables().begin() + i),
                std::vector<I>(a._box.begin(), a._box.begin() + i));
            auto old = a._box[i] + I(a._shifts[i]);
            // 首层两界相同，宽度至多减半；高层各界根缩至旧宽的 1/8 后再取并包络。
            QQ epsilon = old.width() / QQ(i ? 8 : 2);
            auto l = bracket(b.first, old, epsilon), u = bracket(b.second, old, epsilon);
            a._box[i] = hull(l, u) - I(a._shifts[i]);
        }
    }

    // PDF 算法 3（REFINE）：将每维宽度缩至严格小于目标精度。
    // 终止性依赖 NSHR-QQ 的收敛证明，不设置固定迭代次数。
    inline void __refine(real_algebraic_point &a, const QQ &epsilon)
    {
        assert(epsilon > QQ(0));
        while (a.max_width() >= epsilon)
            shrink(a);
    }

    inline void refine(real_algebraic_point &a, const QQ &epsilon)
    {
        if (epsilon <= QQ(0))
        {
            throw std::invalid_argument("REFINE: nonpositive precision");
        }
        __refine(a, epsilon);
    }

    // 便捷成员只转发到自由函数，精化算法仍位于数据类型之外。
    inline void real_algebraic_point::shrink()
    {
        clpoly::shrink(*this);
    }
    inline void real_algebraic_point::refine(const QQ &epsilon)
    {
        clpoly::refine(*this, epsilon);
    }

    namespace xz_detail
    {
        // PDF 算法 1（XZ-LIFT）：输入首项在指定点上非零。精化前缀，直到上下界
        // 认证全部根；后续 NSHR-QQ 保持 used_shift 不变。全过程只构造 QQ 多项式。
        // 调用者还保证在所选前缀处的一元特化平方自由，v 是新增主变量。
        // 返回按末坐标递增的原坐标严格根区间；a 可被精化，used_shift 返回成功使用的平移。
        inline std::vector<I> lift(const P &f, const variable &v, real_algebraic_point &a, QQ &used_shift)
        {
            const auto &vars = a.chain().variables();
            auto cs = coefficients(f, v);
            if (cs.size() < 2)
            {
                return {};
            }
            const std::vector<variable> ordered_vars(vars.rbegin(), vars.rend());
            std::vector<I> ordered_box;
            I leading;
            // 用精确区间把非零首项与零分离，得到统一的根界分母下界。
            while (true)
            {
                ordered_box.assign(a.box().rbegin(), a.box().rend());
                leading = __assign_interval(cs.back(), ordered_vars, ordered_box);
                if (!leading.contains_zero())
                {
                    break;
                }
                assert(a.size() != 0); // 空前缀的非零标量首项不可能包含零。
                __refine(a, a.max_width() / QQ(2));
            }
            QQ m = std::min(abs(leading.left()), abs(leading.right())), maximum(0);
            for (size_t i = 0; i + 1 < cs.size(); ++i)
            {
                auto c = __assign_interval(cs[i], ordered_vars, ordered_box);
                maximum = std::max(maximum, std::max(abs(c.left()), abs(c.right())));
            }
            const QQ shift = QQ(2) + maximum / m;
            // 严格 Cauchy 根界加余量，使全部根平移后严格落在正半轴。
            used_shift = shift;
            const auto q = translate(f, v, -shift);
            QQ epsilon(1);
            while (true)
            {
                epsilon /= QQ(2);
                auto b = bounds(q, v, vars, a.box());
                bool accept = b.first.back() != QQ(0) && sgn(b.first.back()) == sgn(b.second.back()) &&
                              squarefree(b.first) && squarefree(b.second);
                std::vector<I> intervals;
                if (accept)
                {
                    auto l = __univariate(b.first, epsilon), u = __univariate(b.second, epsilon);
                    // 两界必须给出相同数目的全部实根，再按根序号逐一配对。
                    accept = l.size() == u.size();
                    if (accept)
                    {
                        for (size_t j = 0; j < l.size(); ++j)
                        {
                            auto joint = hull(l[j], u[j]);
                            if (joint.left() <= QQ(0) || !certified(b, joint) ||
                                (!intervals.empty() && intervals.back().right() >= joint.left()))
                            {
                                accept = false;
                                break;
                            }
                            intervals.push_back(joint);
                        }
                    }
                }
                if (accept)
                {
                    // 保存原坐标区间；固定平移由上层与根盒一同写入证书。
                    for (auto &j : intervals)
                    {
                        j -= I(shift);
                    }
                    return intervals;
                }
                if (a.size())
                {
                    // 缩小前缀估值误差；下一轮同时提高一元括区间精度。
                    __refine(a, a.max_width() / QQ(2));
                }
            }
        }
    } // 命名空间 xz_detail

    // XZ06 NREALZERO Step 2：只在一个已经认证的前缀上提升最后坐标。
    // 前置条件：chain 平方自由正则，prefix 的定义方程是 chain 的低一维前缀。
    // 返回所有末层实根对应的新点；不枚举其他前缀，按已有证书直接组合内部数据。
    inline std::vector<real_algebraic_point> __lift_real_points(const regular_chain &chain,
                                                                real_algebraic_point prefix)
    {
        assert(chain.is_squarefree());
        assert(chain.size() == prefix.size() + 1);
        assert(std::equal(prefix.chain().variables().begin(), prefix.chain().variables().end(),
                          chain.variables().begin()));
        assert(std::equal(prefix.chain().equations().begin(), prefix.chain().equations().end(),
                          chain.equations().begin()));
        QQ shift;
        const auto intervals =
            xz_detail::lift(chain.equations().back(), chain.variables().back(), prefix, shift);
        std::vector<real_algebraic_point> points;
        for (const auto &interval : intervals)
        {
            // XZ-LIFT 已认证新层；旧前缀由输入证书保证，不重复验证全部旧层。
            real_algebraic_point point;
            point._chain = chain;
            point._box = prefix.box();
            point._box.push_back(interval);
            point._shifts = prefix.shifts();
            point._shifts.push_back(shift);
            points.push_back(std::move(point));
        }
        return points;
    }

    // PDF 算法 4：对每个既有实前缀逐层调用单前缀提升，得到整条链的全部实点。
    // 输入必须平方自由；无实根返回空表，空链返回一个空点。
    inline std::vector<real_algebraic_point> __isolate_real_points(const regular_chain &t)
    {
        assert(t.is_squarefree());
        std::vector<real_algebraic_point> points(1);
        for (size_t i = 0; i < t.size(); ++i)
        {
            const auto chain = t.prefix(i + 1);
            std::vector<real_algebraic_point> next;
            for (auto &prefix : points)
            {
                auto extensions = __lift_real_points(chain, std::move(prefix));
                for (auto &point : extensions)
                {
                    next.push_back(std::move(point));
                }
            }
            points = std::move(next);
        }
        return points;
    }

    inline std::vector<real_algebraic_point> isolate_real_points(const regular_chain &t)
    {
        if (!t.is_squarefree())
            throw std::invalid_argument("NREALZERO: nonsquarefree chain");
        return __isolate_real_points(t);
    }

    // PDF 算法 5：WRSD 返回零／非零两类复分支，用区间排除错误分支，直到仅剩一种标签。
    // 标签直接采用零集划分，不要求零分支上的逐次伪余式为零。
    // 余式非零本身不能判定某个指定共轭点上的值非零。
    // 接受本点变量上的一般序 ZZ/QQ 多项式；可精化 a，但不改变其定义或选中的实点。
    inline bool __point_zero(const regular_chain::polynomial_type &q, real_algebraic_point &a)
    {
        auto branches = __weak_split_zero(a.chain(), q);
        const std::vector<variable> ordered_vars(a.chain().variables().rbegin(),
                                                 a.chain().variables().rend());
        std::vector<rational_interval> ordered_box;
        // 只有某条定义方程的整个区间估值排除零，才能删去该分支。
        auto eliminate = [&](std::vector<regular_chain> &list)
        {
            list.erase(std::remove_if(
                           list.begin(), list.end(),
                           [&](const regular_chain &t)
                           {
                               for (const auto &g : t.equations())
                               {
                                   if (!__assign_interval(g, ordered_vars, ordered_box).contains_zero())
                                   {
                                       return true;
                                   }
                               }
                               return false;
                           }),
                       list.end());
        };
        while (true)
        {
            // 两类分支共用同一个持续收缩的原点根盒，不从盒相交推断分支归属。
            ordered_box.assign(a.box().rbegin(), a.box().rend());
            eliminate(branches.zero);
            eliminate(branches.nonzero);
            assert(!branches.zero.empty() || !branches.nonzero.empty());
            if (branches.nonzero.empty())
            {
                return true;
            }
            if (branches.zero.empty())
            {
                return false;
            }
            __refine(a, a.max_width() / QQ(2));
        }
    }

    template <class Tc,class comp>
    bool point_zero(const polynomial_<Tc,comp> &f, real_algebraic_point &a)
    {
        return __point_zero(a.chain().convert(f), a);
    }

    // PDF 算法 6：严格区间符号直接返回；不确定时精确判零，再精化非零点值。
    // 返回 -1、0 或 1；区间符号 unknown 仅表示需要继续判定，不能作为返回值。
    inline int __point_sign(const regular_chain::polynomial_type &q, real_algebraic_point &a)
    {
        assert(comp_consistent(q.comp(), *a.chain().order()));
        const std::vector<variable> ordered_vars(a.chain().variables().rbegin(),
                                                 a.chain().variables().rend());
        auto interval_sign = [&]()
        {
            const std::vector<rational_interval> ordered_box(a.box().rbegin(), a.box().rend());
            return __assign_interval(q, ordered_vars, ordered_box).sgn();
        };
        int sign = interval_sign();
        if (xz_detail::strict_sign(sign))
        {
            return sign;
        }
        if (__point_zero(q, a))
        {
            return 0;
        }
        while (true)
        {
            sign = interval_sign();
            if (xz_detail::strict_sign(sign))
            {
                return sign;
            }
            __refine(a, a.max_width() / QQ(2));
        }
    }

    template <class Tc,class comp>
    int point_sign(const polynomial_<Tc,comp> &f, real_algebraic_point &a)
    {
        return __point_sign(a.chain().convert(f), a);
    }

    // 在同一提升坐标表上，逐方程精确判断指定实点是否属于 t 的零集。
    // t 可有其他共轭根；a 可被精化，坐标表不同则抛 invalid_argument。
    inline bool point_on_chain(const regular_chain &t, real_algebraic_point &a)
    {
        if (t.variables() != a.chain().variables())
        {
            throw std::invalid_argument("point membership: different coordinates");
        }
        for (const auto &f : t.equations())
        {
            regular_chain::polynomial_type q(a.chain().order());
            const bool same_order = comp_consistent(f.comp(), *a.chain().order());
            if (!same_order)
                poly_convert(f, q);
            if (!__point_zero(same_order ? f : q, a))
            {
                return false;
            }
        }
        return true;
    }

    // 比较两个证书是否表示同一个实点，a 按值传入以免修改调用方。
    // 先验证属于 b 的定义链，再用严格包含／分离区分 b 的各个共轭根。
    inline bool same_real_point(real_algebraic_point a, const real_algebraic_point &b)
    {
        if (!point_on_chain(b.chain(), a))
        {
            return false;
        }
        while (true)
        {
            bool inside = true;
            for (size_t j = 0; j < a.size(); ++j)
            {
                if (!a.box()[j].overlaps(b.box()[j]))
                {
                    return false;
                }
                if (!b.box()[j].interior_contains(a.box()[j]))
                {
                    inside = false;
                }
            }
            if (inside)
            {
                return true;
            }
            __refine(a, a.max_width() / QQ(2));
        }
    }

    // PDF 算法 9：按维数递归重编码同一个实点，不枚举其他实前缀。
    // 新链须含该实点，但可含更多其他根、可带重根；内部平方自由化后选回原点。
    inline real_algebraic_point reencode_point(real_algebraic_point a, const regular_chain &chain)
    {
        if (chain.variables() == a.chain().variables() && chain.equations() == a.chain().equations())
        {
            return a;
        }
        if (!point_on_chain(chain, a))
        {
            throw std::invalid_argument("REENCODE: point is not on component");
        }
        if (chain.size() == 0)
        {
            return a;
        }
        for (const auto &component : squarefree_decomposition(chain))
        {
            if (!chain.is_squarefree() && !point_on_chain(component, a))
            {
                continue;
            }

            // 每次先少一维重编码，再仅提升末层，构成按维数下降的递归。
            auto prefix = reencode_point(a.prefix(a.size() - 1), component.prefix(a.size() - 1));
            for (auto candidate : __lift_real_points(component, std::move(prefix)))
            {
                if (same_real_point(candidate, a))
                {
                    return candidate;
                }
            }
        }
        throw std::logic_error("REENCODE: lost selected point");
    }

    // 在已有点后追加新坐标 v=value，用 QQ 线性方程和严格根区间表示。
    // v 必须未出现；order 由调用者保持存活，其末尾变量覆盖扩展后的提升坐标。
    // 新增方程的首项和导数均为 1，因此保持正则平方自由性。
    inline real_algebraic_point append_rational(const real_algebraic_point &a, const variable &v,
                                                const QQ &value,
                                                const regular_chain::order_type *order)
    {
        using P = regular_chain::polynomial_type;
        auto vars = a.chain().variables();
        if (std::find(vars.begin(), vars.end(), v) != vars.end())
        {
            throw std::invalid_argument("append_rational: repeated variable");
        }
        vars.push_back(v);
        if (!xz_detail::compatible_coordinates(vars, order))
        {
            throw std::invalid_argument("append_rational: incompatible coordinate order");
        }
        P linear(order);
        basic_monomial<regular_chain::order_type> m(order), one(order);
        m = {{v, 1}};
        linear.push_back({m, QQ(1)});
        if (value != QQ(0))
        {
            linear.push_back({one, -value});
        }
        auto equations = a.chain().equations();
        equations.push_back(linear);
        auto box = a.box();
        box.emplace_back(value - QQ(1), value + QQ(1));
        auto shifts = a.shifts();
        shifts.push_back(box.back().left() > QQ(0) ? QQ(0) : QQ(1) - box.back().left());
        return real_algebraic_point(regular_chain(std::move(vars), equations, order), std::move(box),
                                    std::move(shifts));
    }

    // PDF 算法 10：WRSD 选择指定点上的次数分支，平方自由化后仅提升所选实前缀。
    // v 是新增变量；返回全部互异实根，按末坐标递增排列，输入点按值复制。
    // order 由调用者保持存活，返回实点借用此序；不得传入函数内的临时比较器。
    // 指定点上的零多项式抛 domain_error，非零常数返回空根表。
    inline std::vector<real_algebraic_point> __roots_over(
        const regular_chain::polynomial_type &f, real_algebraic_point a, const variable &v)
    {
        using namespace xz_detail;
        const auto *order = f.comp_ptr();
        auto vars = a.chain().variables();
        assert(std::find(vars.begin(), vars.end(), v) == vars.end());
        vars.push_back(v);
        assert(compatible_coordinates(vars, order));
#ifndef NDEBUG
        for (const auto &term : f)
            for (const auto &ve : term.first)
                assert(ve.second >= 0 && std::find(vars.begin(), vars.end(), ve.first) != vars.end());
#endif
        auto cs = coefficients(f, v);
        auto selected = a.chain();
        int64_t degree_at_point = -1;
        // 从最高系数向下选分支；直到首个在指定点处非零的系数确定实际次数。
        for (size_t j = cs.size(); j-- > 0;)
        {
            // 常用的同序路径直接使用字典序内核；不同外部比较器只换序，不重复校验。
            regular_chain_split branches;
            if (comp_consistent(cs[j].comp(), *selected.order()))
                branches = __weak_split_zero(selected, cs[j]);
            else
            {
                P coefficient(selected.order());
                poly_convert(cs[j], coefficient);
                branches = __weak_split_zero(selected, coefficient);
            }
            bool found = false;
            for (const auto &t : branches.nonzero)
            {
                if (point_on_chain(t, a))
                {
                    selected = t;
                    degree_at_point = static_cast<int64_t>(j);
                    found = true;
                    break;
                }
            }
            if (found)
            {
                break;
            }
            for (const auto &t : branches.zero)
            {
                if (point_on_chain(t, a))
                {
                    selected = t;
                    found = true;
                    break;
                }
            }
            assert(found); // WRSD 的覆盖契约保证所选点始终存在。
        }
        if (degree_at_point < 0)
        {
            throw std::domain_error("ROOTS-QQ: identically zero specialization");
        }
        if (degree_at_point == 0)
        {
            return {};
        }
        P truncated(order);
        // 更高次项在选定分支上恒为零，删除后首项在整个分支上非零。
        for (const auto &term : f)
        {
            if (term.first.deg(v) <= degree_at_point)
            {
                truncated.push_back(term);
            }
        }
        auto equations = selected.equations();
        equations.push_back(truncated);
        regular_chain extended(vars, equations, order);
        std::vector<real_algebraic_point> result;
        for (const auto &t : squarefree_decomposition(extended))
        {
            const auto prefix_chain = t.prefix(a.size());
            if (!point_on_chain(prefix_chain, a))
            {
                continue;
            }
            auto prefix = reencode_point(a, prefix_chain);
            // 只提升与原点相同的前缀；其他前缀不属于本次纤维。
            for (auto candidate : __lift_real_points(t, std::move(prefix)))
            {
                bool duplicate = false;
                // 不同平方自由分支可能给出同一个根，须精确去重，再持久化分离互异根。
                for (auto &previous : result)
                {
                    if (!candidate.box().back().overlaps(previous.box().back()))
                    {
                        continue;
                    }
                    if (same_real_point(candidate, previous))
                    {
                        duplicate = true;
                        break;
                    }

                    // 精化结果保留在根表中；不同分支的互异根最终严格分离。
                    while (candidate.box().back().overlaps(previous.box().back()))
                    {
                        __refine(candidate, candidate.max_width() / QQ(2));
                        __refine(previous, previous.max_width() / QQ(2));
                    }
                }
                if (!duplicate)
                {
                    result.push_back(std::move(candidate));
                }
            }
        }

        // 合并循环已保证末坐标区间两两不相交，比较器只读端点，不求解或复制实点。
        std::sort(result.begin(), result.end(),
                  [](const real_algebraic_point &left, const real_algebraic_point &right)
                  { return left.box().back().left() < right.box().back().left(); });
        return result;
    }
    // 一般序入口：先校验坐标与指数，再转换到目标 QQ 字典序。
    template <class Tc,class comp>
    std::vector<real_algebraic_point> roots_over(const polynomial_<Tc,comp> &input,
                                                real_algebraic_point a, const variable &v,
                                                const regular_chain::order_type *order)
    {
        auto vars = a.chain().variables();
        if (std::find(vars.begin(), vars.end(), v) != vars.end())
            throw std::invalid_argument("ROOTS-QQ: repeated variable");
        vars.push_back(v);
        if (!xz_detail::compatible_coordinates(vars, order))
            throw std::invalid_argument("ROOTS-QQ: incompatible coordinate order");
        for (const auto &term : input)
            for (const auto &ve : term.first)
                if (ve.second < 0 || std::find(vars.begin(), vars.end(), ve.first) == vars.end())
                    throw std::invalid_argument("ROOTS-QQ: undeclared variable");
        regular_chain::polynomial_type f(order);
        poly_convert(input, f);
        return __roots_over(f, std::move(a), v);
    }

}
#endif
