/**
 * @file bench_sqfbasis_vs_factbasis.cc
 * @brief 测试 fullproj 中 basis 计算的时间占比随问题规模的变化
 *
 * 使用 cad_test_cases.hh 中的共享测试用例，对比 SQUAREFREE 和 FACTOR 两种 basis 计算方法。
 * 用法: make bench-sqfbasis-vs-factbasis
 */
#include <clpoly/clpoly.hh>
#include "cad_test_cases.hh"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

using namespace clpoly;
using namespace cad_test;

// 每层投影的计时数据
struct LevelTiming {
    double total_ms;           // 该层总时间
    double basis_ms;           // basis 计算时间
    double project_ms;         // 投影计算时间（不含 basis）
    size_t input_poly_count;   // 输入多项式数量
    size_t output_poly_count;  // 输出多项式数量（prims）
};

// 单次 fullproj 的计时结果
struct FullProjTiming {
    double total_ms;
    std::vector<LevelTiming> levels;
};

/**
 * @brief 带计时的 project_full 实现
 * ⚠️  WARNING: 此函数是 cad.hh 中 __project_full 的副本，添加了计时逻辑。
 *     若 __project_full 的算法逻辑发生变更，必须同步更新此函数，否则
 *     benchmark 结果将失去意义。
 *     维护时请对照 cad.hh 中的 __project_full 逐行检查。
 */
template <class var_order>
FullProjTiming project_full_with_timing(
    const std::vector<polynomial_<ZZ,lex_<var_order>>>& polys, 
    const std::vector<clpoly::variable>& vars, 
    projection_method method = projection_method::LAZARD,
    basis_computation_method basis_method = basis_computation_method::SQUAREFREE
)
{
    FullProjTiming timing;
    timing.total_ms = 0;
    
    auto t_start = std::chrono::high_resolution_clock::now();
    
    // 没有变量，说明全是常数，返回空
    if(vars.empty()) return timing;

    // 保存计算结果
    std::vector<std::vector<polynomial_<ZZ,lex_<var_order>>>> allprojs;
    // current_set 表示用于本层计算 cont/prim 的多项式集合（初始为输入 polys）
    std::vector<polynomial_<ZZ,lex_<var_order>>> current_set = polys;
    
    // 只需要投影前 n-1 个变量
    for(size_t i = 0; i < vars.size(); ++i)
    {
        LevelTiming level_timing;
        level_timing.total_ms = 0;
        level_timing.basis_ms = 0;
        level_timing.project_ms = 0;
        
        const auto &x = vars[i];
        level_timing.input_poly_count = current_set.size();
        
        // 层开始计时
        auto t_level_start = std::chrono::high_resolution_clock::now();
        
        // step1 计算 cont 和 prim
        auto tmp = __conts_prims_polys_var(current_set, x);
        auto conts = std::move(tmp.first);
        auto prims_raw = std::move(tmp.second);
        
        // step2: 根据 basis_method 选择使用 squarefreebasis 或 factorbasis
        auto t_basis_start = std::chrono::high_resolution_clock::now();
        
        std::vector<polynomial_<ZZ,lex_<var_order>>> basis_prims;
        if (basis_method == basis_computation_method::FACTOR) {
            auto [irr_prims, _] = factorbasis(prims_raw);
            basis_prims = std::move(irr_prims);
        } else {
            auto [sqfree_prims, _] = squarefreebasis(prims_raw);
            basis_prims = std::move(sqfree_prims);
        }
        
        auto t_basis_end = std::chrono::high_resolution_clock::now();
        level_timing.basis_ms = std::chrono::duration<double, std::milli>(t_basis_end - t_basis_start).count();
        
        // 过滤掉不含 x 的多项式，作为第l层的投影多项式
        // 同时处理 x 因子（Lazard + SQUAREFREE 模式时）
        std::vector<polynomial_<ZZ,lex_<var_order>>> prims;
        prims.reserve(basis_prims.size());
        conts.reserve(conts.size() + basis_prims.size());

        for (auto& poly : basis_prims) {
            if (is_number(poly))
                continue;

            if (get_first_var(poly) == x) {
                // Lazard 投影算子要求：若多项式含 x 因子，需将 x 单独作为一个 prim
                if (method == projection_method::LAZARD && 
                    basis_method == basis_computation_method::SQUAREFREE &&
                    __has_factor_first_var(poly))
                {
                    polynomial_<ZZ,lex_<var_order>> x_factor(poly.comp_ptr());
                    x_factor.push_back({{{x, 1}}, 1});
                    prims.push_back(std::move(x_factor));
                    
                    __divide_by_first_var(poly);
                }
                if (!is_number(poly))
                    prims.push_back(std::move(poly));
            } else {
                conts.push_back(std::move(poly));
            }
        }
        
        allprojs.push_back(std::move(prims));

        // 如果这是最后一层，不用再投影下一层
        // 注意：最后一层只需要做 cont/prim 分解和 basis，不需要投影
        if (i + 1 >= vars.size()) {
            // 最后一层的输出就是 prims 的数量（没有投影）
            level_timing.output_poly_count = allprojs.back().size();
            auto t_level_end = std::chrono::high_resolution_clock::now();
            level_timing.total_ms = std::chrono::duration<double, std::milli>(t_level_end - t_level_start).count();
            timing.levels.push_back(level_timing);
            break;
        }

        // 计算投影（不含 basis 的部分）
        auto t_project_start = std::chrono::high_resolution_clock::now();
        
        current_set = std::move(conts);
        if (method == projection_method::MCCALLUM) {
            __project_mccallum_prims_to(allprojs.back(), x, current_set);
        } else {
            __project_lazard_prims_to(allprojs.back(), x, current_set);
        }
        
        // 记录投影后的多项式数量（即下一层的输入数量）
        level_timing.output_poly_count = current_set.size();
        
        auto t_level_end = std::chrono::high_resolution_clock::now();
        level_timing.project_ms = std::chrono::duration<double, std::milli>(t_level_end - t_project_start).count();
        level_timing.total_ms = std::chrono::duration<double, std::milli>(t_level_end - t_level_start).count();
        
        timing.levels.push_back(level_timing);
    }
    
    auto t_end = std::chrono::high_resolution_clock::now();
    timing.total_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
    
    return timing;
}

