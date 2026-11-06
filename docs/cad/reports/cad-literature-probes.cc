#include <clpoly/full_cad.hh>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace clpoly;
using P = polynomial_ZZ;

void require(bool value, const char *message)
{
    if (!value)
    {
        throw std::runtime_error(message);
    }
}

// 先按解析根检查一元适配器，再用已知几何胞腔数检查 CAD。
int main(int argc, char **argv)
{
    variable x("audit_x"), y("audit_y"), z("audit_z");
    const regular_chain::order_type order(custom_var_order({z, y, x}));
    const std::string mode = argc > 1 ? argv[1] : "geometry";
    if (mode == "boundary")
    {
        auto cad = full_cad(std::vector<P>{P(x), P(x - 1)}, {x});
        const auto &boundary = cad.structure().node(0, 3).roots()[0];
        const auto &claimed = cad.structure().level_polys(0)[boundary.poly_idx()];
        auto actual = cad.root(0, SIZE_MAX, boundary);
        require(boundary.root_idx() == 0 && degree(claimed, x) == 1, "边界语义探针输入不符合预期");
        require(point_zero(claimed, actual), "边界来源多项式不在所指根上为零");
        require(point_zero(P(x - 1), actual), "来源根身份没有指向 1");
        std::cout << "boundary=" << boundary << " claimed_polynomial=" << claimed
                  << " degree=" << degree(claimed, x) << " actual_root_is_one=1\n";
        return 0;
    }
    if (mode == "uni")
    {
        for (int n = 1; n <= 12; ++n)
        {
            xz_detail::U f{QQ(1)};
            for (int i = 0; i < n; ++i)
            {
                const QQ root(i - n / 2, 3);
                xz_detail::U next(f.size() + 1, QQ(0));
                for (size_t j = 0; j < f.size(); ++j)
                {
                    next[j] -= root * f[j];
                    next[j + 1] += f[j];
                }
                f = std::move(next);
            }
            auto roots = xz_detail::univariate(f, QQ(1, 4096));
            require(roots.size() == size_t(n), "UNI 漏根或重复根");
            for (int i = 0; i < n; ++i)
                require(roots[i].interior_contains(QQ(i - n / 2, 3)), "UNI 根对应错误");
        }
        std::cout << "UNI: 12 个多项式，78 个解析根全部正确\n";
        return 0;
    }
    if (mode == "fibre")
    {
        for (int n : {1, 2, 4, 8, 12})
        {
            P f(1);
            for (int i = 1; i <= n; ++i)
            {
                f = f * (x - i);
            }
            auto points = isolate_real_points(regular_chain({x}, std::vector<P>{f}, &order));
            const auto start = std::chrono::steady_clock::now();
            auto roots = roots_over(P(y - x), points.back(), y, &order);
            const auto elapsed =
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
                    .count();
            require(roots.size() == 1, "单纤维根数错误");
            require(roots[0].box().back().interior_contains(QQ(n)), "单纤维根值错误");
            std::cout << "fibre n=" << n << " returned=" << roots.size()
                      << " elapsed_ms=" << elapsed << std::endl;
        }
        return 0;
    }
    if (mode != "geometry")
    {
        throw std::invalid_argument("可选模式：geometry、uni、fibre、boundary");
    }
    auto run = [&](const char *name, const std::vector<P> &input, const std::vector<variable> &vars,
                   size_t expected)
    {
        const auto start = std::chrono::steady_clock::now();
        auto cad = full_cad(input, vars);
        const double ms =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
                .count();
        std::cout << name << ": leaves=" << cad.leaf_count() << " expected=" << expected
                  << " ms=" << ms << std::endl;
        require(cad.leaf_count() == expected, name);
        if (std::string(name) == "circle-plus-line")
        {
            // 根据圆和水平轴的几何位置独立给出符号，不调用点判零作为判据。
            const int middle_circle[] = {1, 0, -1, -1, -1, 0, 1};
            const int middle_line[] = {-1, -1, -1, 0, 1, 1, 1};
            for (size_t leaf = 0; leaf < cad.leaf_count(); ++leaf)
            {
                const size_t parent = cad.node(1, leaf).parent();
                const size_t child = leaf - cad.node(0, parent).child_begin();
                const int circle_sign = parent == 2 ? middle_circle[child]
                                        : (parent == 1 || parent == 3) && child == 1 ? 0
                                                                                     : 1;
                const int line_sign = parent == 2  ? middle_line[child]
                                      : child == 0 ? -1
                                      : child == 1 ? 0
                                                   : 1;
                require(cad.signs(leaf) == std::vector<int>({circle_sign, line_sign}),
                        "圆与水平轴的独立几何符号不匹配");
            }
        }
    };
    // x<0 时二次纤维无根；x=0 时一个根；x>0 时两个根。
    run("parabola", {P(y * y - x)}, {y, x}, 9);
    // x=0 上两根重合，三个 x 胞腔的贡献分别为 5、3、5。
    run("two-graphs", {P(y - x), P(y + x)}, {y, x}, 13);
    // x=-1、1 分层，五个父胞腔的贡献为 3、3、7、3、3。
    run("circle-plus-line", {P(x * x + y * y - 1), P(y)}, {y, x}, 19);
    // 原点的普通专门化均为零，但 Lazard 值分别为 -1 和 z，仍有 z=0 这个 Section。
    // 两个非零 x 的父扇区各贡献 29 个叶，x=0 的父层贡献 3+3+3 个叶，共 67。
    run("nullification-pair", {P(x * z - y), P(y * z - x)}, {z, y, x}, 67);
    // 改变投影变量顺序，旋转对称的抛物线族仍应有相同胞腔数。
    run("parabola-order", {P(x * x - y)}, {x, y}, 9);
    run("unbound-free-coordinate", {P(x * x - 2)}, {z, y, x}, 5);
    run("no-input", {}, {z, y, x}, 1);
    std::cout << "geometry probes complete\n";
}
