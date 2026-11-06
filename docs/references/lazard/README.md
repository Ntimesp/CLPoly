# Lazard 投影与提升定理

此处是项目既有调研 `docs/research/lazard-cad-complete-theory.md` 引用的 MPP 文献原文，供当前完整 CAD 证明核对；不改变三角列和实点算法的 XY16/XZ06 依据。

- 作者：Scott McCallum、Adam Parusiński、Laurentiu Paunescu。
- 标题：Validity proof of Lazard's method for CAD construction。
- 发表：Journal of Symbolic Computation 92 (2019), 52–69。
- 本地版本：[arXiv v2 PDF](McCallum-Parusinski-Paunescu-Lazard-validity-1607.00264v2.pdf)，版本日期 2017-07-26。
- 原始来源：[arXiv:1607.00264v2](https://arxiv.org/abs/1607.00264v2)，2026-09-24 获取并核对。

使用位置：Algorithm 2.12；Definition 2.10；Propositions 2.11、3.4；Theorem 5.1 与 §5.3。本文中的非零首尾系数/判别式指非零多项式，不要求它们在每个父胞腔点上均不为零。定理的 moreover 条款处理独立的最后变量因子。

当前项目自己的证明和伪代码见 [revision 3 理论 PDF](../../cad/theory/lazard-evaluation.pdf)。原文是数学定理导入，不代表库代码已经获得机器形式化验证。