// Simple fork-based runner without mallinfo2 (macOS compatible).
// The caller provides fn (e.g., warmup or benchmark). Returns 0=ok, 1=timeout, 2=crash.
template<typename Fn>
int fork_run_simple(Fn fn, double timeout_sec) {
    pid_t pid = fork();
    if (pid < 0) return 2;
    if (pid == 0) {
        fn();
        _exit(0);
    }
    auto deadline = std::chrono::steady_clock::now()
                  + std::chrono::milliseconds(static_cast<long long>(timeout_sec * 1000));
    while (true) {
        int wstatus;
        pid_t ret = waitpid(pid, &wstatus, WNOHANG);
        if (ret > 0) {
            return (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == 0) ? 0 : 2;
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
            return 1;
        }
        usleep(5000);
    }
}

/**
 * @brief 运行测试用例（支持重复，返回平均计时结果）
 */
static FullProjTiming bench_test_case(
    const TestCase& tc,
    int repeats,
    basis_computation_method basis_method
)
{
    // 转换为字典序
    lex_<custom_var_order> md(tc.vars);
    polynomial_<ZZ, lex_<custom_var_order>> p(&md);
    std::vector<polynomial_<ZZ, lex_<custom_var_order>>> polys_lex;
    polys_lex.reserve(tc.polys.size());
    for (const auto& i : tc.polys) {
        poly_convert(i, p);
        polys_lex.push_back(std::move(p));
    }
    
    // fork 预热（10秒超时）：把投影计算作为 fn 传入
    if (fork_run_simple([&]() {
            project_full_with_timing(polys_lex, tc.vars, projection_method::LAZARD, basis_method);
        }, 10.0) != 0) {
        FullProjTiming timeout_result;
        timeout_result.total_ms = -1.0;
        return timeout_result;
    }
    
    // 多次运行取平均
    FullProjTiming avg_timing;
    avg_timing.total_ms = 0;
    
    for (int r = 0; r < repeats; ++r) {
        auto timing = project_full_with_timing(polys_lex, tc.vars, projection_method::LAZARD, basis_method);
        
        if (r == 0) {
            avg_timing.levels.resize(timing.levels.size());
            for (size_t i = 0; i < timing.levels.size(); ++i) {
                avg_timing.levels[i] = timing.levels[i];
                avg_timing.levels[i].total_ms = 0;
                avg_timing.levels[i].basis_ms = 0;
                avg_timing.levels[i].project_ms = 0;
            }
        }
        
        avg_timing.total_ms += timing.total_ms;
        for (size_t i = 0; i < timing.levels.size(); ++i) {
            avg_timing.levels[i].total_ms += timing.levels[i].total_ms;
            avg_timing.levels[i].basis_ms += timing.levels[i].basis_ms;
            avg_timing.levels[i].project_ms += timing.levels[i].project_ms;
        }
    }
    
    // 取平均
    avg_timing.total_ms /= repeats;
    for (auto& level : avg_timing.levels) {
        level.total_ms /= repeats;
        level.basis_ms /= repeats;
        level.project_ms /= repeats;
    }
    
    return avg_timing;
}

