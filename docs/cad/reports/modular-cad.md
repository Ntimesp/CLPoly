# CAD 构建与测试验证

本文保留历史验证记录。2026-11-06 的逐功能复核、独立快照和当前工具验证见 [复核记录](uncommitted-review.md) 与 [工具说明](three-datasets/README.md)，不能把下面的旧计数和耗时当作当前版本数据。

WRSD 切换回归日期：2026-10-31；整库 38 个测试文件的全量记录来自 2026-10-22，本批未重新执行该全量测试。规格及分层实现见[设计入口](../README.md)，文献对应见[理论摘要](../theory/lazard-evaluation-theory.md)，指定纤维的契约与性能诊断见[实点设计](../design/modular-cad/real-algebraic.md)。数学证明仍相对于明确列出的文献定理和已审核基线契约；本记录是实际构建与测试结果，新增版本尚未全部经过人工审核，机器形式化尚未完成。

## 历史验证结果（2026-10-22 至 2026-11-03）

| 对象 | 结果 | 证据范围 |
|---|---|---|
| 整库 Debug（2026-10-22） | 38 个测试文件通过，0 失败 | 历史全量记录，本批只重跑下列三角列至 CAD 的相关测试 |
| QQ 区间 | 94 个断言通过 | 明确端点期望、四则、幂、集合与异常 |
| QQ 开区间采样 | 28 个断言通过 | 可手算最简值、开端点、无界、任意精度和长连分数 |
| 采样独立枚举检查 | 10000 个固定种子随机区间通过 | 与直接枚举分母的最简值相等，并严格位于区间内部 |
| 多项式区间求值 | 25 个断言通过 | 独立测试，稀疏 Horner、变量序、转换和绑定 |
| 三角列算法 | 27 个断言通过 | 原有特征列、WRSD、RSD 与平方自由分解测试保持不变 |
| 三角列对象 | 39 个断言通过 | 正则、平方自由、WRSD、前缀缓存、输入转换与所有权 |
| 实代数点 | 171 个断言通过 | 隔离、精化、共轭、降次/重根、递归重编码和外部证书拒绝 |
| QQ Lazard | 19 个断言通过 | 活动系数分组、导数阶数、阶乘与前缀语义 |
| 完整 CAD | 3067 个断言通过 | 完整栈、所有维数、来源根身份、共享根计数、样本和原输入符号；含新采样集成 |
| 三角列算法/对象/CAD 的优化构建 | 27/39/3067 个断言通过 | 本批使用 `-O2 -DNDEBUG -fno-elide-constructors`，链接既有 Debug 静态库 |
| 三角列对象/CAD 的地址检查 | 39/3067 个断言通过 | 新头文件与测试使用 AddressSanitizer；底层 Debug 静态库未插桩 |
| 采样的优化构建（2026-10-22） | 28 个断言通过 | 历史独立采样记录，只链接 GMP；本批未修改采样模块 |
| 存储头独立包含 | 编译运行通过 | 不引入 `cad.hh` 或 `lazard_qq.hh` |
| 独立解析/几何探针 | 全部通过 | 78 个解析根、7 组几何用例、来源身份与单纤维检查 |
| 补充点/纤维探针 | 全部通过 | 48 组生成点/纤维/证书检查、有理网格 25 胞腔及独立符号、UNI 零根和重根拒绝 |
| 理论 PDF | 三遍 XeLaTeX 构建通过，24 页 | POINT-ZERO 直接使用 WRSD 零集标签；含 SAMPLE-RATIONAL/POSITIVE；无未解析引用或 Overfull |
| 补充非约化前缀探针 | 已知失败，待修复 | `squarefree_decomposition([x²,y³,z²+xy])` 重复相同递归参数，不能由以上回归推出全输入正确性 |

## 本批 WRSD 审核范围

`charset.hh` 的原有 RSD 实现及 `test_charset.cc` 原有测试完整保留，二者相对提交 `5402483` 没有修改。新增模块不直接调用 RSD，当前经 `wrsd` 和 `RCtoSqrfree` 的间接路径也没有进入 RSD；若后续发现旧模块间接调用，须单独列明并汇报。原有 `rsd_basic` 测试中的直接调用继续保留。最小输入 `[x²,y³]`、`xy` 仅在新增三角列测试中执行 WRSD，检查零分支保留原链；RSD 的不终止复现不进入正常测试进程。相关条件、复现和调用边界见[RSD 独立说明](../fixes/rsd-nonradical-prefix.md)。

