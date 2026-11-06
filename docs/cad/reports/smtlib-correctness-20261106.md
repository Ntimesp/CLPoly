# 官方 SMT-LIB：CAD 正确性与终止诊断

日期：2026-11-06。生产 CLI 为 `441db70` 的 `_build/release/bin/clpoly-cad`，Z3 为本机 5.1.0。本次只增加外部实验工具、检查和记录，没有修改生产库或已知底层问题。

## 最终实验流程

从 [SMT-LIB 官方入口](https://smt-lib.org/benchmarks.shtml)下载 [2025.08.04 非增量发布](https://zenodo.org/records/16740866) 的 QF_NRA 原库，归档 210801606 字节，官方 MD5 为 `85a04fcea6dd10fb4c505703cac46e2c`，实测 SHA256 为 `e3a74ec7507010bde1474eeaecb3890204fead66236c56864c6137617899d23e`。共有 12154 个 SMT2、16 个系列，原文件合计 3451618165 字节，全部已安全提取。

按用户要求，先由独立 parser 将实变量改名为 `v0,v1,…`，生成与现有题集相同的三段式 TXT，同时保存名称映射、原始文件和 TXT 哈希；再独立核对语义等价。之后 CAD 直接读取已经落盘的 TXT，Z3 读取原始 SMT2，每个原生调用墙钟限时 10 秒。转换／核对耗时不计入 CAD 求解限时，也不把转换失败记作 CAD 超时。

```sh
python3 tools/smtlib/adapter.py question.smt2 --output question.txt --mapping question.names.json
_build/release/bin/clpoly-cad question.txt
z3 -smt2 question.smt2
```

批量 runner 负责外部10秒限时、整个进程组的回收、调用输出及退出码。已知状态只作事后对照，不决定求解结果。原始 SMT 的语义超过多项式适配子集时明确记为不支持；不擅自清除符号分母的零分支。支持范围和标准依据见[工具说明](../../../tools/smtlib/README.md)。

## 已完成的定向正确性集

从全库中筛得 4428 个可精确适配的小规模候选，按固定路径哈希和系列轮转选择 SAT／UNSAT 各 48 例，共96例。扫描先以声明文本出现次数至多8作预筛；正式条件按解析结果检查：1–3个实变量、至多8个多项式、16个子句、源文件至多16KiB、单个转换算术表达式至多2000字符。选择不使用实际求解表现，不替换超时例；这是一组定向正确性样本，不代表全库的性能分布。

96个实际落盘 TXT 与原 SMT2 的 XOR 检查全部为不可满足。随后两个原生程序串行运行，每例交替顺序，结果如下：

| 结果 | CAD | Z3 |
|---|---:|---:|
| SAT | 33 | 48 |
| UNSAT | 16 | 46 |
| 10秒超时 | 47 | 2 |
| 异常退出 | 0 | 0 |

两边共同确定的49例全部一致，且符合已知标签。未观察到本集中的错误答案；47个CAD超时例仍未完成，不能将其视作正确或错误答案。

又选取前5个已完成SAT例，请CAD输出精确样本证书，用独立Z3解释原坐标定义链和严格有理隔离盒，分别检查：证书有实解；证书与原公式否定的合取不可满足。5例均通过。核对工具另用错误点、空证书和错误变量映射作负测试，避免将空证书的蕴含真误当作有效见证。

原始输入、TXT、映射及5个见证保存在[归档](smtlib-20261106/small-inputs-and-witnesses.tar.gz)，完整结果与校验值见[目录](smtlib-20261106/)、[小集结果](smtlib-20261106/small-results.json)、[转换核对](smtlib-20261106/small-equivalence.json)和[见证核对](smtlib-20261106/small-witnesses.json)。

## 超时与循环检查

仅在临时头文件副本中增加诊断，不修改生产实现。记录投影／提升／判零／根隔离阶段、精化精度及调用计数；同时检查：

1. WRSD、RCtoSqrfree的活动递归栈是否再次出现完全相同的完整输入（含比较器身份，不以哈希碰撞作判据）。
2. REFINE循环执行一次收缩后，全部区间端点是否完全不变。

检测器在已记录的底层反例 `[x²,y³,z²+xy]` 上捕获了RCtoSqrfree重复活动输入。它是诊断工具的正控制，属于此前已知问题，不计为本次SMT例子的新发现。正常√2用例也通过了诊断副本运行。

选取4个超时例，诊断仍各限时10秒：

| 原例 | 末段观察 |
|---|---|
| `hong/hong_3.smt2` | 已推进到第二提升层，停留于指定点判零的WRSD调用 |
| `20220314-Uncu/.../ProveIneq_JSC_SUMCracker_19a.smt2` | 已推进到第三层第8个父胞腔，RCtoSqrfree递归内耗时 |
| `20220314-Uncu/.../BDC_ProveIneq_ISSAC05_AmerMathMonthly10347LHSa.smt2` | 第二层平方自由分解耗时 |
| `meti-tarski/.../atan-problem-2-sqrt-weak-chunk-0032.smt2` | 第一层38次乘积的一元认证隔离内耗时 |

四例均未触发上述重复输入或不变精化状态检测，仍在10秒内未完成。这不排除其他不终止模式，也不能证明全部实现终止；目前没有把它们判作已确认死循环。具体上下文与阶段日志见[诊断记录](smtlib-20261106/diagnoses.json)。已知底层缺陷仍见[独立说明](../fixes/rsd-nonradical-prefix.md)。

## UNSAT较快候选的复测

小集16个共同UNSAT中没有观察到CAD更快。全量初轮出现 `hong/hong_1.smt2` 的单次候选：公式为 `x²<1 ∧ x>1`，CAD与Z3均判UNSAT。它确实无解：x>1使 `(x−1)(x+1)>0`，与x²<1矛盾。

全量在自然边界暂停后，对同一原文件和已转换TXT交替串行各复测5次，10次答案均为UNSAT：CAD中位数18.502ms，Z3中位数6.962ms。初轮单次较快未复现，不将其宣称为CAD优势。另一个候选 `mbo_E11` 只是无原变量的常量假公式，单独标为trivial，不作为算法优势。原始重复计时见[记录](smtlib-20261106/retest-hong_1.json)。

## 后台全量仍在进行

子agent建立了独立持久worker，冻结CLI、parser、核对器和runner的副本及哈希；采用JSONL日志和原子checkpoint，可自然边界暂停、恢复，意外退出后续跑。全部12154例都保留，转换不支持、核对未完成、求解超时、错误与确定答案分别计数。通过转换及核对后才运行CAD。

当前目录为 `/private/tmp/clpoly-smtlib-full-20261106/`，worker PID记在 `launcher.json`／`status.json`；本次启动PID为89970。整理报告时已恢复运行，全量尚未完成。[状态快照](smtlib-20261106/full-background-snapshot.json)只表示记录时刻，不自动代表之后的进度。随时查询：

```sh
python3 tools/smtlib/full_benchmark.py --stage status --out /private/tmp/clpoly-smtlib-full-20261106
```

原始结果在该目录的 `events.jsonl`，汇总在 `summary.json`，实际TXT在 `converted/`，变量映射在 `mappings/`。后续发现答案冲突、转换不同或异常退出时保留原输入和冻结版本作最小复现，不将一次超时直接归为死循环。

## 工具验证与早期探索边界

parser及映射工具10组测试、精确见证工具4组测试通过；后台设施9组测试通过，覆盖JSONL恢复、暂停／恢复、按顺序落盘与核对、原生TXT调用、超时supervisor及子进程回收。生产C++代码没有修改，因此本次没有重复运行前一版本已经通过的40个C++测试文件。

早期100例广泛抽样使用了含Python转换的wrapper总计时，结果仅作探索性冒烟记录，保留在 `exploratory-*` 文件。它不是最终原生CAD计时；其样本、变量规模和与上述定向96例的选择口径也不同，不合并计算性能结论。
