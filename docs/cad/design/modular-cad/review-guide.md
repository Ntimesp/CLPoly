# CAD 分层审查入口

## 本轮边界（2026-11-06）

按本轮用户指定，以当前 `HEAD`（`5402483`）为边界，假设全部已提交底层代码满足其正确的输入、输出和性质，只审查未提交的上层代码及其调用契约。底层已知缺陷单独留档，后续处理，不计入上层缺陷。逐项契约、审核结论和独立验证见[未提交上层审核记录](../../reports/uncommitted-review.md)。

## 历史批次范围

以本地远程跟踪引用 `origin/CAD_dev` 的 `1c214ed1175dd1de18e2e35e0be24db59ae23b9c` 为可信基线；该提交中的算法与数据结构按可信契约使用。

审查对象是该基线之后的实现。检查工作区时，须同时查看 `git diff origin/CAD_dev` 与未跟踪文件，保证新增模块进入审查范围。

## 1. 可信依赖与基础修改

可信依赖包括现有 QQ、多项式算术、结式、分解、一元实根隔离、`charset.hh` 中使用的 WRSD，以及 `cad.hh` 投影/Open CAD 和 `cad_tree.hh` 树结构。`charset.hh` 原有 RSD 及其基线测试保持不变。新增代码不直接调用 RSD；如果经旧代码间接进入 RSD，须列出路径并汇报。当前新增算法没有这类间接路径，也不把 RSD 作为证明依赖。平方自由分解仍引用 XY16 的数学规格，但原实现存在非约化前缀上的不终止反例，须作为本批未完成的审查项，不能直接导入其全输入终止性。

以下基线文件修改属于独立审查对象：

| 修改 | 说明与审查文档 | 代码/测试 |
|---|---|---|
| ZZ 边界运算 | [INT64_MIN 修复](../../fixes/zz-int64-min-cad-sanitizer.md) | `clpoly/number/ZZ.hh`；`test/test_number.cc` |
| RCtoSqrfree 首层扫描 | [首层重因子漏检修复](../../fixes/rctosqrfree-first-level.md) | `clpoly/charset.hh`；`test/test_charset.cc` |
| 公共入口和测试集成 | 增加新模块包含与测试注册 | `clpoly/clpoly.hh`、`test/run_all_tests.sh`、根 README/TODO |

QQ 的分子/分母表示保持不变。投影和树结构复用可信基线实现。

## 2. 模块依赖分层

```text
可信 QQ ──────────────→ 独立 QQ 区间
可信多项式 + QQ 区间 ─→ 多项式区间估值
可信 charset ─────────→ 三角列对象与契约
                              │
三角列 + 区间估值 + 可信一元隔离
                              ↓
                    实代数点：三角列 + QQ 隔离盒
                              ↓
                    QQ 代表上的 Lazard 求值
                              ↓
                  完整 CAD（复用可信投影和树）
```

区间和三角列是并列基础模块。多项式估值是连接层；实代数点才组合三角列和隔离盒。Lazard 保留形式变量和 QQ 系数，不构造无理系数多项式。

## 3. 按顺序审查的七个包

每包按“规格/证明 → 实现 → 测试”阅读。