本批还更新三角列规格、实点伪代码、理论 PDF、公共 API 与形式化接口，使其全部只依赖 WRSD 的零集覆盖；没有为零分支额外假设伪余式证书。当前 Debug 的五组测试共 3319 项检查。

补充探针发现原有平方自由分解的递归进展缺口，详见同一独立说明。这不是本次点判零替换引入的失败；不过平方自由分解是纤维接口的依赖，因此本批不能据已有 3319 项检查通过而宣称完整算法的正确性审核完成。

## 覆盖范围

区间测试使用可手算的期望值，覆盖大端点、乘法符号组合、跨零幂、自别名和异常。区间包含零只表示无法由当前包络决定符号。

采样测试以直接期望值为主：`(1/3,1/2)` 返回 `2/5`，`(0,1)` 返回 `1/2`，严格整数端点不选入；覆盖负区间反射、无界端点、相等/反序拒绝、超过 64 位的端点、`(0,1/2^200)` 和 400 步 Fibonacci 构造的长连分数间隙。CAD 的根 0、1 之间返回 1/2，两侧无界样本分别为 -1、2。

独立枚举探针用 `mt19937(20261022)`，端点分子在 [-200,200]、分母在 [1,100]。对 10000 个非空开区间，按分母从 1 到两端点分母之和枚举，计算严格满足开端点条件的整数分子范围并取绝对值最小者。该上界由端点中项保证；验证结果与连分数完全相同。枚举不调用连分数辅助函数，也不进入生产代码。它是小规模独立实验，不能替代任意精度的数学证明。

实点测试覆盖零根、有理根和正负无理根，平方自由正规化、可约链上的指定共轭、近邻根、次数下降和恒零纤维。重编码在不同定义链上保留同一点，平方自由分支筛选后仅提升所选前缀。公开构造仍拒绝无法认证的宽盒。

CAD 独立预期包括一元 sqrt2 的 5 个胞腔、坐标轴 9 个、圆 13 个、圆加水平线 19 个及其几何符号、四维坐标分解 81 个叶和维数分布 1/8/24/32/16。消零对 `xz-y,yz-x` 在原点的 Lazard 代表为 -1 与 z，仍切出 z=0，总叶数为 2×29+9=67。抛物线为 9 叶，交叉直线为 13 叶；一元探针验证 12 个多项式的 78 个已知有理根，另覆盖变量次序交换、自由坐标和三变量空输入。

边界回归检查 `cad_root` 的来源索引和来源根序号，尤其检查共享根对所有来源计数。完整栈验证父子关系、Section/Sector 交替、前缀点一致、Sector 样本位于邻根之间、来源表及维数正确。

## 复现

```sh
bash test/run_all_tests.sh
sh docs/cad/theory/build.sh
```

[独立解析/几何探针](cad-literature-probes.cc)使用 geometry、uni、boundary、fibre 四种模式：

```sh
g++-13 -O2 -DNDEBUG -I. -I/opt/homebrew/opt/gmp/include -I/opt/homebrew/opt/boost/include docs/cad/reports/cad-literature-probes.cc lib/debug/libclpoly.a -L/opt/homebrew/opt/gmp/lib -lgmpxx -lgmp -o /tmp/clpoly-cad-probes
/tmp/clpoly-cad-probes geometry
/tmp/clpoly-cad-probes uni
/tmp/clpoly-cad-probes boundary
/tmp/clpoly-cad-probes fibre
```

[补充点/纤维探针](review-probes.cc)保留 UNI 零根、重根拒绝、生成点和网格检查；两者不代替正式测试集。

当前整库日志 `/private/tmp/clpoly-cad-fix-20261022/suite.log`，优化测试翻译单元、存储头、PDF 构建和性能诊断日志位于同一目录。临时文件可能由系统清理，本文记录结果与边界，仓库保留源码及可复现入口。

