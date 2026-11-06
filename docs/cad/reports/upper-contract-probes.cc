// Upper-layer review: independent rational Taylor expansion, without derivatives
// or the implementation's translation/coefficient-collection helpers.
#include <clpoly/lazard_qq.hh>
#include <array>
#include <iostream>
#include <map>
#include <random>
using namespace clpoly;
using U = std::map<int64_t, QQ>;
using Taylor = std::map<std::array<int64_t, 2>, U>;

static ZZ choose(int64_t n, int64_t k)
{
    ZZ result(1);
    for (int64_t j = 1; j <= k; ++j)
        result = result * ZZ(n - j + 1) / ZZ(j);
    return result;
}

static void clean(U &coefficients)
{
    for (auto it = coefficients.begin(); it != coefficients.end();)
        if (it->second == QQ(0))
            it = coefficients.erase(it);
        else
            ++it;
}

int main()
{
    variable x("x"), y("y"), z("z");
    std::mt19937 random(20261106);
    for (int n = 0; n < 72; ++n)
    {
        const variable u = n % 2 ? y : x, v = n % 2 ? x : y;
        regular_chain::order_type order(custom_var_order({z, v, u}));
        const QQ a(n % 5 - 2, n % 3 + 1), b(n % 7 - 3, n % 4 + 1);
        auto point = append_rational(real_algebraic_point{}, u, a, &order);
        point = append_rational(point, v, b, &order);
        polynomial_QQ X, Y, Z;
        poly_convert(polynomial_ZZ(u), X);
        poly_convert(polynomial_ZZ(v), Y);
        poly_convert(polynomial_ZZ(z), Z);
        polynomial_QQ f = Z + QQ(1);
        for (int j = 0; j < 6; ++j)
        {
            const int dx = random() % 4, dy = random() % 4, dz = random() % 3;
            const QQ c(static_cast<int>(random() % 7) - 3, random() % 3 + 1);
            f = f + c * pow(X - a, dx) * pow(Y - b, dy) * pow(Z, dz);
        }
        f = f * pow(X - a, n % 4) * pow(Y - b, n % 3);
        if (f.empty())
            throw std::logic_error("unexpected zero fixture");
        Taylor expansion;
        for (const auto &term : f)
        {
            const auto dx = term.first.deg(u), dy = term.first.deg(v), dz = term.first.deg(z);
            for (int64_t i = 0; i <= dx; ++i)
                for (int64_t j = 0; j <= dy; ++j)
                    expansion[{i, j}][dz] += term.second * QQ(choose(dx, i) * choose(dy, j)) *
                                             pow(a, dx - i) * pow(b, dy - j);
        }
        for (auto it = expansion.begin(); it != expansion.end();)
        {
            clean(it->second);
            if (it->second.empty())
                it = expansion.erase(it);
            else
                ++it;
        }
        const auto expected = expansion.begin();
        const auto actual = lazard_evaluate_qq(f, {u, v, z}, point);
        const std::vector<size_t> valuation{static_cast<size_t>(expected->first[0]),
                                            static_cast<size_t>(expected->first[1])};
        if (actual.valuation != valuation)
            throw std::logic_error("Taylor valuation mismatch");
        U specialized;
        for (const auto &term : actual.polynomial)
            specialized[term.first.deg(z)] += term.second * pow(a, term.first.deg(u)) *
                                                          pow(b, term.first.deg(v));
        clean(specialized);
        if (specialized != expected->second)
            throw std::logic_error("Taylor coefficient mismatch");
    }
    std::cout << "72 rational two-prefix Taylor cases: valuation and polynomial agree\n";
}
