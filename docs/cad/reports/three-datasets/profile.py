"""只在临时头文件副本中插桩；不修改生产代码，不作为性能基准。"""
import argparse
import json
import re
import subprocess
import time
import textwrap
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
HELPER = r'''#pragma once
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <condition_variable>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
namespace three_diag {
using Clock=std::chrono::steady_clock;
struct Frame { std::string name; Clock::time_point start; };
struct Metric { size_t calls=0; double seconds=0; };
inline std::mutex mutex;
inline std::vector<Frame> stack;
inline std::map<std::string,Metric> metrics;
inline std::map<std::string,std::string> context;
struct Scope {
 const char *name; Clock::time_point start=Clock::now();
 Scope(const char *n):name(n) { std::lock_guard<std::mutex> lock(mutex);stack.push_back({name,start});++metrics[name].calls; }
 ~Scope() { auto now=Clock::now();std::lock_guard<std::mutex> lock(mutex);metrics[name].seconds+=std::chrono::duration<double>(now-start).count();stack.pop_back(); }
};
template<class T> void set(const char *key,const T &v) { std::ostringstream s;s<<v;std::lock_guard<std::mutex> lock(mutex);context[key]=s.str(); }
template<class Poly> size_t bits(const Poly &p) {size_t n=0;for(const auto &t:p)n=std::max(n,t.second.sizeinbase(2));return n;}
template<class U> size_t ubits(const U &p) {size_t n=0;for(const auto &q:p)n=std::max(n,std::max(q.num().sizeinbase(2),q.den().sizeinbase(2)));return n;}
inline void dump() {
 auto now=Clock::now();std::lock_guard<std::mutex> lock(mutex);
 std::cerr<<"PATH";for(auto &f:stack)std::cerr<<" "<<f.name<<":"<<std::chrono::duration<double>(now-f.start).count();std::cerr<<"\n";
 std::cerr<<"METRICS";for(auto &[k,v]:metrics)std::cerr<<" "<<k<<":"<<v.calls<<":"<<v.seconds;std::cerr<<"\n";
 std::cerr<<"CONTEXT";for(auto &[k,v]:context)std::cerr<<" "<<k<<":"<<v;std::cerr<<std::endl;
}
// 监控对象最后构造、最先销毁，退出时唤醒并 join，随后才能销毁共享状态。
struct Monitor
{
    std::condition_variable wake;
    bool stop = false;
    std::thread worker;

    Monitor() : worker([this]() {
        std::unique_lock<std::mutex> lock(mutex);
        while (!wake.wait_for(lock, std::chrono::seconds(1), [this]() { return stop; }))
        {
            lock.unlock();
            dump();
            lock.lock();
        }
    }) {}

    ~Monitor()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            stop = true;
        }
        wake.notify_one();
        worker.join();
    }
};
inline Monitor monitor;
}
'''


def replace(text, old, new):
    # 匹配相对缩进；每个插桩点必须唯一，源码变动不能悄悄插到另一处。
    lines = textwrap.dedent(old).splitlines()
    pattern = r'(?m)^(?P<pad>[ \t]*)' + re.escape(lines[0])
    pattern += ''.join(r'\n(?P=pad)' + re.escape(line) for line in lines[1:])
    pattern += r'(?=\n|$)'
    matches = list(re.finditer(pattern, text))
    if len(matches) != 1:
        raise ValueError(f'expected one instrumentation site: {old!r}, found {len(matches)}')
    match = matches[0]
    replacement = textwrap.indent(textwrap.dedent(new), match['pad'])
    return text[:match.start()] + replacement + text[match.end():]


def scope(text, marker, label):
    if text.count(marker) != 1:
        raise ValueError(f'expected one scope marker: {marker!r}')
    pos = text.index(marker)
    body = text.index('{', pos)
    return text[:body + 1] + f'\n    three_diag::Scope diag_scope("{label}");' + text[body + 1:]


