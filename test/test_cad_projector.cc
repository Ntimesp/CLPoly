/**
 * @file test_cad_projector.cc
 * @brief Test projection operators (MCCALLUM and LAZARD)
 *
 * This test verifies that projection computations run without errors.
 */
#include "clpoly_test.hh"
#include <clpoly/clpoly.hh>
#include <iostream>

using namespace clpoly;

// 检查两个投影结果是否等价。
// 设计契约：投影只归一化输入（cont/prim + basis），输出不做处理留给下一层，
// 因此输出可能含重复项/内嵌因子内容，正确性判据是输出的 squarefree basis 一致。
// squarefreebasis 修复后（issue #14/#17，上游 2026-05）输出已 canonical：
// 去重、无 unit、首项正，可直接集合比较。
void verify_proj_eq(const std::vector<polynomial_ZZ>& F,const std::vector<polynomial_ZZ>& G)
{
    auto Fn = squarefreebasis(F).first;
    auto Gn = squarefreebasis(G).first;
    CLPOLY_ASSERT_EQ(Fn.size(), Gn.size());
    for (const auto& p : Fn) {
        auto it = std::find(Gn.begin(), Gn.end(), p);
        CLPOLY_ASSERT(it != Gn.end());
        if (it != Gn.end())
            Gn.erase(it);
    }
}