| 顺序 | 职责及核心审查点 | 规格 | 实现 | 测试 |
|---|---|---|---|---|
| 1：独立区间 | QQ 闭区间、精确端点运算、包含性、零与未知符号、除法前置条件；不含根对象 | [设计与证明 I01–I13](../interval-arithmetic/design.md)、[实现审核](../interval-arithmetic/review.md) | `rational_interval.hh` | `test_rational_interval.cc` |
| 1a：独立有理采样 | QQ 开区间的严格包含、最简性、连分数终止和无界分派；不依赖区间或根类型 | [采样规格与证明](../rational-sampling.md)、PDF SAMPLE-RATIONAL/POSITIVE | `rational_sample.hh` | `test_rational_sample.cc` |
| 2：多项式区间求值 | 字典序稀疏 Horner、其他序转换、包含性、单调性与收敛性 | [设计、证明与函数对应](../polynomial-interval/design.md) | `polynomial_interval.hh` | `test_polynomial_interval.cc` |
| 3：三角列 | 三角形/正则/平方自由契约、变量序生命周期、WRSD 封装及 RSD 使用边界 | [三角列](regular-chain.md)、[RSD 独立说明](../../fixes/rsd-nonradical-prefix.md)；RCtoSqrfree 首层修复已单独审核提交 | `regular_chain.hh`、`charset.hh`，以及 `real_algebraic.hh` 的 WRSD 调用 | `test_regular_chain.cc`、`test_charset.cc`；下游联测 |
| 4：实代数点 | 正则平方自由三角列 + 严格 QQ 隔离盒；完整隔离、精化、判零/符号、同点判定、重编码、指定纤维全部实根 | [实代数点设计](real-algebraic.md)、理论 PDF 算法 1–6、9–11 | `real_algebraic.hh` | `test_real_algebraic.cc` |
| 5：QQ Lazard | 按活动单项式合并系数后判零；连续求导、阶乘除法、冻结前缀变量；保留原 QQ 代表 | [Lazard 与 CAD 设计](lazard-and-cad.md)、理论 PDF 算法 7–8 | `lazard_qq_result.hh`（值类型）、`lazard_qq.hh`（算法） | `test_lazard_qq.cc` |
| 6：完整 CAD | 每父栈全部根；m 个 Section 与 m+1 个 Sector；样本、来源、维数、原输入符号；任意维数的 Section/Sector 交替 | [Lazard 与 CAD 设计](lazard-and-cad.md)、[公共 API](../full-cad/api.md) | `full_cad_result.hh`（存储）与 `full_cad.hh`（算法），复用可信投影和树 | `test_full_cad.cc` |
| 7：集成与证明接口 | 跨层契约、伪代码对应、退化输入、比较器生命周期、最终定理的依赖是否闭合 | [模块架构](architecture.md)、[形式化接口](formalization.md)、验证报告 | 公共头文件及测试注册 | 整库回归 |

表中头文件位于 `clpoly/`，测试位于 `test/`。

实代数模块目前提供 CAD 所需的点与 QQ 多项式查询，并非任意独立数域合并的通用代数数四则类。实点盒重叠不能作为两点相等的判断；点上非零也不能用商环余式非零替代。这两项是第 4 包的关键审查点。

## 4. 理论、论文与完整文档目录

- [总架构](architecture.md)：模块职责与依赖。
- [总体方案](../proposal.md)：总体路线。
- [理论 PDF](../../theory/lazard-evaluation.pdf)、[LaTeX 源码](../../theory/lazard-evaluation.tex)、[理论摘要](../../theory/lazard-evaluation-theory.md)：13 个编号算法及人工论证。
- 算法到具体函数的映射分别见[实点设计](real-algebraic.md)、[Lazard 设计](lazard-and-cad.md)及[完整 CAD 设计](../full-cad/detailed-design.md)。
- [形式化接口](formalization.md)：QQRep、IntervalRep、ChainRep、PointRep、Representative、TreeRep，正确性、终止性和 C++ 精化义务。
- [CAD 文档入口](../full-cad/README.md)、[架构](../full-cad/architecture.md)、[详细设计](../full-cad/detailed-design.md)、[形式规格](../full-cad/formal-specification.md)、[API](../full-cad/api.md)。各文档分别负责结构、步骤、证明义务和调用契约。
- 验证报告：测试范围、结果、复现命令和限制。

指定论文：

1. [Xia–Yang 2016](<../../../references/triangular-set/Xia 和 Yang - 2016 - Automated Inequality Proving and Discovering.pdf>)：可信 charset 算法来源；审查新增封装和首层平方自由处理是否保持其契约。
2. [Xia–Zhang 2006](../../../references/triangular-set/Xia-Zhang-2006-Real-Solution-Isolation-Using-Interval-Arithmetic.pdf)：实点隔离和精化的主要依据；有理平移、上下界配对、证书与收敛细化在理论 PDF 中说明。
3. [Xia–Yang 2002](<../../../references/triangular-set/Xia和Yang - 2002 - An Algorithm for Isolating the Real Solutions of Semi-algebraic Systems.pdf>)：补充参考，不以可能失败的早期 REALZERO 作为当前完整性的依据。

论文的 OCR 文本和 theorem 摘要仅为阅读辅助，以原论文为准。

## 5. 证明与证据边界

基线算法按可信契约使用。新增层须证明自身满足依赖算法前提，并正确解释其结果。理论 PDF 给出实点、QQ Lazard、完整提升的伪代码和数学证明，明确导入 MPP 定理，使用 valuation 归纳证明柱形性和整胞腔符号不变性。

验证报告记录执行过的构建与测试；[形式化接口](formalization.md)列出机器证明义务。规范的完整性不表示用户审核或机器形式化验证已经完成。
