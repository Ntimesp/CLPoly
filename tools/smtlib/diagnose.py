"""在临时头文件中记录 CAD 进展／活动递归重复输入，不修改生产实现。"""
import argparse
import importlib.util
import json
import subprocess
from pathlib import Path

import benchmark

ROOT = Path(__file__).resolve().parents[2]
CYCLE_HELPER = r'''
#include <stdexcept>
namespace three_diag {
inline std::vector<std::pair<std::string,std::string>> active_inputs;

// 比较的是完整输入串及比较器地址，不以哈希碰撞判断循环。
template<class Chain> std::string chain_key(const Chain &chain)
{
    std::ostringstream out;
    out << chain.size() << ':';
    if (!chain.empty()) out << static_cast<const void*>(chain.front().comp_ptr());
    for (const auto &p : chain) out << '{' << p << '}';
    return out.str();
}

struct InputScope
{
    InputScope(const char *name, std::string key)
    {
        const std::pair<std::string,std::string> current(name, std::move(key));
        if (std::find(active_inputs.begin(), active_inputs.end(), current) != active_inputs.end())
        {
            std::cerr << "REPEATED-ACTIVE-INPUT " << current.first << " " << current.second << std::endl;
            throw std::logic_error("diagnostic: repeated active recursive input");
        }
        active_inputs.push_back(current);
    }
    ~InputScope() { active_inputs.pop_back(); }
};

template<class Boxes> bool same_boxes(const Boxes &a, const Boxes &b)
{
    if (a.size() != b.size()) return false;
    for (size_t i=0; i<a.size(); ++i)
        if (a[i].left()!=b[i].left() || a[i].right()!=b[i].right()) return false;
    return true;
}
}
'''


def instrument(text, marker, body):
    if text.count(marker) != 1:
        raise ValueError('expected unique marker: ' + marker)
    start = text.index('{', text.index(marker)) + 1
    return text[:start] + '\n' + body + text[start:]


def prepare(out):
    out.mkdir(parents=True, exist_ok=True)
    path = ROOT / 'docs/cad/reports/three-datasets/profile.py'
    spec = importlib.util.spec_from_file_location('cad_profile', path)
    profile = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(profile)
    # 复用已验证的临时插桩头生成器（同时构建其原诊断驱动）。
    profile.prepare(out)
    include = out / 'profile-include'
    helper = include / 'three_diag.hh'
    helper.write_text(helper.read_text() + CYCLE_HELPER)
    path = include / 'clpoly/charset.hh'
    text = path.read_text()
    text = instrument(text, ' RCtoSqrfree\n',
                      '        three_diag::InputScope recursive_input("RCtoSqrfree", three_diag::chain_key(T));\n')
    text = instrument(text, '> wrsd\n',
                      '        three_diag::Scope wrsd_scope("WRSD");\n'
                      '        std::ostringstream query; query << three_diag::chain_key(T) << "f{" << f << "}";\n'
                      '        three_diag::InputScope recursive_input("WRSD", query.str());\n')
    path.write_text(text)
    path = include / 'clpoly/real_algebraic.hh'
    text = path.read_text()
    text = profile.replace(text,
        '    while (a.max_width() >= epsilon)\n        shrink(a);',
        '    while (a.max_width() >= epsilon)\n    {\n'
        '        three_diag::set("refine_width_den_bits",a.max_width().den().sizeinbase(2));\n'
        '        three_diag::set("refine_target_den_bits",epsilon.den().sizeinbase(2));\n'
        '        auto before = a.box();\n        shrink(a);\n'
        '        if(three_diag::same_boxes(before,a.box()))\n        {\n'
        '            std::cerr << "STALLED-REFINE exact unchanged box" << std::endl;\n'
        '            throw std::logic_error("diagnostic: unchanged refinement state");\n'
        '        }\n    }')
    path.write_text(text)
    args = ['g++-13', '-std=c++17', '-O2', '-DNDEBUG', '-include', str(helper), '-I'+str(include), '-I'+str(ROOT),
            '-I/opt/homebrew/opt/gmp/include', '-I/opt/homebrew/opt/boost/include', str(ROOT/'tools/clpoly-cad.cc'),
            str(ROOT/'lib/libclpoly.a'), '-L/opt/homebrew/opt/gmp/lib', '-lgmpxx', '-lgmp', '-pthread',
            '-o', str(out/'clpoly-cad-diagnostic')]
    subprocess.run(args, check=True)
    source = out / 'cycle-probe.cc'
    source.write_text('''#include <clpoly/clpoly.hh>
int main() {
 using namespace clpoly;
 variable x("x"),y("y"),z("z");
 const regular_chain::order_type order(custom_var_order({z,y,x}));
 regular_chain chain({x,y,z},std::vector<polynomial_ZZ>{x*x,pow(y,3),z*z+x*y},&order);
 try { squarefree_decomposition(chain); }
 catch(const std::logic_error &e) { std::cerr << e.what() << std::endl;return 1; }
 return 0;
}
''')
    args[-2:] = ['-o', str(out/'cycle-probe')]
    args[args.index(str(ROOT/'tools/clpoly-cad.cc'))] = str(source)
    subprocess.run(args, check=True)
    probe = benchmark.run_process([str(out/'cycle-probe')], 10)
    benchmark.dump(out/'cycle-probe.json', probe)
    if 'REPEATED-ACTIVE-INPUT RCtoSqrfree' not in probe['stderr']:
        raise ValueError('known recursive cycle was not detected')
    print('known baseline cycle detector: PASS', flush=True)


def run(out, txt, timeout):
    result = benchmark.run_process([str(out/'clpoly-cad-diagnostic'), str(txt)], timeout)
    result['repeated_active_input'] = 'REPEATED-ACTIVE-INPUT' in result['stderr']
    result['unchanged_refinement_state'] = 'STALLED-REFINE' in result['stderr']
    result['snapshots'] = [line for line in result['stderr'].splitlines()
                           if line.startswith(('PATH', 'CONTEXT', 'METRICS'))]
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--prepare', action='store_true')
    parser.add_argument('--input', type=Path)
    parser.add_argument('--timeout', type=float, default=10)
    args = parser.parse_args()
    if args.prepare: prepare(args.out)
    if args.input:
        result = run(args.out, args.input, args.timeout)
        benchmark.dump(args.out/'diagnosis.json', result)
        print(json.dumps({k: result[k] for k in ['status','wall_seconds','repeated_active_input','unchanged_refinement_state']}))
