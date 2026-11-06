/**
 * @file rational_sample.hh
 * @brief QQ 开区间中的最简有理数采样
 *
 * 支持有限和无穷端点，使用连分数算法，仅依赖 ZZ/QQ。
 * 规格见 docs/cad/design/rational-sampling.md。
 */
#ifndef CLPOLY_RATIONAL_SAMPLE_HH
#define CLPOLY_RATIONAL_SAMPLE_HH

#include <clpoly/number/QQ.hh>
#include <cassert>
#include <optional>
#include <stdexcept>
#include <vector>

namespace clpoly{

    namespace rational_sample_detail
    {
        // 精确向负无穷取整；QQ 分母为正，不能用向零截断代替。
        inline ZZ floor(const QQ &q)
        {
            assert(q.den() > ZZ(0));
            ZZ result;
            ZZ::fdiv_q(result, q.num(), q.den());
            return result;
        }

        // 非负区间内部算法：要求 0 <= left < right，两个端点都不包含。
        // 保存整数部分并逆序恢复，避免长连分数耗尽递归调用栈。
        // 返回既约分母最小、同分母时正分子最小的严格内点，全部运算使用 ZZ/QQ。
        inline QQ positive(QQ left, QQ right)
        {
            assert(QQ(0) <= left && left < right);
            std::vector<ZZ> parts;
            QQ result;
            while (true)
            {
                assert(QQ(0) <= left && left < right);
                const ZZ k = floor(left);
                // 若有整数内点，最小的可用整数已达到分母 1，可立即结束。
                result = QQ(k + ZZ(1));
                if (result < right)
                {
                    break;
                }

                // 没有整数内点，变换 x -> 1/(x-k)，同时交换上下界。
                parts.push_back(k);
                const QQ fraction = left - QQ(k);
                left = QQ(1) / (right - QQ(k));
                if (fraction.is_zero())
                {
                    // 旧下界恰为整数，新上界为正无穷；不实际计算 1/0。
                    result = QQ(floor(left) + ZZ(1));
                    break;
                }
                right = QQ(1) / fraction;
            }
            for (auto it = parts.rbegin(); it != parts.rend(); ++it)
            {
                // 逆序应用已保存的整数部分，恢复原区间中的有理数。
                result = QQ(*it) + QQ(1) / result;
            }
            return result;
        }
    } // 命名空间 rational_sample_detail

    // 有限开区间采样：分母最小，同分母时分子绝对值最小。
    // 只接受非空开区间；相等端点也拒绝，不能当成闭区间单点。
    // 跨零直接取零；非正区间先反射到非负区间，最终恢复符号。
    inline QQ sample_rational(const QQ &left, const QQ &right)
    {
        if (!(left < right))
        {
            throw std::invalid_argument("sample_rational: empty or reversed open interval");
        }
        if (left.num().sgn() < 0 && right.num().sgn() > 0)
        {
            return QQ(0);
        }
        if (right.num().sgn() <= 0)
        {
            return -rational_sample_detail::positive(-right, -left);
        }
        return rational_sample_detail::positive(left, right);
    }

    // 缺失左/右端点分别表示负/正无穷；存在的端点始终排除。
    // 有限双端点交给上面的重载；无界情形直接取零或绝对值最小的合法整数。
    inline QQ sample_rational(const std::optional<QQ> &left, const std::optional<QQ> &right)
    {
        if (left && right)
        {
            return sample_rational(*left, *right);
        }
        if (!left && !right)
        {
            return QQ(0);
        }
        if (!left)
        {
            if (right->num().sgn() > 0)
            {
                return QQ(0);
            }
            return QQ(-rational_sample_detail::floor(-*right) - ZZ(1));
        }
        if (left->num().sgn() < 0)
        {
            return QQ(0);
        }
        return QQ(rational_sample_detail::floor(*left) + ZZ(1));
    }
}
#endif