int main() {
    // Define variables and lexicographic order (x1 < x2 < x3)
    variable x1("x1"), x2("x2"), x3("x3");
    // Construct polynomials
    polynomial_ZZ p1, p2, p3;
    p1 = x3*x3 + x2*x3 + x1 - 1;
    p2 = x2*x2 - x1*x3 + 2;
    p3 = x3 - x1*x1 - x2;

    std::vector<polynomial_ZZ> polys = {p1, p2, p3};

    // ----- MCCALLUM projection -----
    CLPOLY_TEST("MCCALLUM Projection operators");
    {
        auto mccallum_res = polys;
        auto mccallum_fac = polys;  // FACTOR 分支（SQUAREFREE 为默认参数）
        std::vector<polynomial_ZZ> expected_res;
        // Project x3
        mccallum_res = project(mccallum_res, x3, projection_method::MCCALLUM);
        mccallum_fac = project(mccallum_fac, x3, projection_method::MCCALLUM,
                               basis_computation_method::FACTOR);
        expected_res = {x1-1, x2, pow(x2,2)+polynomial_ZZ({{{},2}}), x1, \
pow(x1,2)+x2, -pow(x2,2)+4*x1+polynomial_ZZ({{{},-4}}), \
pow(x2,4)+pow(x2,3)*x1+pow(x1,3)+4*pow(x2,2)+2*x2*x1-pow(x1,2)+polynom\
ial_ZZ({{{},4}}), pow(x1,4)+3*x2*pow(x1,2)+2*pow(x2,2)+x1-1, \
-pow(x1,3)+pow(x2,2)-x2*x1+polynomial_ZZ({{{},2}})};
        verify_proj_eq(mccallum_res,expected_res);
        verify_proj_eq(mccallum_fac,expected_res);  // FACTOR 分支应与同一正确答案一致

        // Project x2
        mccallum_res = project(mccallum_res, x2, projection_method::MCCALLUM);
        mccallum_fac = project(mccallum_fac, x2, projection_method::MCCALLUM,
                               basis_computation_method::FACTOR);
        expected_res = {x1-1, x1, polynomial_ZZ(), polynomial_ZZ({{{},2}}), \
polynomial_ZZ({{{},4}}), pow(x1,3)-pow(x1,2)+polynomial_ZZ({{{},4}}), \
pow(x1,4)+x1-1, polynomial_ZZ({{{},3}}), \
pow(x1,3)+polynomial_ZZ({{{},-2}}), polynomial_ZZ({{{},-8}}), \
polynomial_ZZ({{{},16}}), \
27*pow(x1,6)-310*pow(x1,5)+603*pow(x1,4)-288*pow(x1,3)-1024*pow(x1,2)+1\
024*x1+polynomial_ZZ({{{},-256}}), \
pow(x1,4)-8*x1+polynomial_ZZ({{{},8}}), \
4*pow(x1,3)+pow(x1,2)+polynomial_ZZ({{{},-8}}), \
pow(x1,4)+polynomial_ZZ({{{},2}}), 2*x1-1, \
pow(x1,8)+2*pow(x1,5)+8*pow(x1,4)+pow(x1,2)-10*x1+polynomial_\
ZZ({{{},25}}), pow(x1,4)-4*x1+polynomial_ZZ({{{},4}}), \
pow(x1,8)-pow(x1,7)+4*pow(x1,4)-pow(x1,3)-pow(x1,2)+polynomial_\
ZZ({{{},4}}), pow(x1,3)-17*pow(x1,2)+16*x1+polynomial_ZZ({{{},-4}}), \
pow(x1,4)-9*x1+polynomial_ZZ({{{},9}}), \
pow(x1,6)-8*pow(x1,4)+20*pow(x1,2)-16*x1+polynomial_ZZ({{{},4}}), \
pow(x1,8)-2*pow(x1,7)+2*pow(x1,5)+15*pow(x1,4)-9*pow(x1,3)-pow(x1,2)-1\
0*x1+polynomial_ZZ({{{},25}}), \
pow(x1,8)-pow(x1,7)+2*pow(x1,5)-2*pow(x1,4)+10*pow(x1,3)-7*pow(x1,2)-1\
0*x1+polynomial_ZZ({{{},25}})};
        verify_proj_eq(mccallum_res,expected_res);
        verify_proj_eq(mccallum_fac,expected_res);  // FACTOR 分支应与同一正确答案一致
    }

    // ----- LAZARD projection -----
    CLPOLY_TEST("LAZARD Projection operators");
    {
        auto lazard_res = polys;   // intentionally keeping the typo
        auto lazard_fac = polys;   // FACTOR 分支（SQUAREFREE 为默认参数）
        std::vector<polynomial_ZZ> expected_res;
        // Project x3
        lazard_res = project(lazard_res, x3, projection_method::LAZARD);
        lazard_fac = project(lazard_fac, x3, projection_method::LAZARD,
                             basis_computation_method::FACTOR);
        expected_res = {x1-1, pow(x2,2)+polynomial_ZZ({{{},2}}), x1, pow(x1,2)+x2, \
-pow(x2,2)+4*x1+polynomial_ZZ({{{},-4}}), \
pow(x2,4)+pow(x2,3)*x1+pow(x1,3)+4*pow(x2,2)+2*x2*x1-pow(x1,2)+polynom\
ial_ZZ({{{},4}}), pow(x1,4)+3*x2*pow(x1,2)+2*pow(x2,2)+x1-1, \
-pow(x1,3)+pow(x2,2)-x2*x1+polynomial_ZZ({{{},2}})};
        verify_proj_eq(lazard_res,expected_res);
        verify_proj_eq(lazard_fac,expected_res);  // FACTOR 分支应与同一正确答案一致

        // Project x2
        lazard_res = project(lazard_res, x2, projection_method::LAZARD);
        lazard_fac = project(lazard_fac, x2, projection_method::LAZARD,
                             basis_computation_method::FACTOR);
        expected_res = {x1-1, x1, polynomial_ZZ({{{},2}}), polynomial_ZZ({{{},4}}), \
pow(x1,3)-pow(x1,2)+polynomial_ZZ({{{},4}}), pow(x1,4)+x1-1, \
pow(x1,3)+polynomial_ZZ({{{},-2}}), polynomial_ZZ({{{},-8}}), \
polynomial_ZZ({{{},16}}), \
27*pow(x1,6)-310*pow(x1,5)+603*pow(x1,4)-288*pow(x1,3)-1024*pow(x1,2)+1\
024*x1+polynomial_ZZ({{{},-256}}), \
pow(x1,4)-8*x1+polynomial_ZZ({{{},8}}), \
4*pow(x1,3)+pow(x1,2)+polynomial_ZZ({{{},-8}}), \
pow(x1,4)+polynomial_ZZ({{{},2}}), 2*x1-1, \
pow(x1,8)+2*pow(x1,5)+8*pow(x1,4)+pow(x1,2)-10*x1+polynomial_\
ZZ({{{},25}}), pow(x1,4)-4*x1+polynomial_ZZ({{{},4}}), \
pow(x1,8)-pow(x1,7)+4*pow(x1,4)-pow(x1,3)-pow(x1,2)+polynomial_\
ZZ({{{},4}}), pow(x1,3)-17*pow(x1,2)+16*x1+polynomial_ZZ({{{},-4}}), \
pow(x1,4)-9*x1+polynomial_ZZ({{{},9}}), \
pow(x1,6)-8*pow(x1,4)+20*pow(x1,2)-16*x1+polynomial_ZZ({{{},4}}), \
pow(x1,8)-2*pow(x1,7)+2*pow(x1,5)+15*pow(x1,4)-9*pow(x1,3)-pow(x1,2)-1\
0*x1+polynomial_ZZ({{{},25}}), \
pow(x1,8)-pow(x1,7)+2*pow(x1,5)-2*pow(x1,4)+10*pow(x1,3)-7*pow(x1,2)-1\
0*x1+polynomial_ZZ({{{},25}})};
        verify_proj_eq(lazard_res,expected_res);
        verify_proj_eq(lazard_fac,expected_res);  // FACTOR 分支应与同一正确答案一致
    }

    // ----- LAZARD projection with x factor (covers __divide_by_first_var) -----
    CLPOLY_TEST("LAZARD projection with x factor");
    {
        // f = x * (x + y - 1) = x^2 + xy - x
        // 在 SQUAREFREE 模式下，squarefreebasis 后仍是 x*(x+y-1)
        // Lazard 投影需要提取 x 因子，触发 __divide_by_first_var
        variable x("x"), y("y");
        polynomial_ZZ f = x * (x + y - 1);  // x^2 + xy - x
        
        std::vector<polynomial_ZZ> polys = {f};
        
        // Project x（消去 x）：SQUAREFREE 走 __divide_by_first_var 手动拆 x 因子；
        // FACTOR 下 factorbasis 已完全分解，x 因子自然单独成项。两者应与同一正确答案一致。
        auto proj_res = project(polys, x, projection_method::LAZARD);
        auto proj_fac = project(polys, x, projection_method::LAZARD,
                                basis_computation_method::FACTOR);

        // x*(x+y-1) 的 Lazard 投影应包含：tc=y-1（来自 x+y-1 的 tailcoeff，判别式）
        std::vector<polynomial_ZZ> expected_res = {y-1};
        verify_proj_eq(proj_res, expected_res);
        verify_proj_eq(proj_fac, expected_res);  // FACTOR 分支应与同一正确答案一致
    }

    return clpoly_test::test_summary();
}