// ANSI 颜色代码
#define BOLD "\033[1m"
#define RESET "\033[0m"

/**
 * @brief 打印带可选加粗的时间
 */
static void print_time(double time, bool bold) {
    if (bold) std::cout << BOLD;
    std::cout << std::fixed << std::setprecision(2) << time;
    if (bold) std::cout << RESET;
}

/**
 * @brief 同时打印 SQUAREFREE 和 FACTOR 两种方法的结果
 * 每层 basis/proj 时间较少的会加粗显示
 */
static void print_comparison_result(const TestCase& tc, 
                                    const FullProjTiming& timing_sqf,
                                    const FullProjTiming& timing_fac)
{
    // 计算 basis 占比
    double total_basis_ms_sqf = 0, total_basis_ms_fac = 0;
    for (const auto& level : timing_sqf.levels) total_basis_ms_sqf += level.basis_ms;
    for (const auto& level : timing_fac.levels) total_basis_ms_fac += level.basis_ms;
    double basis_ratio_sqf = (timing_sqf.total_ms > 0) ? (total_basis_ms_sqf / timing_sqf.total_ms * 100.0) : 0;
    double basis_ratio_fac = (timing_fac.total_ms > 0) ? (total_basis_ms_fac / timing_fac.total_ms * 100.0) : 0;

    bool sqf_timeout = timing_sqf.total_ms < 0;
    bool fac_timeout = timing_fac.total_ms < 0;

    // 第一行：SQUAREFREE
    int max_deg = 0;
    for (const auto& p : tc.polys) {
        max_deg = std::max(max_deg, static_cast<int>(degree(p)));
    }
    std::cout << std::left << std::setw(20) << tc.name
              << std::setw(1) << tc.vars.size() << " "
              << std::setw(1) << tc.polys.size() << " "
              << std::setw(3) << max_deg << " ";

    if (sqf_timeout) {
        std::cout << std::right << std::setw(5) << "TIMEOUT";
        std::cout << " " << std::setw(5) << "—" << "% SQ  |";
    } else {
        bool sqf_total_faster = !fac_timeout && timing_sqf.total_ms < timing_fac.total_ms;
        std::cout << std::right << std::setw(5);
        print_time(timing_sqf.total_ms, sqf_total_faster);
        std::cout << " " << std::setw(5) << std::setprecision(1) << basis_ratio_sqf << "% SQ  |";
    }
    
    // 每层详情：basis/proj/total
    for (size_t i = 0; i < timing_sqf.levels.size(); ++i) {
        const auto& lvl = timing_sqf.levels[i];
        double fac_basis = (i < timing_fac.levels.size()) ? timing_fac.levels[i].basis_ms : -1;
        double fac_proj = (i < timing_fac.levels.size()) ? timing_fac.levels[i].project_ms : -1;
        
        bool basis_bold = (fac_basis < 0) || (lvl.basis_ms < fac_basis);
        bool proj_bold = (fac_proj < 0) || (lvl.project_ms < fac_proj);
        
        std::cout << " L" << i << ":" << lvl.input_poly_count << "→" << lvl.output_poly_count
                  << " B=";
        print_time(lvl.basis_ms, basis_bold);
        std::cout << "/P=";
        print_time(lvl.project_ms, proj_bold);
        std::cout << "/T=" << std::fixed << std::setprecision(2) << lvl.total_ms;
    }
    std::cout << "\n";
    
    // 第二行：FACTOR
    std::cout << std::setw(20) << "" 
              << std::setw(1) << "" << " "
              << std::setw(1) << "" << " "
              << std::setw(3) << "" << " ";

    if (fac_timeout) {
        std::cout << std::right << std::setw(5) << "TIMEOUT";
        std::cout << " " << std::setw(5) << "—" << "% FAC |";
    } else {
        bool fac_total_faster = !sqf_timeout && timing_fac.total_ms < timing_sqf.total_ms;
        std::cout << std::right << std::setw(5);
        print_time(timing_fac.total_ms, fac_total_faster);
        std::cout << " " << std::setw(5) << std::setprecision(1) << basis_ratio_fac << "% FAC |";
    }
    
    // 每层详情
    for (size_t i = 0; i < timing_fac.levels.size(); ++i) {
        const auto& lvl = timing_fac.levels[i];
        // 获取对应层的 SQUAREFREE 数据
        double sqf_basis = (i < timing_sqf.levels.size()) ? timing_sqf.levels[i].basis_ms : -1;
        double sqf_proj = (i < timing_sqf.levels.size()) ? timing_sqf.levels[i].project_ms : -1;
        
        bool basis_bold = (sqf_basis < 0) || (lvl.basis_ms < sqf_basis);
        bool proj_bold = (sqf_proj < 0) || (lvl.project_ms < sqf_proj);
        
        std::cout << " L" << i << ":" << lvl.input_poly_count << "→" << lvl.output_poly_count
                  << " B=";
        print_time(lvl.basis_ms, basis_bold);
        std::cout << "/P=";
        print_time(lvl.project_ms, proj_bold);
        std::cout << "/T=" << std::fixed << std::setprecision(2) << lvl.total_ms;
    }
    std::cout << "\n";
    
    // 分隔线
    std::cout << std::string(110, '-') << "\n";
}

