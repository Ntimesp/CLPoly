// 补充审核探针：检查零根补齐、重根拒绝及指定纤维。
// 验证范围和编译方法见 modular-cad.md。
#include <clpoly/full_cad.hh>
#include <iostream>
using namespace clpoly;
using P = polynomial_ZZ;
int main(int argc, char **argv)
{
    variable x("x"), y("y"), z("z");
    const regular_chain::order_type order(custom_var_order({z, y, x}));
    if (argc > 1 && std::string(argv[1]) == "uni-zero")
    {
        auto a = xz_detail::univariate({QQ(0), QQ(1)}, QQ(1, 16));
        auto b = xz_detail::univariate({QQ(0), QQ(-1), QQ(1)}, QQ(1, 16));
        std::cout << "UNI(z): " << a.size() << " (expected 1); UNI(z(z-1)): " << b.size()
                  << " (expected 2)\n";
        return a.size() == 1 && b.size() == 2 ? 0 : 1;
    }
    if (argc > 1 && std::string(argv[1]) == "grid")
    {
        auto c = full_cad(
            std::vector<P>{pow((x + 2) * (2 * x - 1), 2), pow((3 * y + 1) * (y - 2), 3)}, {y, x});
        if (c.leaf_count() != 25)
            throw std::runtime_error("grid count");
        for (size_t j = 0; j < 25; ++j)
        {
            auto a = c.leaf_sample(j);
            auto parent = c.node(1, j).parent();
            int sx = (parent % 2) ? 0 : 1;
            size_t iy = j % 5;
            int sy = iy % 2 ? 0 : (iy == 2 ? -1 : 1);
            if (c.signs(j) != std::vector<int>{sx, sy})
                throw std::runtime_error("grid signs");
        }
        std::cout << "25 cells: independently expected signs and count passed\n";
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "uni-repeated")
    {
        try
        {
            xz_detail::univariate({QQ(1), QQ(-2), QQ(1)}, QQ(1, 16));
        }
        catch (const std::invalid_argument &)
        {
            std::cout << "UNI: repeated roots rejected\n";
            return 0;
        }
        throw std::runtime_error("UNI accepted nonsquarefree input");
    }
    size_t checks = 0;
    for (int n = 1; n <= 12; ++n)
    {
        auto pts = isolate_real_points(regular_chain(
            {x, y}, std::vector<P>{(2 * x - n) * (3 * x + n),
                                   (x * x + 1) * (5 * y - 5 * x - 1) * (7 * y + 7 * x + 2)}, &order));
        if (pts.size() != 4)
            throw std::runtime_error("root count");
        for (auto &a : pts)
        {
            auto before = a.box();
            a.refine(QQ(1, 4096));
            real_algebraic_point recert(a.chain(), a.box(), a.shifts());
            if (!before[0].contains(a.box()[0]) || !before[1].contains(a.box()[1]) ||
                !same_real_point(a, recert))
                throw std::runtime_error("certificate");
            bool xp = point_zero(P(2 * x - n), a), yp = point_zero(P(5 * y - 5 * x - 1), a);
            QQ xx = xp ? QQ(n, 2) : QQ(-n, 3);
            QQ yy = yp ? xx + QQ(1, 5) : -xx - QQ(2, 7);
            if (!a.box()[0].interior_contains(xx) || !a.box()[1].interior_contains(yy))
                throw std::runtime_error("known rational solution");
            auto r = roots_over(P(pow((2 * x - n) * z * z + (3 * x + n) * z + 1, 2)), a, z, &order);
            if (r.size() != (xp ? 1 : 2))
                throw std::runtime_error("branch roots");
            for (auto b : r)
                if (!point_zero(P((2 * x - n) * z * z + (3 * x + n) * z + 1), b))
                    throw std::runtime_error("root equation");
            ++checks;
        }
    }
    auto a = isolate_real_points(regular_chain({x}, std::vector<P>{x * x - 2}, &order)).back();
    real_algebraic_point b;
    b = a;
    a = real_algebraic_point{};
    if (!point_zero(P(x * x - 2), b))
        throw std::runtime_error("assignment owner");
    std::cout << checks << " generated point/fibre/certificate checks passed; assignment passed\n";
}
