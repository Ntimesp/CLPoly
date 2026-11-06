#include <clpoly/rational_sample.hh>
#include "clpoly_test.hh"
#include <random>

using namespace clpoly;

// 测试数据的分子绝对值不超过 20、分母不超过 10，整数乘积不会溢出。
static int floor_fraction(int numerator, int denominator)
{
    return numerator >= 0 ? numerator / denominator
                          : -((-numerator + denominator - 1) / denominator);
}

// 独立逐分母枚举：端点的中项保证不晚于两端分母之和就能找到内点。
// 不复用生产代码的连分数或取整辅助函数。
static QQ enumerate_sample(int ln, int ld, int rn, int rd)
{
    for (int d = 1; d <= ld + rd; ++d)
    {
        const int first = floor_fraction(ln * d, ld) + 1;
        const int last = -floor_fraction(-rn * d, rd) - 1;
        if (first <= last)
            return QQ(first > 0 ? first : (last < 0 ? last : 0), d);
    }
    throw std::logic_error("enumerate_sample: missing mediant");
}

int main()
{
    CLPOLY_TEST("rational sample: zero and integers");
    // 能选零或整数时分母已达到最小值 1，再按分子绝对值择优。
    {
        CLPOLY_ASSERT_EQ(sample_rational(QQ(-2), QQ(3)), QQ(0));
        CLPOLY_ASSERT_EQ(sample_rational(QQ(1, 2), QQ(7, 2)), QQ(1));
        CLPOLY_ASSERT_EQ(sample_rational(QQ(-7, 2), QQ(-1, 2)), QQ(-1));
        CLPOLY_ASSERT_EQ(sample_rational(QQ(2), QQ(4)), QQ(3));
        CLPOLY_ASSERT_EQ(sample_rational(QQ(-4), QQ(-2)), QQ(-3));
    }

    CLPOLY_TEST("rational sample: excluded endpoints and continued fractions");
    // 严格排除端点；区间内没有整数时走连分数变换，并覆盖负区间的反射。
    {
        CLPOLY_ASSERT_EQ(sample_rational(QQ(0), QQ(1)), QQ(1, 2));
        CLPOLY_ASSERT_EQ(sample_rational(QQ(-1), QQ(0)), QQ(-1, 2));
        CLPOLY_ASSERT_EQ(sample_rational(QQ(0), QQ(1, 3)), QQ(1, 4));
        CLPOLY_ASSERT_EQ(sample_rational(QQ(1, 3), QQ(1, 2)), QQ(2, 5));
        CLPOLY_ASSERT_EQ(sample_rational(QQ(-1, 2), QQ(-1, 3)), QQ(-2, 5));
        CLPOLY_ASSERT_EQ(sample_rational(QQ(7, 5), QQ(29, 20)), QQ(10, 7));
        CLPOLY_ASSERT_EQ(sample_rational(QQ(2), QQ(5, 2)), QQ(7, 3));
    }

    CLPOLY_TEST("rational sample: infinite endpoints");
    // 缺失端点不进入 QQ 运算，无界开区间直接选择合法的最简整数。
    {
        CLPOLY_ASSERT_EQ(sample_rational(std::nullopt, std::nullopt), QQ(0));
        CLPOLY_ASSERT_EQ(sample_rational(std::nullopt, QQ(1, 3)), QQ(0));
        CLPOLY_ASSERT_EQ(sample_rational(std::nullopt, QQ(0)), QQ(-1));
        CLPOLY_ASSERT_EQ(sample_rational(std::nullopt, QQ(-7, 3)), QQ(-3));
        CLPOLY_ASSERT_EQ(sample_rational(std::nullopt, QQ(-3)), QQ(-4));
        CLPOLY_ASSERT_EQ(sample_rational(QQ(-1, 3), std::nullopt), QQ(0));
        CLPOLY_ASSERT_EQ(sample_rational(QQ(0), std::nullopt), QQ(1));
        CLPOLY_ASSERT_EQ(sample_rational(QQ(7, 3), std::nullopt), QQ(3));
        CLPOLY_ASSERT_EQ(sample_rational(QQ(3), std::nullopt), QQ(4));
        const std::optional<QQ> left = QQ(1, 3);
        const std::optional<QQ> right = QQ(1, 2);
        CLPOLY_ASSERT_EQ(sample_rational(left, right), QQ(2, 5));
    }

    CLPOLY_TEST("rational sample: arbitrary precision endpoints");
    // 大分母和长连分数使用任意精度 ZZ，避免依赖固定机器整数范围。
    {
        const ZZ d = ZZ(1) << 200;
        CLPOLY_ASSERT_EQ(sample_rational(QQ(0), QQ(ZZ(1), d)), QQ(ZZ(1), d + ZZ(1)));
        const QQ large(ZZ("1000000000000000000000000000000"));
        CLPOLY_ASSERT_EQ(sample_rational(large, large + QQ(1)), large + QQ(1, 2));
        CLPOLY_ASSERT_EQ(sample_rational(-large - QQ(1), -large), -large - QQ(1, 2));

        // 相邻 Fibonacci 比值之间，最小分母的内点是它们的中项。
        ZZ previous(1), current(1);
        for (size_t i = 0; i < 400; ++i)
        {
            const ZZ next = previous + current;
            previous = current;
            current = next;
        }
        const QQ left(current, previous);
        const QQ right(current + previous, current);
        CLPOLY_ASSERT_EQ(sample_rational(left, right),
                         QQ(current + current + previous, current + previous));
    }

    CLPOLY_TEST("rational sample: empty open intervals rejected");
    // 相等端点同样没有严格内点，应与倒置端点一起拒绝。
    {
        for (const auto &right : {QQ(1), QQ(0)})
        {
            bool rejected = false;
            try
            {
                sample_rational(QQ(1), right);
            }
            catch (const std::invalid_argument &)
            {
                rejected = true;
            }
            CLPOLY_ASSERT(rejected);
            rejected = false;
            try
            {
                sample_rational(std::optional<QQ>(QQ(1)), std::optional<QQ>(right));
            }
            catch (const std::invalid_argument &)
            {
                rejected = true;
            }
            CLPOLY_ASSERT(rejected);
        }
    }
    CLPOLY_TEST("rational sample: independent denominator enumeration");
    {
        std::mt19937 random(20261106);
        size_t checked = 0;
        while (checked < 1000)
        {
            int ln = static_cast<int>(random() % 41) - 20, ld = random() % 10 + 1;
            int rn = static_cast<int>(random() % 41) - 20, rd = random() % 10 + 1;
            if (ln * rd == rn * ld)
                continue;
            if (ln * rd > rn * ld)
            {
                std::swap(ln, rn);
                std::swap(ld, rd);
            }
            const QQ left(ln, ld), right(rn, rd);
            const QQ q = sample_rational(left, right);
            CLPOLY_ASSERT(left < q && q < right);
            CLPOLY_ASSERT_EQ(q, enumerate_sample(ln, ld, rn, rd));
            ++checked;
        }
    }
    return clpoly_test::test_summary();
}