// 运行单个测试用例并打印结果
static void run_and_print(const TestCase& tc, int repeats) {
    auto timing_sqf = bench_test_case(tc, repeats, basis_computation_method::SQUAREFREE);
    auto timing_fac = bench_test_case(tc, repeats, basis_computation_method::FACTOR);
    print_comparison_result(tc, timing_sqf, timing_fac);
}

// ========== 实验配置 ==========

// 非随机测试用例配置（通过 TestCaseConfig + make_test_case 统一构建）
static std::vector<TestCaseConfig> get_experiment_plan() {
    return {
        // Lee 2025
        {"hypersphere", {1}, 10, true},
        {"hypersphere", {2}, 10, true},
        {"hypersphere", {3}, 10, true},
        {"hypersphere", {4}, 10, true},

        // Chen 2023
        {"five_var",    {},  10, true},
        {"hong",        {4}, 10, true},
        {"hong",        {5}, 10, true},
        {"hong",        {6}, 10, true},
        {"hong",        {7}, 10, false},
        {"hong",        {8}, 10, false},
        {"cyclic",      {4}, 10, true},
        {"cyclic",      {5}, 10, false},
        {"cyclic",      {6}, 10, false},
        {"ebd",         {2}, 10, true},
        {"ebd",         {5}, 10, true},
        {"ebd",         {8}, 10, true},
        {"fis",         {1}, 10, true},
        {"fis",         {2}, 10, true},
        {"fis",         {3}, 10, true},
        {"fis",         {4}, 10, true},
        {"fis",         {5}, 10, false},  // 超时
        {"fis",         {6}, 10, true},
        {"fis",         {7}, 10, false},  // 超时
        {"fis",         {8}, 10, false},  // 超时

        // Li 2020
        {"han",         {3}, 10, true},
        {"han",         {4}, 10, true},
        {"han",         {5}, 10, false},
        {"han",         {6}, 10, false},
        {"p",           {},  10, false},
        {"hong",        {10}, 10, false},
        {"hong",        {11}, 10, false},
        {"hong2",       {11}, 10, false},
        {"hong2",       {12}, 10, false},
        {"cnr",         {3, 1}, 10, false},   // 超时
        {"cnr",         {3, 32}, 10, false},  // 超时
        {"cnr",         {3, 63}, 10, false},  // 超时
        {"cnr",         {3, 64}, 10, false},  // 超时
        {"cnr",         {4, 1}, 10, false},   // 超时
        {"cnr",         {4, 32}, 10, false},  // 超时
        {"cnr",         {4, 63}, 10, false},  // 超时
        {"cnr",         {4, 64}, 10, false},  // 超时
        {"cnr",         {5, 1}, 10, false},   // 超时
        {"cnr",         {5, 32}, 10, false},  // 超时
        {"cnr",         {5, 63}, 10, false},  // 超时
        {"cnr",         {5, 64}, 10, false},  // 超时
    };
}