本次采样集成的整库日志为 `/private/tmp/clpoly-rational-sample-suite.log`，CAD 优化日志为 `/private/tmp/clpoly-full-cad-rational-sample-optimized.log`，PDF 构建日志为 `/private/tmp/clpoly-rational-sample-pdf.log`。此前日志记录历史验证，本节结果表已更新到采样集成后的版本。

## 采样独立实验复现

[探针源码](rational-sampling-probes.cc)默认执行 10000 个随机区间的枚举比较；`fibre-old` 和 `fibre-simple` 仅对 054 固定首纤维的首轮上下界执行原有两次平方自由检查。

```sh
g++-13 -std=c++17 -O2 -DNDEBUG -I. -I/opt/homebrew/opt/gmp/include -I/opt/homebrew/opt/boost/include docs/cad/reports/rational-sampling-probes.cc lib/debug/libclpoly.a -L/opt/homebrew/opt/gmp/lib -lgmpxx -lgmp -o /tmp/clpoly-rational-sampling-probes
/tmp/clpoly-rational-sampling-probes
/tmp/clpoly-rational-sampling-probes fibre-old
/tmp/clpoly-rational-sampling-probes fibre-simple
```

性能测量应分别运行两种模式三次，交替运行次序，取 `two_squarefree_ms` 中位数。计时仅包围两次 `xz_detail::squarefree`，不包含输入构造、上下界构造、采样或进程启动。两种配置使用同一 y 层乘积、各自样本加减 1/8 的前缀盒、原有精确分式平移，不改变 GCD 或 XZ06 代码。数学背景及早期分项实验见[采样设计](../design/rational-sampling.md)。

GCC 13.4.0、arm64，本次保存的探针三次交替运行结果如下。所有上下界均通过平方自由检查。

| 配置 | 样本分母位数 | 上/下界系数最大位数 | 两次平方自由检查中位数 ms |
|---|---:|---:|---:|
| 旧选点规则 | 90 bit | 5073 bit | 3956.50 |
| `sample_rational`，返回 -3 | 1 bit | 278 bit | 100.55 |

这是固定首纤维的阶段对照，不是完整 054 的求解耗时。后续精化、其他纤维及全空间构建仍可能昂贵。逐次结果为 `/private/tmp/clpoly-rational-sampling-probes-fibre.json`；仓库保留探针源码和以上复现方法，临时结果可能被系统清理。

## FormalMath500 困难输入复测

对线性等式预处理后仍未完成的 054、055、056、058 复测。使用与先前实验相同的残余公式、声明变量序及其反序，GCC 13.4.0、`-O2 -DNDEBUG` 测试翻译单元链接同一既有 Debug 静态库，串行运行各进程。复用的是实验脚本的预处理结果，不是已经接入公共 API 的公式求解器。

| 输入 | 剩余变量数 | 旧采样，每序 10 秒 | 新采样，每序 10 秒 | 新采样，每序 30 秒 |
|---|---:|---|---|---|
| 054 | 3 | 两序均超时 | 两序均超时 | 两序均超时 |
| 055 | 3 | 两序均超时 | 两序均超时 | 两序均超时 |
| 056 | 4 | 两序均超时 | 两序均超时 | 两序均超时 |
| 058 | 3 | 两序均超时 | 两序均超时 | 两序均超时 |

本轮没有新增完成题，不能从截断的超时数据计算整题加速倍数。前述首轮平方自由检查的改善不代表整个 CAD 完成。输入、适配器源码、二进制、SHA256 元数据及逐次记录位于 `/private/tmp/clpoly-formalmath500-sampling-20261022/`。

进一步将 054 的首个 y 纤维单独运行到完成：原实现于第 113 轮返回 9 个根；仅替换 GCD 实现的诊断对照在相同轮次返回相同区间。该例的终止性核验、符号 unknown 的含义和区间过估计示例见[实点设计](../design/modular-cad/real-algebraic.md)。这里仍未完成整道 054。

## 平方自由检查改用普通 GCD：054 复测

2026-10-23，仅将 `xz_detail::squarefree` 中的四参数扩展 GCD 换成二参数普通 GCD；零与常量处理、最终次数判断、重复检查、精化和根认证条件均保持原样。普通 GCD 复用库中既有实现，未修改整数或多项式 GCD 模块，数学伪代码仍使用同一个平方自由判据。

