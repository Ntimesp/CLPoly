/**
 * @file cad_test_cases.hh
 * @brief CAD 测试用例库 - 供 benchmark 和功能测试共享
 *
 * 包含多族测试用例：
 * 1. Lee 2025: Hyperspheres
 * 2. Chen 2023: Hong, cyclic, EBD, FIS, 5-VAR
 * 3. Li 2020: Han, P, Hong, Hong2, C
 * 4. 随机多项式
 * 5. 基础几何用例
 *
 * 使用方式：
 *   - 直接调用 make_xxx(n, ...) 获取单个 TestCase
 *   - 或通过 TestCaseConfig + make_test_case(cfg) 统一构建
 */
#ifndef CAD_TEST_CASES_HH
#define CAD_TEST_CASES_HH

#include <clpoly/clpoly.hh>
#include <clpoly/polynomial_convert.hh>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <cassert>

namespace cad_test {

using namespace clpoly;

// ========== 测试用例结构体 ==========

struct TestCase {
    std::string name;
    std::vector<polynomial_ZZ> polys;
    std::vector<variable> vars;
};

// ========== 统一配置（用于配置表和批量运行） ==========

/**
 * @brief 测试用例配置
 * @param family 族名，如 "hypersphere"
 * @param params 参数列表，如 {3} 或 {3, 64}
 * @param repeats benchmark 重复次数（默认 1）
 * @param enabled 是否启用（默认 true）
 *
 * 示例：
 *   {"hypersphere", {3}, 10, true}  → Hypersphere-3，重复 10 次
 *   {"cnr", {3, 64}, 10, true}      → C-3-64，重复 10 次
 */
struct TestCaseConfig {
    std::string family;
    std::vector<int> params;
    int repeats = 1;
    bool enabled = true;
};

// ========== 测试用例生成函数 ==========

/**
 * @brief Lee 2025 Hypersphere
 * f1 = Σ(x_i - 1)^2 - 4
 * f2 = Σ(x_i + 1)^2 - 4
 */
inline TestCase make_hypersphere(int n) {
    TestCase tc;
    std::ostringstream oss;
    oss << "Hypersphere-" << n;
    tc.name = oss.str();
    
    for (int i = 1; i <= n; ++i) {
        std::ostringstream voss;
        voss << "x" << i;
        tc.vars.emplace_back(voss.str());
    }
    
    polynomial_ZZ f1, f2;
    for (int i = 0; i < n; ++i) {
        const auto& v = tc.vars[i];
        if (i == 0) {
            f1 = pow(v, 2) - 2 * v + 1;
            f2 = pow(v, 2) + 2 * v + 1;
        } else {
            f1 = f1 + pow(v, 2) - 2 * v + 1;
            f2 = f2 + pow(v, 2) + 2 * v + 1;
        }
    }
    
    tc.polys.push_back(f1 - 4);
    tc.polys.push_back(f2 - 4);
    
    return tc;
}

/**
 * @brief Hong-n
 * f1 = sum(xi^2) - 1
 * f2 = prod(xi) - 1
 *
 * Chen 2023: n=4..8, Li 2020: n=10..11
 */
inline TestCase make_hong(int n) {
    TestCase tc;
    std::ostringstream oss;
    oss << "Hong-" << n;
    tc.name = oss.str();
    
    for (int i = 0; i < n; ++i) {
        std::ostringstream voss;
        voss << "x" << i;
        tc.vars.emplace_back(voss.str());
    }
    
    polynomial_ZZ f1;
    for (int i = 0; i < n; ++i) {
        if (i == 0) f1 = pow(tc.vars[i], 2);
        else f1 = f1 + pow(tc.vars[i], 2);
    }
    f1 = f1 - 1;
    
    polynomial_ZZ f2;
    for (int i = 0; i < n; ++i) {
        if (i == 0) f2 = tc.vars[i];
        else f2 = f2 * tc.vars[i];
    }
    f2 = f2 - 1;
    
    tc.polys = {f1, f2};
    return tc;
}

/**
 * @brief Cyclic-n
 * 经典循环根问题。n 个变量 x0..x{n-1}，n 个多项式：
 *   fk = sum_{i=0}^{n-1} (prod_{j=0}^{k-1} x_{(i+j) mod n})   (k = 1..n-1)
 *   fn = (prod_{i=0}^{n-1} xi) - 1
 */
inline TestCase make_cyclic(int n) {
    TestCase tc;
    std::ostringstream oss;
    oss << "cyclic-" << n;
    tc.name = oss.str();
    
    for (int i = 0; i < n; ++i) {
        std::ostringstream voss;
        voss << "x" << i;
        tc.vars.emplace_back(voss.str());
    }
    
    for (int k = 1; k <= n; ++k) {
        polynomial_ZZ f;
        for (int i = 0; i < n; ++i) {
            polynomial_ZZ term(1);
            for (int j = 0; j < k; ++j) {
                term = term * tc.vars[(i + j) % n];
            }
            if (i == 0) f = term;
            else f = f + term;
        }
        if (k == n) f = f - 1;
        tc.polys.push_back(f);
    }
    
    return tc;
}

/**
 * @brief EBD-n (n = 2, 5, 8)
 * 来自 England, Bradford, Davenport 的论文，Chen2023 中的具体例子
 */
inline TestCase make_ebd(int n) {
    TestCase tc;
    std::ostringstream oss;
    oss << "EBD-" << n;
    tc.name = oss.str();

    if (n == 2) {
        variable x("x"), y("y"), z("z");
        tc.vars = {x, y, z};
        tc.polys = {
            x + pow(y, 2) + z,
            x - pow(y, 2) + z,
            pow(x, 2) + pow(y, 2) + pow(z, 2) - 1
        };
    } else if (n == 5) {
        variable v("v"), u("u"), x("x"), y("y"), z("z");
        tc.vars = {v, u, x, y, z};
        tc.polys = {
            x - y + pow(z, 2),
            pow(z, 2) - pow(u, 2) + pow(v, 2) - 1,
            x + y + pow(z, 2),
            pow(z, 2) + pow(u, 2) - pow(v, 2) - 1,
            pow(x, 2) - 1,
            z
        };
    } else if (n == 8) {
        variable w("w"), x("x"), y("y"), z("z");
        tc.vars = {w, x, y, z};
        tc.polys = {
            x * y - pow(z, 2) - pow(w, 2) + 2 * z,
            pow(x, 2) + pow(y, 2) + pow(z, 2) + w + z,
            -pow(w, 2) - pow(y, 2) - pow(z, 2) + x + z,
            z + w
        };
    } else {
        throw std::invalid_argument("make_ebd only supports n = 2, 5, 8");
    }
    return tc;
}

/**
 * @brief FIS-n (n = 1..8)
 * 来自 Fukui, Iwamoto, Sun 2015，用于测试 equational constraints。
 * 从 GeometricCADv2 仓库提取的真实多项式。
 */
inline TestCase make_fis(int n) {
    TestCase tc;
    std::ostringstream oss;
    oss << "FIS-" << n;
    tc.name = oss.str();

    if (n == 1) {
        variable a("a"), b("b"), c("c"), x("x"), y("y");
        tc.vars = {a, b, c, x, y};
        tc.polys = {
            pow(b, 2) * pow(x - c, 2) + pow(a, 2) * pow(y, 2) - pow(a, 2) * pow(b, 2),
            pow(x, 2) + pow(y, 2) - 1
        };
    } else if (n == 2) {
        variable s("s"), x("x"), y("y"), z("z");
        tc.vars = {s, x, y, z};
        // FIS-2 has rational coefficients, build with QQ then convert to ZZ
        polynomial_QQ x_q = x * QQ(1), y_q = y * QQ(1), z_q = z * QQ(1), s_q = s * QQ(1);
        polynomial_QQ f1_q = x_q * s_q * (QQ(1) - x_q / QQ(400)) / QQ(200)
                           + y_q * s_q * (QQ(1) - x_q / QQ(400)) - QQ(35) * x_q / QQ(2);
        polynomial_QQ f2_q = QQ(250) * x_q * s_q * (QQ(1) - y_q / QQ(600)) * (z_q + QQ(3) / QQ(250))
                           - QQ(55) * y_q / QQ(2);
        polynomial_QQ f3_q = QQ(500) * (y_q + x_q / QQ(20)) * (QQ(1) - z_q / QQ(700))
                           - QQ(5) * z_q;
        polynomial_ZZ f1, f2, f3;
        poly_convert(f1_q, f1);
        poly_convert(f2_q, f2);
        poly_convert(f3_q, f3);
        tc.polys = {f1, f2, f3};
    } else if (n == 3) {
        variable l("l"), z("z"), r("r"), s1("s1"), c1("c1"), s2("s2"), c2("c2");
        tc.vars = {l, z, r, s1, c1, s2, c2};
        tc.polys = {
            r - c1 + l * (s1 * s2 - c1 * c2),
            z - s1 - l * (s1 * c2 + s2 * c1),
            pow(s1, 2) + pow(c1, 2) - 1,
            pow(s2, 2) + pow(c2, 2) - 1
        };
    } else if (n == 4) {
        variable x("x"), y("y"), z("z");
        tc.vars = {x, y, z};
        tc.polys = {
            pow(x, 2) + pow(y, 2) * z + pow(z, 3),
            3 * pow(x, 2) + 3 * pow(y, 2) + pow(z, 2) - 1,
            pow(x, 2) + pow(z, 2) - pow(y, 3) * pow(y - 1, 3)
        };
    } else if (n == 5) {
        variable a("a"), b("b"), x("x"), y("y"), z("z");
        tc.vars = {a, b, x, y, z};
        tc.polys = {
            x * y + a * x * z + y * z - 1,
            x * y * z + x * z + x * y - a,
            x * z + y * z - a * z - x - y - 1,
            a * x * y - b * y * z,
            a * y * z - b * z * x
        };
    } else if (n == 6) {
        variable a("a"), b("b"), x("x"), y("y"), z("z");
        tc.vars = {a, b, x, y, z};
        tc.polys = {
            x * y + a * x * z + y * z - 1,
            x * y * z + x * z + x * y - b,
            x * z + y * z - a * z - x - y - 1
        };
    } else if (n == 7) {
        variable x0("x0"), x1("x1"), x2("x2"), x3("x3"), x4("x4");
        tc.vars = {x4, x1, x3, x2, x0};
        tc.polys = {
            x0 * x2 - 1,
            pow(x0, 2) - 2 * x0 * x1 + pow(x2, 2) - 2 * x2 * x3 - x4,
            -16 * x1 * pow(x4, 2) - 800 * pow(x3, 3) - 1240 * pow(x3, 2) * x4 - 408 * x3 * pow(x4, 2) - 40 * pow(x4, 3) + 240 * x1 * x3 - 532 * x1 * x4 - 17720 * pow(x3, 2) - 6214 * x3 * x4 - 550 * pow(x4, 2) - 4480 * x1 + 25240 * x3 + 5695 * x4 + 1050,
            32 * pow(x1, 2) + 168 * x1 * x3 + 40 * x1 * x4 + 8 * pow(x3, 2) + 20 * x3 * x4 + 4 * pow(x4, 2) - 270 * x1 - 390 * x3 - 105 * x4 + 450,
            320 * x1 * x3 * x4 + 32 * x1 * pow(x4, 2) + 16 * x3 * pow(x4, 2) + 8320 * x1 * x3 + 264 * x1 * x4 + 240 * pow(x3, 2) - 372 * x3 * x4 - 140 * pow(x4, 2) - 14840 * x1 - 23380 * x3 - 2575 * x4 + 36750
        };
    } else if (n == 8) {
        variable x0("x0"), x1("x1"), x2("x2"), x3("x3"), x4("x4"), x5("x5"), x6("x6"), x7("x7"), x8("x8"), x9("x9");
        tc.vars = {x8, x9, x0, x1, x2, x3, x4, x5, x6, x7};
        tc.polys = {
            x4,
            x5,
            x0 * x8 + x1 * x9 - 1,
            2 * x0 * x2 + 2 * x1 * x3 + 2 * x4 * x5 - x0 - x1 - x4,
            x0 * x6 + x0 * x7 - x1 * x7 - x4 * x6 - x0 + x8,
            x2 * x6 + x2 * x7 - x3 * x7 - x5 * x6 - x2 + x9,
            2 * x0 * x2 - 2 * x0 * x4 + 2 * x1 * x3 - 2 * x1 * x5 - pow(x2, 2) - pow(x3, 2) + pow(x4, 2) + pow(x5, 2),
            pow(x0, 2) + pow(x1, 2) - pow(x2, 2) - pow(x3, 2) + pow(x4, 2) - pow(x5, 2) + x2 + x3 + x5,
            14 * pow(x9, 3) + 2 * pow(x8, 2) - 12 * x8 * x9 - 16 * pow(x9, 2) + 4 * x8 + 17 * x9 - 10,
            14 * x8 * pow(x9, 2) - 3 * pow(x8, 2) - 10 * x8 * x9 - 11 * pow(x9, 2) + x8 + 20 * x9 - 6,
            14 * pow(x8, 2) * x9 - 6 * pow(x8, 2) - 20 * x8 * x9 + 20 * pow(x9, 2) + 16 * x8 - 9 * x9 + 2,
            14 * pow(x8, 3) - 19 * pow(x8, 2) + 58 * x8 * x9 + 5 * pow(x9, 2) - 17 * x8 - 32 * x9 + 46,
            x2 * x3 * (x2 - x3)
        };
    } else {
        throw std::invalid_argument("make_fis only supports n = 1..8");
    }
    return tc;
}

/**
 * @brief 5-VAR
 * f = w^2 + ((x+y)^2*u - y*z)
 */
inline TestCase make_five_var() {
    TestCase tc;
    tc.name = "5-VAR";
    
    variable x("x"), y("y"), z("z"), u("u"), w("w");
    tc.vars = {x, y, z, u, w};
    tc.polys = {pow(w, 2) + (pow(x + y, 2) * u - y * z)};
    return tc;
}

/**
 * @brief Han-n
 * f = (sum xi^2)^2 - 4 * sum(xi^2 * x_{i+1}^2)
 *
 * 参考：Han, Dai, Hong, Xia (2017)
 */
inline TestCase make_han(int n) {
    TestCase tc;
    std::ostringstream oss;
    oss << "Han-" << n;
    tc.name = oss.str();
    
    for (int i = 1; i <= n; ++i) {
        std::ostringstream voss;
        voss << "x" << i;
        tc.vars.emplace_back(voss.str());
    }
    
    polynomial_ZZ sum_sq;
    for (int i = 0; i < n; ++i) {
        if (i == 0) sum_sq = pow(tc.vars[i], 2);
        else sum_sq = sum_sq + pow(tc.vars[i], 2);
    }
    
    polynomial_ZZ cross;
    for (int i = 0; i < n; ++i) {
        int next = (i + 1) % n;
        polynomial_ZZ term = pow(tc.vars[i], 2) * pow(tc.vars[next], 2);
        if (i == 0) cross = term;
        else cross = cross + term;
    }
    
    tc.polys = {pow(sum_sq, 2) - 4 * cross};
    return tc;
}

/**
 * @brief P
 * 6变量复杂多项式（Li 2020 Example 4）
 */
inline TestCase make_p() {
    TestCase tc;
    tc.name = "P";
    
    variable a("a"), b("b"), c("c"), d("d"), e("e"), f("f");
    tc.vars = {a, b, c, d, e, f};
    
    // From Li2020 / GeometricCADv2: P = (a*b + c)*(c - e*f + c*f^2)*(...)
    auto term1 = a*b + c;
    auto term2 = c - e*f + c*pow(f,2);
    auto term3 = 1 + pow(b,2) + pow(d,2)
               + a*(-2*(b*c + d*e) + a*(1 + pow(c,2) + pow(e,2)))
               + 2*(b - a*c)*(-d + a*e)*f + pow(b - a*c, 2)*pow(f,2);
    auto term4 = a*b*(d*f - b*(1 + pow(f,2)) + a*(c - e*f + c*pow(f,2)));
    auto term5 = -d*e + c*d*f - b*(c - e*f + c*pow(f,2))
               + a*(1 + pow(e,2) + c*(c - 2*e*f + c*pow(f,2)));
    
    tc.polys = { term1 * term2 * term3 - term4 * term5 };
    
    return tc;
}

/**
 * @brief Hong2-n
 * f1 = sum xi^2 - 2n
 * f2 = prod xi - 1
 */
inline TestCase make_hong2(int n) {
    TestCase tc;
    std::ostringstream oss;
    oss << "Hong2-" << n;
    tc.name = oss.str();
    
    for (int i = 0; i < n; ++i) {
        std::ostringstream voss;
        voss << "x" << i;
        tc.vars.emplace_back(voss.str());
    }
    
    polynomial_ZZ f1;
    for (int i = 0; i < n; ++i) {
        if (i == 0) f1 = pow(tc.vars[i], 2);
        else f1 = f1 + pow(tc.vars[i], 2);
    }
    f1 = f1 - 2 * n;
    
    polynomial_ZZ f2;
    for (int i = 0; i < n; ++i) {
        if (i == 0) f2 = tc.vars[i];
        else f2 = f2 * tc.vars[i];
    }
    f2 = f2 - 1;
    
    tc.polys = {f1, f2};
    return tc;
}

/**
 * @brief C-n-r
 * 球距离问题
 * f1 = x1^2 + ... + xn^2 - r
 * f2 = y1^2 + ... + yn^2 - 64
 * f3 = (x1-y1)^2 + ... + (xn-yn)^2 - 1/1000000
 */
inline TestCase make_cnr(int n, int r) {
    TestCase tc;
    std::ostringstream oss;
    oss << "C-" << n << "-" << r;
    tc.name = oss.str();
    
    for (int i = 1; i <= n; ++i) {
        std::ostringstream xoss, yoss;
        xoss << "x" << i;
        yoss << "y" << i;
        tc.vars.emplace_back(xoss.str());
        tc.vars.emplace_back(yoss.str());
    }
    
    // 先用 QQ 多项式构建，再转换为 ZZ
    polynomial_QQ f1_q, f2_q, f3_q;
    
    for (int i = 0; i < n; ++i) {
        if (i == 0) f1_q = pow(tc.vars[2*i], 2);
        else f1_q = f1_q + pow(tc.vars[2*i], 2);
    }
    f1_q = f1_q - r;
    
    for (int i = 0; i < n; ++i) {
        if (i == 0) f2_q = pow(tc.vars[2*i + 1], 2);
        else f2_q = f2_q + pow(tc.vars[2*i + 1], 2);
    }
    f2_q = f2_q - 64;
    
    for (int i = 0; i < n; ++i) {
        polynomial_QQ term = pow(tc.vars[2*i] - tc.vars[2*i + 1], 2);
        if (i == 0) f3_q = term;
        else f3_q = f3_q + term;
    }
    f3_q = f3_q - QQ(1, 1000000);
    
    polynomial_ZZ f1, f2, f3;
    poly_convert(f1_q, f1);
    poly_convert(f2_q, f2);
    poly_convert(f3_q, f3);
    
    tc.polys = {f1, f2, f3};
    return tc;
}

/**
 * @brief 随机多项式
 */
inline TestCase make_random(int num_vars, int num_polys, int degree, int terms) {
    TestCase tc;
    std::ostringstream oss;
    oss << "random_" << num_vars << "v_" << num_polys << "p_d" << degree << "_t" << terms;
    tc.name = oss.str();
    
    for (int i = 0; i < num_vars; ++i) {
        std::ostringstream voss;
        voss << "x" << i;
        tc.vars.emplace_back(voss.str());
    }
    
    for (int i = 0; i < num_polys; ++i) {
        tc.polys.push_back(random_polynomial<ZZ>(tc.vars, degree, terms, {-5, 5}));
    }
    
    return tc;
}

// ========== 统一构建接口 ==========

using MakeTestCase = std::function<TestCase(const std::vector<int>&)>;

/**
 * @brief 测试用例注册表
 * family 名 → 构建函数（lambda 内做参数数量 assert）
 */
inline const std::unordered_map<std::string, MakeTestCase>& test_case_registry() {
    static const std::unordered_map<std::string, MakeTestCase> registry = {
        // 1 参数
        {"hypersphere", [](const std::vector<int>& p){ assert(p.size()==1); return make_hypersphere(p[0]); }},
        {"hong",        [](const std::vector<int>& p){ assert(p.size()==1); return make_hong(p[0]); }},
        {"cyclic",      [](const std::vector<int>& p){ assert(p.size()==1); return make_cyclic(p[0]); }},
        {"ebd",         [](const std::vector<int>& p){ assert(p.size()==1); return make_ebd(p[0]); }},
        {"fis",         [](const std::vector<int>& p){ assert(p.size()==1); return make_fis(p[0]); }},
        {"han",         [](const std::vector<int>& p){ assert(p.size()==1); return make_han(p[0]); }},
        {"hong2",       [](const std::vector<int>& p){ assert(p.size()==1); return make_hong2(p[0]); }},
        // 2 参数
        {"cnr",         [](const std::vector<int>& p){ assert(p.size()==2); return make_cnr(p[0], p[1]); }},
        // 4 参数
        {"random",      [](const std::vector<int>& p){ assert(p.size()==4); return make_random(p[0], p[1], p[2], p[3]); }},
        // 0 参数
        {"five_var",    [](const std::vector<int>& p){ assert(p.empty()); (void)p; return make_five_var(); }},
        {"p",           [](const std::vector<int>& p){ assert(p.empty()); (void)p; return make_p(); }},
    };
    return registry;
}

/**
 * @brief 统一工厂：根据 TestCaseConfig 构建 TestCase
 */
inline TestCase make_test_case(const TestCaseConfig& cfg) {
    const auto& reg = test_case_registry();
    auto it = reg.find(cfg.family);
    if (it == reg.end()) {
        throw std::invalid_argument("Unknown family: " + cfg.family);
    }
    return it->second(cfg.params);
}

} // namespace cad_test

#endif // CAD_TEST_CASES_HH
