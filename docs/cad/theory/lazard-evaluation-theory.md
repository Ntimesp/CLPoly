# Lazard evaluation：有理系数代表多项式与三角列实点

数学规格：[理论 PDF](lazard-evaluation.pdf) / [LaTeX 源码](lazard-evaluation.tex)。

## 文献边界

- 三角列分解：Xia–Yang 2016，Algorithm 2.2 WRSD（书页 40–41）、3.1 RCtoSqrfree（62）。调用均为无参数情形。RSD 不作为当前算法依赖，原有实现及测试保持不变，新增代码的直接／间接调用边界和文献条件问题见[独立说明](../fixes/rsd-nonradical-prefix.md)。
- 实代数点表示、隔离与精化：Xia–Zhang 2006，§3，NREALZERO/NSHR（855–858）。严格检查正则 TSA、平方自由、正半轴夹逼和平移条件。
- Lazard 投影与提升：[MPP](../../references/lazard/McCallum-Parusinski-Paunescu-Lazard-validity-1607.00264v2.pdf) 的 Algorithm 2.12、Propositions 2.11/3.4、Theorem 5.1 及 §5.3。父胞腔的 valuation 不变由逐层归纳保持，不能以样本点符号代替。
- Xia–Yang 2002 仅作补充；其可能失败的 REALZERO 不承担完整性。

XZ06 的具体对应为 §3 Definition 3.1、Proposition 3.1、NREALZERO/NSHR；单前缀提升使用 NREALZERO Step 2。有理 Cauchy 界 `2+M/m` 实现其正向平移；严格导数同号和端点异号给出单调配对条件的可判定充分证书。上述细化及 QQ Taylor 代表均在 PDF 中单独证明，不视为原论文伪代码的逐行抄写。

## 可执行路线

存储始终是 `QQ` 系数多项式、三角列和有理隔离盒。`E_i(p)` 只是证明中解释代表多项式的语义，不是“把无理数代入”的实现指令。

PDF 的编号算法包含：XZ-LIFT、NSHR-QQ、REFINE、ISOLATE-REGULAR、POINT-ZERO、POINT-SIGN、PREFIX-ZERO、LAZARD-QQ、REENCODE、ROOTS-QQ，以及 UNI-QQ、LIFT-STACK、FULL-CAD。

另有无编号 SAMPLE-RATIONAL/POSITIVE 辅助过程：用精确 QQ 连分数在开区间中求最简有理数，给出严格包含、最简性和分母和下降的终止性证明。LIFT-STACK 对已分离根盒的 QQ 内间隙调用它；不改变 Section、XZ06 平移或投影。算法依据及 C++ 对应见[采样设计](../design/rational-sampling.md)。

POINT-ZERO 直接使用 WRSD 返回的零／非零分支标签，不通过伪余式重新分类。在原点盒上用区间估值排除不含所选点的分支，反复执行 XZ06 精化，直到只剩同一标签。正确性与终止性只依赖有限分支的零集覆盖、区间包含和精化收敛，不要求零分支上的伪余式为零。绝不以“区间相交”直接确认根，也不以“商环余式非零”判断指定点非零。

LAZARD-QQ 保留全部形式变量，对当前活动变量连续求导，用 PREFIX-ZERO 检查前缀点上是否恒零。第一个非零阶除以阶乘，冻结当前变量，继续下一层。不能先模掉当前变量的定义方程。

输出结构为 `(QQ 代表多项式, valuation)`，另拥有多项式的比较器；数学意义相对于调用方保留的输入点 A。返回结构不拥有 A，也不是可脱离 A 使用的独立代数值。后续隔离前，必须分出首项不消去的分支，再作平方自由正则化，随后重编码所选前缀，只对该前缀调用 XZ06 单层提升；不能让其他共轭分支上的恒零纤维破坏零维前提。

## 完整 CAD 与证明边界

逐算法对照见[实点设计](../design/modular-cad/real-algebraic.md)、[Lazard 设计](../design/modular-cad/lazard-and-cad.md)和[完整 CAD 设计](../design/full-cad/detailed-design.md)。完整 CAD 的结果存储与提升算法分离，实点证书由公共接口解释。

边界沿用 `cad_root(来源多项式索引,该来源根序号)`；共享根对所有来源计数，通过 `root(level,parent,reference)` 解析，不把合并根表下标冒充来源序号。

完整提升的胞腔语义、循环不变量及正确性/终止性证明见 PDF §9–10。引用 MPP 投影定理作为明确的可信依赖。机器形式化验证仍未完成。
