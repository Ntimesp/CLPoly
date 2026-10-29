#include <clpoly/clpoly.hh>
#include <clpoly/charset.hh>
#include "clpoly_test.hh"

using namespace clpoly;

int main() {
    variable c("c"), x("x"), y("y"), z("z");

    // ======== charset: basic ========
    CLPOLY_TEST("charset_basic");
    {
        lex_<custom_var_order> od(custom_var_order({x, c, y, z}));
        polynomial_<ZZ, lex_<custom_var_order>> f1(&od), f2(&od), f3(&od), f4(&od);
        f1 = 1 - c*x - x*y*y - x*z*z;
        f2 = 1 - c*y - y*pow(x,2) - y*pow(z,2);
        f3 = 1 - c*z - z*pow(x,2) - z*pow(y,2);
        f4 = 8*pow(c,6) + 378*pow(c,3) - 27;

        std::vector<polynomial_<ZZ, lex_<custom_var_order>>> P = {f1, f2, f3, f4};
        auto C = charset(P);

        // charset should return non-empty result
        CLPOLY_ASSERT_TRUE(C.size() > 0);

        // Verify: prem of each input w.r.t. charset should be zero
        for (auto& p : P) {
            auto f = p;
            for (auto it = C.rbegin(); it < C.rend(); ++it) {
                f = prem(f, *it, get_first_var(*it), false);
            }
            polynomial_<ZZ, lex_<custom_var_order>> zero(&od);
            CLPOLY_ASSERT_EQ(f, zero);
        }
    }

    // ======== charset: empty input ========
    CLPOLY_TEST("charset_empty");
    {
        lex_<custom_var_order> od(custom_var_order({x, y}));
        std::vector<polynomial_<ZZ, lex_<custom_var_order>>> P;
        auto C = charset(P);
        CLPOLY_ASSERT_EQ((int)C.size(), 0);
    }

    // ======== wrsd: basic ========
    CLPOLY_TEST("wrsd_basic");
    {
        variable x1("x1"), x2("x2");
        lex_<custom_var_order> mo(custom_var_order({x2, x1}));
        polynomial_<ZZ, lex_<custom_var_order>> T1(&mo), T2(&mo), f(&mo);
        T1 = (x1 + 1) * pow(x1, 2);
        T2 = x2;
        f = x1 + x2;

        poly_convert(T1, T1);
        poly_convert(T2, T2);
        poly_convert(f, f);

        std::vector<polynomial_<ZZ, lex_<custom_var_order>>> T = {T1, T2};
        auto W = wrsd(T, f);

        // W.first = H (zero decomposition), W.second = G (nonzero decomposition)
        // At least one of H or G should be non-empty
        CLPOLY_ASSERT_TRUE(W.first.size() > 0 || W.second.size() > 0);
    }

    // ======== rankcompare ========
    CLPOLY_TEST("rankcompare_basic");
    {
        lex_<custom_var_order> od(custom_var_order({x, y, z}));
        polynomial_<ZZ, lex_<custom_var_order>> f1(&od), f2(&od), f3(&od);
        f1 = pow(x, 2) + y;     // main var: x, degree 2
        f2 = pow(x, 3) + z;     // main var: x, degree 3
        f3 = pow(y, 2) + 1;     // main var: y

        // rank(f1) < rank(f2) since same variable but lower degree
        CLPOLY_ASSERT_TRUE(rankcompare(f1, f2));
        CLPOLY_ASSERT_FALSE(rankcompare(f2, f1));
    }

    // ======== is_reduced ========
    CLPOLY_TEST("is_reduced_basic");
    {
        lex_<custom_var_order> od(custom_var_order({x, y, z}));
        polynomial_<ZZ, lex_<custom_var_order>> f(&od), g(&od);
        f = y + 1;              // deg(f, x) = 0
        g = pow(x, 2) + y;     // main var: x, first_deg = 2

        // f is reduced w.r.t. g if deg(f, main_var(g)) < first_deg(g)
        CLPOLY_ASSERT_TRUE(is_reduced(f, g));

        // h has x^2 so deg(h, x) = 2 >= 2 = first_deg(g), not reduced
        polynomial_<ZZ, lex_<custom_var_order>> h(&od);
        h = pow(x, 2) + 1;
        CLPOLY_ASSERT_FALSE(is_reduced(h, g));

        // 零和非零常数关于任何非常数多项式都已约化。
        f = 0;
        CLPOLY_ASSERT_TRUE(is_reduced(f, g));
        f = 2;
        CLPOLY_ASSERT_TRUE(is_reduced(f, g));
    }

    // ======== __basset ========
    CLPOLY_TEST("__basset_basic");
    {
        lex_<custom_var_order> od(custom_var_order({x, y, z}));
        polynomial_<ZZ, lex_<custom_var_order>> f1(&od), f2(&od);
        f1 = pow(x, 2) + y;
        f2 = x*y + z;
        std::vector<polynomial_<ZZ, lex_<custom_var_order>>> P = {f1, f2};
        auto B = __basset<ZZ>(P.begin(), P.end());
        // 基列非空。
        CLPOLY_ASSERT_TRUE(B.size() > 0);
        // 此例中的基列方程均为非常数。
        for (auto& b : B) {
            CLPOLY_ASSERT_FALSE(b.empty());
        }
    }

    CLPOLY_TEST("__basset_nonzero_constant");
    {
        lex_<custom_var_order> od(custom_var_order({x}));
        using P = polynomial_<ZZ, lex_<custom_var_order>>;
        P f(&od), constant(&od);
        f = x*x - 1;
        constant = 2;
        const std::vector<P> input{f, constant};

        // 非零常数构成矛盾列，直接结束，不调用常数上的约化判定。
        CLPOLY_ASSERT_EQ(__basset<ZZ>(input.begin(), input.end()),
                         std::vector<P>{constant});
    }

    // ======== sqrfree (vector version) ========
    CLPOLY_TEST("sqrfree_basic");
    {
        lex_<custom_var_order> od(custom_var_order({x, y}));
        polynomial_<ZZ, lex_<custom_var_order>> f1(&od), f2(&od);
        // f1 = (x+1)^2 = x^2 + 2x + 1 (not squarefree)
        f1 = pow(x, 2) + 2*x + 1;
        // f2 = x^2 - 1 = (x-1)(x+1) (squarefree)
        f2 = pow(x, 2) - 1;
        std::vector<polynomial_<ZZ, lex_<custom_var_order>>> polys = {f1, f2};
        auto result = sqrfree(polys);
        CLPOLY_ASSERT_EQ((int)result.size(), 2);
        // Each result should be squarefree
        for (auto& r : result) {
            if (!is_number(r)) {
                CLPOLY_ASSERT_TRUE(is_squarefree(r));
            }
        }
    }

    // ======== rsd (regular subresultant decomposition) ========
    CLPOLY_TEST("rsd_basic");
    {
        lex_<custom_var_order> od(custom_var_order({x, y}));
        polynomial_<ZZ, lex_<custom_var_order>> T1(&od), f(&od);
        T1 = pow(x, 2) + y;   // triangular set with main var x
        f = x + y;

        std::vector<polynomial_<ZZ, lex_<custom_var_order>>> T = {T1};
        auto W = rsd(T, f);
        // W.first = zero decomposition, W.second = nonzero decomposition
        // At least one part should be non-empty
        CLPOLY_ASSERT_TRUE(W.first.size() > 0 || W.second.size() > 0);
    }

    // ======== is_regular ========
    CLPOLY_TEST("is_regular_basic");
    {
        lex_<custom_var_order> od(custom_var_order({x, y, z}));
        polynomial_<ZZ, lex_<custom_var_order>> T1(&od), T2(&od);
        // Simple triangular set: T1 in y, T2 in x
        T1 = y + 1;
        T2 = pow(x, 2) + y;
        std::vector<polynomial_<ZZ, lex_<custom_var_order>>> T = {T1, T2};
        // Check if triangular set is regular
        bool reg = is_regular(T);
        // A simple triangular set with constant leading coefficients should be regular
        CLPOLY_ASSERT_TRUE(reg);

        // Single element triangular set should be regular
        std::vector<polynomial_<ZZ, lex_<custom_var_order>>> T_single = {T1};
        CLPOLY_ASSERT_TRUE(is_regular(T_single));
    }

    // ======== RCtoSqrfree: single equation with repeated roots ========
    CLPOLY_TEST("RCtoSqrfree_single_equation");
    {
        lex_<custom_var_order> od(custom_var_order({x}));
        using P = polynomial_<ZZ, lex_<custom_var_order>>;
        P f(&od), expected(&od);
        f = pow(x - 1, 3) * pow(x + 2, 2);
        expected = (x - 1) * (x + 2);

        auto result = RCtoSqrfree(std::vector<P>{f});
        CLPOLY_ASSERT_TRUE(result.size() == 1 && result[0].size() == 1);
        if (result.size() == 1 && result[0].size() == 1) {
            // Pseudo-division may introduce a nonzero constant factor.
            CLPOLY_ASSERT_EQ(pp(result[0][0]), expected);
        }
    }

    // ======== RCtoSqrfree: only the first equation has repeated roots ========
    CLPOLY_TEST("RCtoSqrfree_first_equation");
    {
        lex_<custom_var_order> od(custom_var_order({y, x}));
        using P = polynomial_<ZZ, lex_<custom_var_order>>;
        P f(&od), g(&od), expected(&od);
        f = pow(x - 1, 3) * pow(x + 2, 2);
        g = y - x;
        expected = (x - 1) * (x + 2);

        auto result = RCtoSqrfree(std::vector<P>{f, g});
        CLPOLY_ASSERT_TRUE(result.size() == 1 && result[0].size() == 2);
        if (result.size() == 1 && result[0].size() == 2) {
            CLPOLY_ASSERT_EQ(pp(result[0][0]), expected);
            CLPOLY_ASSERT_EQ(result[0][1], g);
        }
    }

    return clpoly_test::test_summary();
}
