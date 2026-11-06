# SMT-LIB QF_NRA 对比工具

本工具使用下载的官方公开例子，不使用原数学题集转换得到的 SMT 输入。原始文件、来源、选择规则与求解器调用全部保留；当前 CLPoly 通过独立适配器读取它能精确表示的多项式子集。

## 独立转换后调用原生程序

```sh
python3 tools/smtlib/adapter.py problem.smt2 --output problem.txt --mapping problem.names.json
_build/release/bin/clpoly-cad problem.txt
z3 -smt2 problem.smt2
```

`problem.txt` 是变量已改名的现有三段式格式；`problem.names.json` 保存原名→别名及源／目标哈希。转换完成并落盘后再启动 CAD。下面的后台设施负责两个原生调用各 10 秒的墙钟限时、等价核对和记录；上面三条命令仅演示分离的输入流程。

## 全量后台实验：先转换，再原生计时

最终计时采用 `full_benchmark.py`，不把旧 100 例中“Python 适配器 + CAD”的整段耗时当作原生 CAD 时间。全量包含官方归档的全部 12154 个 SMT2；逐系列轮询、系列内按源文件大小及固定哈希排序，便于较早取得可核对答案，不删除难例或根据结果挑选题目。

每例分四个独立阶段：

1. 解析 SMT2，先保存改名后的三段式 TXT、原变量→`v0/v1/...` 映射、源／目标 SHA256；用冻结的 CLI `--check` 验证格式。转换与格式验证最多 10 秒，失败、超限和 `conversion_timeout` 独立记录，绝不是 CAD 求解超时。
2. 正式求解计时前，用独立 Z3 XOR 检查实际 TXT 与原 SMT2 等价，最多 10 秒；失败或未知单列。当前保守策略只把等价已建立的 TXT 交给 CAD，不能把 `not_run` 记成 CAD 无能力或超时。
3. Z3 原生进程读取原 SMT2，单次墙钟限时 10 秒。
4. CAD 原生进程直接读取已落盘的 TXT，单次墙钟限时 10 秒。其时间只含原生进程启动、TXT 读取及求解，不含 Python 转换、格式预检或等价核对。两个原生求解器按例交替先后、串行运行。

`prepare` 校验归档 MD5、预先统计解压体积和磁盘余量，流式安全提取；冻结 CLI、适配器、比较设施和验证脚本，并记录 Z3 二进制哈希。生产库不变。后台 worker 与其启动会话分离；每次调用有独立超时 supervisor，worker 暂时退出也不会让该次 solver 无期限遗留。

```sh
python3 tools/smtlib/full_benchmark.py --stage prepare --archive /private/tmp/QF_NRA.tar.zst --out /private/tmp/clpoly-smt-full
python3 tools/smtlib/full_benchmark.py --stage start --out /private/tmp/clpoly-smt-full --paused
python3 tools/smtlib/full_benchmark.py --stage status --out /private/tmp/clpoly-smt-full
python3 tools/smtlib/full_benchmark.py --stage resume --out /private/tmp/clpoly-smt-full
python3 tools/smtlib/full_benchmark.py --stage pause --out /private/tmp/clpoly-smt-full
```

暂停在调用自然边界生效，观察 `status.json` 的 `state=paused` 后已无活跃求解调用，可进行小规模正确性实验或插桩诊断而不争用求解 CPU。`stop` 同样在边界退出；之后用 `start` 恢复。所有源文件及产物保存在独立输出目录，后续工作区修改不影响冻结实验。

`cad_faster_unsat` 只表示这次共同 UNSAT 调用中 CAD 耗时较小的候选，不是稳定速度结论；尤其有同时进行的诊断时，需在诊断结束后独立串行复测确认。

`events.jsonl` 逐次追加、flush/fsync，包含开始、结果和告警；恢复时保留未写完整的尾部片段，并只补做未完成阶段。`status.json` 原子更新当前阶段，`summary.json` 给出真实已处理范围、共同确定答案数、冲突、异常和 CAD 更快的 UNSAT。`complete_cases` 是四阶段均已记录的处理数，不等于全部实际求解成功。每个尝试的 stdout、stderr、返回码和时长另存于 `attempts/`；映射位于 `mappings/`。