使用三数据集快照中的同一 054 残余输入、声明投影序及原适配器源码。旧二进制 SHA256 与原实验清单一致；新适配器及单纤维探针使用当前生产头文件，`-std=c++17 -O2 -DNDEBUG`，链接同一既有 Debug 静态库。编译完成后串行运行：

| 检查 | 结果 |
|---|---|
| 旧版完整 054，限时 30 秒 | 超时 |
| 普通 GCD 版完整 054，限时 30 秒 | 超时 |
| 普通 GCD 版首个 y 纤维，z=-3 | 约 7.20 秒完成，返回 9 个根，平移量 4259 |
| 首纤维与原扩展 GCD 诊断日志比较 | 9 个 QQ 根括区间逐字一致 |
| 重新编译并运行 `test_real_algebraic` | 171 个断言全部通过 |
| 重新编译并运行 `test_full_cad` | 3067 个断言全部通过 |

旧首纤维约 114.94 秒的结果来自此前带插桩诊断，当前约 7.20 秒来自未插桩生产头文件；不据此计算严格加速比。局部隔离已能较快完成，但本轮仍未完成整道 054，不能宣称整题已解决。两次完整 CAD 都被限时截断，也不能据此计算整题加速比。

[原始记录](gcd-squarefree-054.json)保存命令、结果、构建及输入信息、精确根区间和生产代码差异。适配器源码见[三数据集实验](three-datasets/driver.cc)；首纤维探针是[诊断归档](three-datasets/profile-evidence.tar.gz)中的 `054-termination/probe.cc`，以当前生产头文件重新编译即可复测。完整 CAD 可使用三数据集数字输入与该适配器复现，墙钟上限需由外部子进程控制。

## Mathematica 对 054 原始公式的判定

2026-10-23，本机 Wolfram 14.2.1（MacOSX-ARM64）直接接收 054 的展开多项式公式：

```wolfram
formula054 = x > 0 && y > 0 && z > 0 &&
  x^2 y z + x y^2 z + x y z^2 - 1 == 0 &&
  x y + x z + y^2 + y z - 2 < 0;

Resolve[Exists[{x, y, z}, formula054], Reals]
Reduce[formula054, {x, y, z}, Reals]
CylindricalDecomposition[formula054, {x, y, z}]
```

不输入手工证明的恒等式，不代入取等号见证，不执行 CLPoly 线性预处理。每种方法在独立新内核中连续调用三次，采用函数内部 `AbsoluteTiming`，不计内核启动；单次限时 30 秒。重复调用可能复用加载代码或缓存，因此首轮和后两轮分别列出，不能仅用重复调用代表首次求解成本。

| 方法 | 第一轮秒数 | 第二轮秒数 | 第三轮秒数 | 返回值 |
|---|---:|---:|---:|---|
| `Resolve[Exists[…], Reals]` | 0.102658 | 0.005614 | 0.005085 | `False` |
| `Reduce[…, {x,y,z}, Reals]` | 0.723616 | 0.004428 | 0.004018 | `False` |
| `CylindricalDecomposition[…, {x,y,z}]` | 0.708855 | 0.004695 | 0.003682 | `False` |
| `CylindricalDecomposition[…, {z,y,x}]` | 0.682514 | 0.004249 | 0.003606 | `False` |

将末尾严格不等式改为等式得到 055，`Resolve[Exists[…], Reals]` 在同一 Resolve 内核中约 0.010435 秒返回 `True`。054 无反例、055 可达，合起来确认候选最小值 2。

这些结果说明该公式可由本机 Mathematica 很快判定，也表明当前求解路径仍有优化空间。`Resolve`/`Reduce` 的内部算法选择不由此次黑箱测试确定；显式调用 `CylindricalDecomposition` 也不等于要求生成我们 `full_cad` 所返回的、对全部输入多项式符号不变的全空间分解。它可以直接输出与输入公式等价的 `False`，因此不能将两种任务的耗时相除作为同一完整 CAD 算法的加速比。

[复现脚本](mma-054.wls)和[逐次原始记录](mma-054.json)保留公式、版本、命令、输出和计时。`wolframscript` 在本机未能自动定位内核，最终直接调用已安装的 WolframKernel，不修改全局配置。例如在仓库根目录运行一个独立的 CAD 内核：

