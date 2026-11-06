// 小规模分母枚举验证，以及 054 固定纤维的采样阶段对照。
#include <clpoly/rational_sample.hh>
#include <clpoly/real_algebraic.hh>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <utility>

using namespace clpoly;

// 生成数据范围有限，以下整数乘积不超过 40000。
struct fraction
{
    int numerator;
    int denominator;
};

static int floor_fraction(int numerator, int denominator)
{
    if (numerator >= 0)
    {
        return numerator / denominator;
    }
    return -((-numerator + denominator - 1) / denominator);
}

static QQ enumerate_denominators(const fraction &left, const fraction &right)
{
    // 两端点的中项严格位于其间，故最小分母不超过端点分母之和。
    for (int d = 1; d <= left.denominator + right.denominator; ++d)
    {
        const int first = floor_fraction(left.numerator * d, left.denominator) + 1;
        const int last = -floor_fraction(-right.numerator * d, right.denominator) - 1;
        if (first <= last)
        {
            const int n = first > 0 ? first : (last < 0 ? last : 0);
            return QQ(n, d);
        }
    }
    throw std::logic_error("enumeration: no rational before the mediant bound");
}

static size_t coefficient_bits(const xz_detail::U &f)
{
    size_t bits = 0;
    for (const auto &q : f)
    {
        bits = std::max(bits, std::max(q.num().sizeinbase(2), q.den().sizeinbase(2)));
    }
    return bits;
}

static int check_fibre(bool simple)
{
    // 054 旧追踪中的首个根盒左端点是 old_sample+1。
    // 固定 QQ 边界与 y 层乘积，仅比较后续首轮上下界和平方自由检查。
    const QQ old_sample(ZZ("-2029916547461639640406606009"),
                        ZZ("618970019642690137449562112"));
    const QQ q = simple ? sample_rational(std::nullopt, old_sample + QQ(1)) : old_sample;
    const std::vector<rational_interval> box{rational_interval(q - QQ(1, 8), q + QQ(1, 8))};
    variable y("y"), z("z");
    const std::vector<variable> vars{z};
    regular_chain::order_type order(custom_var_order({y, z}));
    xz_detail::P f(&order);
    const polynomial_ZZ original =
        polynomial_ZZ(y) * (pow(y, 3) * z + 2 * pow(y, 2) * pow(z, 2) + y * pow(z, 3) + 4) *
        (y + z) * (pow(y, 2) + y * z - 2) * (pow(y, 2) * z + y * pow(z, 2) + y - z);
    poly_convert(original, f);

    const auto coefficients = xz_detail::coefficients(f, y);
    const auto leading = assign_interval(coefficients.back(), vars, box);
    const QQ lower = std::min(abs(leading.left()), abs(leading.right()));
    QQ maximum(0);
    for (size_t i = 0; i + 1 < coefficients.size(); ++i)
    {
        const auto value = assign_interval(coefficients[i], vars, box);
        maximum = std::max(maximum, std::max(abs(value.left()), abs(value.right())));
    }
    // 保持正式实现的精确分式平移，隔离采样这一项变化。
    const QQ shift = QQ(2) + maximum / lower;
    const auto translated = xz_detail::translate(f, y, -shift);
    const auto bounds = xz_detail::bounds(translated, y, vars, box);
    const auto begin = std::chrono::steady_clock::now();
    const bool lower_squarefree = xz_detail::squarefree(bounds.first);
    const bool upper_squarefree = xz_detail::squarefree(bounds.second);
    const auto end = std::chrono::steady_clock::now();
    std::cout << "sample=" << q << " sample_den_bits=" << q.den().sizeinbase(2)
              << " lower_bits=" << coefficient_bits(bounds.first)
              << " upper_bits=" << coefficient_bits(bounds.second)
              << " two_squarefree_ms="
              << std::chrono::duration<double, std::milli>(end - begin).count()
              << " both_squarefree=" << (lower_squarefree && upper_squarefree) << '\n';
    return lower_squarefree && upper_squarefree ? 0 : 1;
}

int main(int argc, char **argv)
{
    if (argc == 2 && std::string(argv[1]) == "fibre-old")
    {
        return check_fibre(false);
    }
    if (argc == 2 && std::string(argv[1]) == "fibre-simple")
    {
        return check_fibre(true);
    }
    if (argc != 1)
    {
        std::cerr << "usage: rational-sampling-probes [fibre-old|fibre-simple]\n";
        return 2;
    }
    std::mt19937 random(20261022);
    size_t checked = 0;
    while (checked < 10000)
    {
        fraction left{static_cast<int>(random() % 401) - 200,
                      static_cast<int>(random() % 100) + 1};
        fraction right{static_cast<int>(random() % 401) - 200,
                       static_cast<int>(random() % 100) + 1};
        const int comparison = left.numerator * right.denominator -
                               right.numerator * left.denominator;
        if (comparison == 0)
        {
            continue;
        }
        if (comparison > 0)
        {
            std::swap(left, right);
        }
        const QQ a(left.numerator, left.denominator);
        const QQ b(right.numerator, right.denominator);
        const QQ actual = sample_rational(a, b);
        const QQ expected = enumerate_denominators(left, right);
        if (!(a < actual && actual < b) || actual != expected)
        {
            std::cerr << "interval=(" << a << "," << b << "), actual=" << actual
                      << ", expected=" << expected << '\n';
            return 1;
        }
        ++checked;
    }
    std::cout << checked << " rational intervals: membership and simplestness passed\n";
}