// 随机测试配置（统一通过 TestCaseConfig + make_test_case 构建）
static std::vector<TestCaseConfig> get_random_configs() {
    return {
        // 2变量
        {"random", {2, 1, 3, 4}, 100, true},
        {"random", {2, 2, 3, 4}, 100, true},
        {"random", {2, 2, 4, 5}, 50, true},
        {"random", {2, 3, 4, 5}, 50, true},
        {"random", {2, 3, 5, 6}, 20, true},
        {"random", {2, 3, 6, 8}, 10, true},
        {"random", {2, 5, 4, 5}, 10, true},
        {"random", {2, 3, 8, 10}, 3, true},
        {"random", {2, 5, 6, 8}, 3, false},  // 超时
        
        // 3变量
        {"random", {3, 1, 3, 5}, 50, true},
        {"random", {3, 2, 3, 5}, 20, true},
        {"random", {3, 2, 4, 6}, 5, true},
        {"random", {3, 3, 3, 5}, 10, true},
        {"random", {3, 3, 4, 6}, 3, false},  // 超时
        {"random", {3, 2, 5, 7}, 3, false},  // 超时
        {"random", {3, 4, 4, 6}, 3, false},  // 超时
        
        // 4变量
        {"random", {4, 1, 3, 5}, 20, true},
        {"random", {4, 2, 3, 5}, 10, true},
        {"random", {4, 3, 3, 5}, 5, false},  // 超时
        {"random", {4, 1, 4, 6}, 5, false},  // 超时
        
        // 5变量
        {"random", {5, 1, 3, 5}, 10, true},
        {"random", {5, 2, 3, 5}, 3, false},  // 超时
    };
}

int main()
{
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "=== Fullproj Basis Time Scaling ===\n";
#ifdef NDEBUG
    std::cout << "Build: Release\n\n";
#else
    std::cout << "Build: Debug (timings not representative)\n\n";
#endif

    // 打印表头
    std::cout << "TestCase             V P Deg Total Basis%  Type | Level breakdown (In→Out polys, B=basis/P=proj/T=total ms)\n";
    std::cout << std::string(110, '-') << "\n";

    // ========== 非随机测试用例（统一通过 TestCaseConfig 构建） ==========
    for (const auto& cfg : get_experiment_plan()) {
        if (!cfg.enabled) continue;
        run_and_print(make_test_case(cfg), cfg.repeats);
    }

    // ========== 随机测试用例 ==========
    for (const auto& cfg : get_random_configs()) {
        if (!cfg.enabled) continue;
        run_and_print(make_test_case(cfg), cfg.repeats);
    }

    std::cout << "\n";
    return 0;
}