```sh
CLPOLY_MMA_METHOD=cadxyz /Applications/Wolfram.app/Contents/MacOS/WolframKernel -noinit -noprompt -script docs/cad/reports/mma-054.wls
```

方法可选 `resolve`、`reduce`、`cadxyz`、`cadzyx`；各次分别启动进程。默认 JSON 输出到脚本所在目录，也可用 `CLPOLY_MMA_OUT` 指定已有输出目录。记录中的 `process_wall_seconds` 含内核启动，各调用的 `seconds` 不含启动。

## 三角列顺序与外部比较器借用

`full_cad` 按投影顺序消去变量，`cad_tree` 和逐层求根采用反向的提升顺序。三角列构造器接收提升坐标及外部比较器地址，检查坐标在完整序的末尾连续反向排列，保证原 WRSD 的 cls 编号仍从 1 开始。`bounds` 将提升坐标与根盒同步反转后调用字典序区间内核。

三角列只保存 `const order_type*`，不拥有比较器。复制、移动、前缀及分解分支共用外部序；不再实现指针重绑定或私有交换。`append_rational` 和 `roots_over` 显式接收目标比较器，避免返回借用局部变量的实点。CAD 样本和根栈统一借用结果容器持有的比较器，复制整个结果保留其共享所有权，单独复制样本不延长该比较器的生命周期。原 `charset.hh` 分解算法未修改。

2026-11-03 验证：

| 检查 | 结果 |
|---|---|
| 三角列 Debug | 39 个断言通过 |
| 实代数点 Debug | 171 个断言通过 |
| QQ Lazard Debug | 19 个断言通过 |
| 完整 CAD Debug | 3067 个断言通过 |
| 三角列／CAD `-O2 -DNDEBUG -fno-elide-constructors` | 39／3067 个断言通过 |
| 三角列／CAD AddressSanitizer | 39／3067 个断言通过，无地址错误 |
| 理论 PDF | 三遍 XeLaTeX 构建，无未解析引用或 Overfull |

测试覆盖源链销毁后继续借用外部序、默认复制／移动赋值、容器扩容、分支与前缀共享比较器，以及空指针和目标序编号缺口的拒绝。完整 CAD 的结果副本在源结果销毁后继续运行多项式运算和树检查；三变量非平凡排列同时检查提升坐标、根盒和最终符号。

基础复现：`make test/test_regular_chain test/test_real_algebraic test/test_lazard_qq test/test_full_cad`，随后运行 `_build/debug/bin/` 下的相应二进制。优化构建使用已有 GMP/Boost 路径和 Debug 静态库，对三角列及完整 CAD 测试增加 `-O2 -DNDEBUG -fno-elide-constructors`。

地址检查使用 Clang，配合 GCC 的 C++ 标准库头文件和 `libstdc++` 保持既有静态库 ABI，增加 `-fdelayed-template-parsing` 兼容原头文件前向声明顺序，以及 `-fsanitize=address -fno-omit-frame-pointer -fno-elide-constructors`。运行时启用栈返回后使用检查并关闭平台不支持的 LeakSanitizer。底层 Debug 静态库未插桩，不能视为全库内存验证。

此前 UBSan 检查发现可信基线 `clpoly/monomial_order.hh:464` 在变量数为 1 时执行 `mc <<= 64`。该文件与 `origin/CAD_dev` 相同，本批未改动；单独地址检查通过不消除该未定义行为。原诊断在 `/private/tmp/clpoly-triangular-review-20261028/`。

当前优化与地址检查的命令和日志在 `/private/tmp/clpoly-borrowed-order/`，命名统一后的 Debug 日志为 `/private/tmp/clpoly-regular-chain/test_*.log`。临时文件可能被清理；本批未重跑完整题集性能或整库全部测试。相关接口、伪代码和借用约定见[三角列设计](../design/modular-cad/regular-chain.md)。

## 验证边界

优化测试翻译单元通过不代表整库 Release 构建；未运行整库 sanitizer、作者 Maple 交叉验证或正式大规模 CAD 基准。诊断样例的收益不作为普遍性能承诺。样本和测试证据不能替代整胞腔上的 valuation 归纳，也不能替代后续机器形式化证明。