```sh
python3 tools/smtlib/test_full_benchmark.py
```

设施测试验证落盘／映射、转换超时分类、JSONL 恢复、supervisor 独立生存、整组超时回收及汇总语义。全量实验是长期任务；不能把中间样本或已启动状态说成全量完成，10 秒未完成也不能直接判定死循环。

## 官方来源与 100 例探索抽样

源数据为 [SMT-LIB 官方基准入口](https://smt-lib.org/benchmarks.shtml)所列的 [2025.08.04 非增量发布](https://zenodo.org/records/16740866)，逻辑 `QF_NRA`。官方 `QF_NRA.tar.zst` 为 210801606 字节，MD5 为 `85a04fcea6dd10fb4c505703cac46e2c`，工具先核对 MD5，再记录 SHA256。

归档内 12154 个例子分属 16 个顶层系列。默认先按系列名排序，再将各系列的文件按 `SHA256("20261106:" + 原始路径)` 排序，逐轮从各系列取一个，直到 100 个。选择不使用状态标签、变量数、文件大小、转换是否成功或求解表现；因此包含大文件、不支持内容与难例，不将它们悄悄换成简单题。

```sh
curl -L --fail 'https://zenodo.org/records/16740866/files/QF_NRA.tar.zst?download=1' -o /private/tmp/QF_NRA.tar.zst
python3 tools/smtlib/benchmark.py --stage select --archive /private/tmp/QF_NRA.tar.zst --out /private/tmp/clpoly-smt-run --count 100 --seed 20261106
```

抽取使用 `zstd` 流式读取，不展开整个基准库。`selection.json` 保存总体计数、抽样规则和每个原始文件的路径、大小、SHA256。原始文件不改写，保留各文件的作者、来源和许可字段。

## 输入适配与正确性边界

`adapter.py` 仅依赖 Python 标准库，不导入 Z3。它独立解析 S 表达式，将实数变量改成安全别名，转换 QQ 多项式和布尔 CNF，交给现有原生 `clpoly-cad`。没有线性消元、变量求解、Z3 化简或使用已知答案决定输出。

支持实常量声明、无参数 Real／Bool 定义、词法 `let`（绑定同时解释）、注释、引号符号、精确整数／十进制、加减乘、非负整数幂、非零有理常量除法，及 `and/or/not/=>`、布尔 `ite`、实数比较和 `distinct`。链式比较按相邻项解释，`distinct` 按两两不等解释；否定使用德摩根律，非严格关系和不等式精确转换成 `=/>/<` 的析取。CNF 分配保持逻辑等价，不引入用于求解的额外布尔编码变量；无原变量时只添加一个不受约束的实变量以满足现有 TXT 格式。

非零常量除法限制依据 [SMT-LIB Reals 理论](https://smt-lib.org/theories-Reals.shtml)：实除法是总函数，在零分母处未指定，不能擅自清除符号分母或排除其零分支。词法与 `let` 的依据为 [SMT-LIB 2.7 标准](https://smt-lib.org/papers/smt-lib-reference-v2.7-r2025-07-07.pdf) §3.1、§3.6、§4.1.6。

当前适配子集不支持 Bool／Int 自由变量、参数函数、算术 `ite`、量词、增量栈或多个检查、任意 `set-option`、符号／零分母等。转换限额为单个展开算术表达式 2000000 字符、4096 个 CNF 子句、100000 个分配文字；超限明确标为 `unsupported`，不是 SAT／UNSAT。适配不等于 CLPoly 已有完整原生 SMT-LIB 支持。

## 旧探索脚本的计时规则

```sh
make cad
python3 tools/smtlib/test_adapter.py
python3 tools/smtlib/benchmark.py --stage run --out /private/tmp/clpoly-smt-run --timeout 10 --z3 /opt/homebrew/bin/z3
python3 tools/smtlib/verify.py --out /private/tmp/clpoly-smt-run
```

每例只用声明顺序，不尝试反序重跑。两个独立进程串行运行，每例交替谁先运行，避免同时求解造成 CPU 争抢。每次调用外部墙钟限时 10 秒，超时杀死整个进程组并回收；适配器启动的 CAD 子进程同样终止。进程启动及超时回收产生少量计时开销。

Z3 读取原始 SMT-LIB；CLPoly 总耗时包含 Python 启动、SMT 解析／CNF 转换、保存转换输入、原生进程启动、TXT 读取与完整 CAD。转换耗时另记在 stderr 的 JSON 中；不将转换移到限时之外以提高 CLPoly 完成率。保存原始 stdout、stderr、返回码，区分 `sat`、`unsat`、`unknown`、`timeout`、`unsupported`、`error`，不会从超时中推测答案。

开始／结束检查二进制及实验脚本 SHA256 一致，环境记录生产版本和 Z3 版本。汇总给出所有样本、各来源系列、共同完成例数、答案冲突和标签不符；耗时比较只在两边均实际完成的交集上报告。CLPoly 构建完整空间的符号不变 CAD；Z3 执行可满足性求解，完成率和端到端时间不作为相同内部算法工作的速度比。

`verify.py` 在正式计时结束后，用 Z3 原生 SMT 解析器和独立 Python AST 解释实际保存的 TXT，对两公式 XOR 求不可满足；每次也限时 10 秒。验证过程不参与求解器决策，结果写入 `equivalence.json`。未知、错误和超时均保留，不能当作已经证明等价。

## 文件与测试

- `results.json`：每次调用的完整输出和耗时。
- `results.csv`、`summary.json`：逐题及总体／分系列汇总。
- `environment.json`：版本、二进制与脚本哈希、计时口径。
- `converted/`：限时内成功转换的实际 TXT 输入。
- `equivalence.json`：计时后的格式转换核对。

`test_adapter.py` 依赖可选 Python `z3-solver`，覆盖关系／否定／链式比较、并行与嵌套 let、定义与引号名字、精确有理数、CNF 限额及进程超时／错误分类。正式比较使用 Z3 原生可执行文件；运行 CLPoly 适配器本身不需要 Python Z3 包。

## 定向正确性与循环诊断

主要检查错误答案、错误见证、异常退出和可确认的重复状态，不从10秒超时直接推断死循环。针对性小集的复现顺序如下，转换和核对都先完成，再开始原生求解：

```sh
python3 tools/smtlib/correctness_cases.py --archive /private/tmp/QF_NRA.tar.zst --out /private/tmp/clpoly-smt-small --per-status 48
python3 tools/smtlib/verify.py --out /private/tmp/clpoly-smt-small
python3 tools/smtlib/run_correctness.py --out /private/tmp/clpoly-smt-small --timeout 10
python3 tools/smtlib/check_witness.py --out /private/tmp/clpoly-smt-small --limit 5
```

`correctness_cases.py`只按公开的规模条件、已知SAT／UNSAT类别和固定哈希选样，不用实际求解表现替换题目。`run_correctness.py`要求每个实际TXT先通过等价检查，再对两个原生程序独立限时。

`check_witness.py`把返回样本的原坐标方程与严格有理隔离盒重新交给Z3，分别检查证书非空及证书蕴含原公式；不接受空证书的真蕴含充当SAT见证。

`diagnose.py --out /private/tmp/clpoly-smt-diagnostic --prepare` 仅生成临时插桩头和诊断binary。该工具复用旧阶段计数器，另检测WRSD／RCtoSqrfree活动递归中的完全相同输入，以及REFINE迭代后完全不变的根盒。用 `--input converted.txt --timeout 10` 观察指定例子。它用已记录的底层循环反例作正控制，生产实现不修改；未触发这些检测不代表已证明所有路径终止。构建目前沿用本机GCC／Homebrew路径，其他环境需调整编译参数。

工具测试：

```sh
python3 tools/smtlib/test_adapter.py
python3 tools/smtlib/test_witness.py
python3 tools/smtlib/test_full_benchmark.py
```

本次完整记录、原始小集、TXT、映射、见证和超时诊断见[正确性报告](../../docs/cad/reports/smtlib-correctness-20261106.md)。
