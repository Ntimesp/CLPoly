# 完整 CAD：设计、理论与实现

本设计以 QQ 多项式、三角列和 QQ 隔离盒实现实代数点运算，并据此构造符号不变的完整柱状代数分解。算法与数据结构分离：数据类型维护表示和所有权，独立算法执行分解、隔离、精化与投影提升。

2026-11-06 本轮仅审核相对当前 `HEAD`（`5402483`）未提交的上层代码，假设已提交底层库满足文献与接口契约；逐项契约和条件正确性结论见[未提交上层审核记录](reports/uncommitted-review.md)。以下相对 `origin/CAD_dev` 的范围描述保留为历史批次背景。

审查基线为 `origin/CAD_dev`（`1c214ed`）；使用中的基线算法作为可信依赖，其后的新增模块和基础修改属于审查范围。原有 RSD 实现和测试保留；新增代码只直接调用 WRSD，若经旧模块间接使用 RSD 须单独汇报。当前调用链未发现间接 RSD。文档提供数学证明与 C++ 实现对应，机器形式化证明尚未完成。

当前尚有平方自由分解的已知不终止输入 `[x²,y³,z²+xy]`，本批整体正确性审核未完成。WRSD 点查询替换的论证与这一递归进展缺口分别记录在[RSD／非约化前缀说明](fixes/rsd-nonradical-prefix.md)中。

文献对应、函数映射、证明要点及性能数据直接写入各模块文档；[构建与测试验证](reports/modular-cad.md)记录实测结果和复现命令。新增版本尚未全部经过人工审核。

[三数据集实验报告](reports/three-datasets.md)记录新版题集全部 72 个查询的求解结果、47 道原题的转换核对，以及剩余超时输入的瓶颈、诊断证据和复现快照。

## 阅读顺序

| 顺序 | 文档 | 内容 |
|---|---|---|
| 1 | [总体方案](design/proposal.md)、[模块架构](design/modular-cad/architecture.md) | 目标、模块边界、数据所有权和算法接口 |
| 2 | [区间规格](design/interval-arithmetic/design.md)、[有理采样](design/rational-sampling.md)、[多项式区间求值](design/polynomial-interval/design.md)、[零维正则链](design/modular-cad/regular-chain.md) | 精确算术基础、连分数开区间采样和 XY16 接口契约 |
| 3 | [实代数点](design/modular-cad/real-algebraic.md) | XZ06 隔离、精化、指定实分支判零和符号 |
| 4 | [QQ Lazard](design/modular-cad/lazard-and-cad.md)、[完整 CAD 设计](design/full-cad/detailed-design.md) | 有理代表、纤维根与完整胞腔提升 |
| 5 | [理论 PDF](theory/lazard-evaluation.pdf)、[理论摘要](theory/lazard-evaluation-theory.md) | 13 个编号算法、有理采样辅助过程、正确性和终止性证明 |
| 6 | [公共 API](design/full-cad/api.md) | 调用方式、数据语义、前提和异常 |
| 7 | [逐层阅读指南](design/modular-cad/review-guide.md) | 基线范围、逐层代码和测试映射 |
| 8 | [形式化证明接口](design/modular-cad/formalization.md)、[验证报告](reports/modular-cad.md) | 表示关系、证明义务、测试结果与复现入口 |

基础修改的规格：[RCtoSqrfree 首层处理](fixes/rctosqrfree-first-level.md)、[ZZ 最小整数边界](fixes/zz-int64-min-cad-sanitizer.md)、[RSD 使用边界与 WRSD 替代范围](fixes/rsd-nonradical-prefix.md)。RSD 独立说明属于本批三角列接口的审核内容。

## 文档依据与构建

[外部参考资料](../references/README.md)存放论文和阅读辅助；[既有文档索引](../existing-docs.md)列出基线项目材料。算法数学定义以本项目理论 PDF 及其明确引用的文献为准；独立区间及多项式区间模块的伪代码与证明见对应规格，其他 Markdown 描述工程结构与实现对应，验证报告集中记录实测结果。

[LaTeX 源码](theory/lazard-evaluation.tex)使用以下命令生成 PDF，需要 XeLaTeX 和 ripgrep：

```sh
sh docs/cad/theory/build.sh
```