def prepare(out):
    include = out / 'profile-include'
    (include / 'clpoly').mkdir(parents=True, exist_ok=True)
    (include / 'three_diag.hh').write_text(HELPER)
    s = (ROOT / 'clpoly/full_cad.hh').read_text()
    s = '#include <three_diag.hh>\n' + s
    s = scope(s, 'inline void __lift_stack(', 'LIFT-STACK')
    s = replace(s, '    st.roots = __roots_over(product, alpha, result.level_var(level));',
                '    three_diag::set("product_degree",degree(product,result.level_var(level)));\n'
                '    three_diag::set("product_terms",product.size());\n'
                '    st.roots = __roots_over(product, alpha, result.level_var(level));')
    s = replace(s, '    auto projection =\n        __project_full(polys, vars, projection_method::LAZARD, basis_computation_method::FACTOR);',
                '    auto projection = [&](){three_diag::Scope diag_scope("PROJECT");return __project_full(polys,vars,projection_method::LAZARD,basis_computation_method::FACTOR);}();')
    s = replace(s, '        for (size_t j = 0; j < parents; ++j)\n        {',
                '        for (size_t j = 0; j < parents; ++j)\n        {\n'
                '            three_diag::set("level",level);three_diag::set("parent",j);three_diag::set("parents",parents);')
    (include / 'clpoly/full_cad.hh').write_text(s)

    s = (ROOT / 'clpoly/real_algebraic.hh').read_text()
    for marker, label in [('inline bool squarefree(', 'SQUAREFREE'), ('inline std::vector<I> __univariate(', 'UNI'),
                          ('inline void shrink(real_algebraic_point &a)\n', 'NSHR'),
                          ('inline std::vector<I> lift(', 'XZ-LIFT'), ('inline bool __point_zero(', 'POINT-ZERO'),
                          ('inline real_algebraic_point reencode_point(', 'REENCODE'),
                          ('inline std::vector<real_algebraic_point> __roots_over(', 'ROOTS')]:
        s = scope(s, marker, label)
    s = replace(s, '    auto roots = uspensky(z);',
                '    auto roots = [&](){three_diag::Scope diag_scope("USPENSKY");return uspensky(z);}();')
    s = replace(s, '    QQ epsilon(1);\n    while (true)', '    size_t diag_iteration=0;\n    QQ epsilon(1);\n    while (true)')
    s = replace(s, '        epsilon /= QQ(2);\n        auto b = bounds(q, v, vars, a.box());',
                '        ++diag_iteration;three_diag::set("xz_iteration",diag_iteration);\n'
                '        three_diag::set("xz_degree",degree(f,v));\n'
                '        epsilon /= QQ(2);\n        auto b = bounds(q, v, vars, a.box());\n'
                '        three_diag::set("bound_bits",std::max(three_diag::ubits(b.first),three_diag::ubits(b.second)));')
    (include / 'clpoly/real_algebraic.hh').write_text(s)

    s = (ROOT / 'clpoly/regular_chain.hh').read_text()
    s = scope(s, 'inline regular_chain_split __weak_split_zero(', 'TRIANGULAR-SPLIT')
    s = scope(s, 'inline std::vector<regular_chain> squarefree_decomposition(', 'SF-DECOMPOSE')
    s = scope(s, 'regular_chain(std::vector<variable> vars,', 'CHAIN-VALIDATE')
    (include / 'clpoly/regular_chain.hh').write_text(s)

    s = (ROOT / 'clpoly/charset.hh').read_text()
    s = scope(s, ' RCtoSqrfree\n', 'RCtoSqrfree')
    (include / 'clpoly/charset.hh').write_text(s)
    s = (ROOT / 'clpoly/lazard_qq.hh').read_text()
    s = scope(s, 'inline void __lazard_evaluate_qq(', 'LAZARD')
    (include / 'clpoly/lazard_qq.hh').write_text(s)

    s = (ROOT / 'clpoly/polynomial_gcd.hh').read_text()
    pos = s.index('    // 扩展 GCD (ZZ): c = polynomial_GCD(F, G, s, t)')
    part = s[pos:]
    part = scope(part, 'inline upolynomial_<ZZ> polynomial_GCD(', 'EXTENDED-GCD')
    part = replace(part, '        while (!r1.empty())\n        {',
                   '        while (!r1.empty())\n        {\n'
                   '            three_diag::set("gcd_bits",std::max(three_diag::bits(r0),three_diag::bits(r1)));')
    part = replace(part, '            // Bézout 更新: s2 = lc_pow * s0 - q * s1, t2 同理',
                   '            three_diag::Scope bezout_scope("BEZOUT-UPDATE");\n'
                   '            // Bézout 更新: s2 = lc_pow * s0 - q * s1, t2 同理')
    (include / 'clpoly/polynomial_gcd.hh').write_text(s[:pos] + part)
    driver = (Path(__file__).parent / 'driver.cc').read_text()
    driver = replace(driver, '        const auto end = std::chrono::steady_clock::now();',
                            '        three_diag::dump();\n        const auto end = std::chrono::steady_clock::now();')
    (out / 'profile-driver.cc').write_text(driver)
    args = ['g++-13', '-std=c++17', '-O2', '-DNDEBUG', '-I' + str(include), '-I' + str(ROOT),
            '-I/opt/homebrew/opt/gmp/include', '-I/opt/homebrew/opt/boost/include', str(out / 'profile-driver.cc'),
            str(ROOT / 'lib/debug/libclpoly.a'), '-L/opt/homebrew/opt/gmp/lib', '-lgmpxx', '-lgmp',
            '-pthread', '-o', str(out / 'profile-driver')]
    subprocess.run(args, check=True)


def main(out, limit):
    # 正式计时全部结束后，才运行带插桩的诊断副本。
    for _ in range(3600):
        if (out / 'table.json').exists():
            break
        time.sleep(2)
    else:
        raise RuntimeError('benchmark did not finish')
    prepare(out)
    table = json.loads((out / 'table.json').read_text())
    jobs = [(c['id'], mode) for mode in ['raw', 'reduced'] for c in table
            if c.get(mode) not in ['sat', 'unsat']]
    results = []
    for ident, mode in jobs:
        log_path = out / f'profile-{ident}-{mode}.log'
        args = [str(out / 'profile-driver'), str(out / 'inputs' / f'{ident}-{mode}.cad')]
        start = time.monotonic()
        with log_path.open('w') as log:
            try:
                p = subprocess.run(args, stdout=log, stderr=log, timeout=limit)
                status = {'returncode': p.returncode}
            except subprocess.TimeoutExpired:
                status = {'timeout_seconds': limit}
        lines = log_path.read_text().splitlines()
        row = {'id': ident, 'mode': mode, **status, 'wall_seconds': time.monotonic() - start}
        for label in ['PATH', 'METRICS', 'CONTEXT']:
            found = [s for s in lines if s.startswith(label)]
            row[label.lower()] = found[-1] if found else None
        results.append(row)
        (out / 'profiles.json').write_text(json.dumps(results, ensure_ascii=False, indent=2) + '\n')
        print(ident, mode, row['path'], row['context'], flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--timeout', type=float, default=10)
    args = parser.parse_args()
    main(args.out, args.timeout)
