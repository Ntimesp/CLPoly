# 模块化 CAD 阅读入口

技术路线：QQ 多项式 + XY16 三角列 + XZ06 实点隔离/精化。

**逐项审查从 [相对 origin/CAD_dev 的审查指南](review-guide.md) 开始。** 基线固定为 `1c214ed`；此前内容视为可信，仅审查之后的新增和修改。指南列出模块职责、基线修改与验证依据。

1. [总架构](architecture.md)：独立模块、数据与算法分离、所有权和存储契约。
2. [区间规格与证明](../interval-arithmetic/design.md)、[区间实现审核](../interval-arithmetic/review.md)。
3. [多项式区间求值](../polynomial-interval/design.md)：独立模块的规格、实现对应与实测。
4. [零维正则链](regular-chain.md)：`charset.hh` 仍为唯一分解算法核心；分支划分仅用 WRSD。[RSD 使用边界说明](../../fixes/rsd-nonradical-prefix.md)与本批接口一并审核。
5. [实代数点细化](real-algebraic.md)：QQ 隔离盒、上下界证书、指定分支判零。
6. [QQ Lazard 与 CAD](lazard-and-cad.md)：有理代表的语义与完整提升。
7. [理论 PDF](../../theory/lazard-evaluation.pdf)：编号算法及人工证明。
8. [后续形式化接口](formalization.md)、构建与测试验证。编号算法与函数的对应表直接写在实点、Lazard 和完整 CAD 设计中。

## 当前公共入口

| 模块 | 文件 |
|---|---|
| 有理数 | `clpoly/number/QQ.hh`，保持原表示 |
| 独立区间 | `clpoly/rational_interval.hh` |
| 独立有理采样 | `clpoly/rational_sample.hh`，见[采样规格](../rational-sampling.md) |
| 区间与多项式连接 | `clpoly/polynomial_interval.hh` |
| 三角列算法核心 | 原有 `clpoly/charset.hh` |
| 零维正则链对象及契约 | `clpoly/regular_chain.hh` |
| 实代数点 | `clpoly/real_algebraic.hh` |
| QQ Lazard | `clpoly/lazard_qq_result.hh`（值类型）、`clpoly/lazard_qq.hh`（算法） |
| 完整 CAD | `clpoly/full_cad_result.hh`（数据）、`clpoly/full_cad.hh`（算法） |

区间公共接口为 `rational_interval.hh`，不依赖 MPRIA。
