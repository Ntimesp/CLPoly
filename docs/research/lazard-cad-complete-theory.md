# Lazard Full CAD 完整理论研究

> 本文档系统梳理 Lazard Cylindrical Algebraic Decomposition (CAD) 的完整理论，重点阐述其与 McCallum CAD 在胞腔边界表示上的本质差异，为 CLPoly 后续实现 Full Lazard CAD 提供理论依据。
>
> 所有引用均标注具体文献及位置。

---

## 目录

1. [引言：从 McCallum 到 Lazard](#1-引言从-mccallum-到-lazard)
2. [Lazard 投影算子](#2-lazard-投影算子)
3. [Lazard Valuation（核心创新）](#3-lazard-valuation核心创新)
4. [Lazard Evaluation（边界表示的关键）](#4-lazard-evaluation边界表示的关键)
5. [Lazard Delineability 定理](#5-lazard-delineability-定理)
6. [Lazard vs McCallum：胞腔边界表示对比](#6-lazard-vs-mccallum胞腔边界表示对比)
7. [Nullification 的完整处理流程](#7-nullification-的完整处理流程)
8. [Full Lazard CAD 算法伪代码](#8-full-lazard-cad-算法伪代码)
9. [参考文献](#9-参考文献)

---

## 1. 引言：从 McCallum 到 Lazard

### 1.1 CAD 的投影-提升范式

Cylindrical Algebraic Decomposition (CAD) 由 Collins 于 1975 年提出 [Col75, pp. 134–183]，核心思想是通过重复的**投影**（projection）和**提升**（lifting）阶段，将 $\mathbb{R}^n$ 分解为有限个柱形排列的胞腔（cells），使得输入多项式在每个胞腔上符号不变。

McCallum 在 1998 年提出了改进的投影算子 $P_M$ [McC98, pp. 242–268]，其基于 **order-invariance**（消失阶数不变性）：

> "The concepts are the same in case $n = 1$ because order and valuation are the same for this case." [MPP15, §2]

McCallum 投影算子定义为：
$$P_M(A) := \text{cont}(A) \cup \text{coeff}(B) \cup \text{disc}(B) \cup \text{res}(B)$$
其中 coeff 包含**所有系数**（leading, middle, trailing）[DNU23, §2.3, Definition 4]。

### 1.2 McCallum 的致命缺陷：Nullification

McCallum 的 lifting 要求输入多项式集合是 **well-oriented** 的：即在任何胞腔上，没有任何投影多项式恒为零（nullify）。如果发生 nullification，$L_M$ **会失败**（抛出错误而非给出错误结果）[DNU23, §2.3]。

> "$L_M$ may fail (giving an error message, not the wrong result) if relevant polynomials nullify (are identically zero) over a cell of $\mathbb{R}^k$ of positive dimension." [DNU23, §2.3]

Lazard 在 1994 年 [Laz94, pp. 467–476] 提出了一种新方法，通过引入 **valuation-invariance** 而非 order-invariance，从根本上绕过了 well-orientedness 的限制。这一方法直到 2019 年才由 McCallum, Parusiński 和 Paunescu 给出完整证明 [MPP19]。

---

## 2. Lazard 投影算子

### 2.1 定义

设 $A$ 是 $\mathbb{Z}[x_1, \dots, x_n]$（或 $\mathbb{R}[x_1, \dots, x_n]$）中的有限**不可约基**（irreducible basis），$n \geq 2$。Lazard 投影算子 $P_L(A)$ 定义为 [MPP19, Definition 2.1]：

> "The Lazard projection $P_L(A)$ of $A$ is the subset of $R_{n-1}$ comprising the following polynomials:
> 1. all leading coefficients of the elements of $A$,
> 2. all trailing coefficients (i.e. coefficients independent of $x_n$) of the elements of $A$,
> 3. all discriminants of the elements of $A$, and
> 4. all resultants of pairs of distinct elements of $A$." [MPP19, Definition 2.1]

形式化地：
$$P_L(A) := \text{lc}(A) \cup \text{tc}(A) \cup \text{disc}(A) \cup \text{res}(A)$$

### 2.2 与 McCallum/Brown 投影算子的比较

**大小关系**：$P_{BM}(A) \subsetneq P_L(A) \subsetneq P_M(A)$ [MPP19, Remark 2.3]

> "$P_L(A)$ is contained in and is usually strictly smaller than the McCallum projection $P_M(A)$. Indeed $P_M(A)$ includes the 'middle coefficients'... which $P_L(A)$ omits." [MPP19, Remark 2.3]

> "$P_L(A)$ contains and is usually strictly larger than the Brown-McCallum projection $P_{BM}(A)$. Indeed $P_{BM}(A)$ omits the trailing coefficients of the elements of $A$, which $P_L(A)$ includes." [MPP19, Remark 2.3]

Brown-McCallum 投影 $P_{BM}$ [Bro01, pp. 447–465] 进一步去掉了 trailing coefficients，但代价是：
1. 可能失败（如果 $A$ 不是 well-oriented）
2. 需要显式识别并添加零维 nullifying cells [MPP19, Remark 2.3]

### 2.3 Brown-McCallum 改进版 $P_{BM}$

Davenport 等人在 [DNU23] 中基于 [Bro04]（Brown-McCallum 对 Lazard 的改进）提出了 $P_{BM}$ 算子，通过测试 $T$ 来决定是否需要 trailing coefficients：

> "The polynomial part of the modified Lazard projection $P_{BM}(A)$ is the union of... (1) All leading coefficients... (2) Those trailing coefficients... for which $T$ returns $\bot$... (3) All discriminants... (4) All resultants..." [DNU23, §2.7, Definition 11]

测试 $T$ 的作用是：给定 $f \in \mathbb{Z}[x_1, \dots, x_m]$，返回一个有限点集 $N \subset \mathbb{R}^{m-1}$ 或 $\bot$；如果返回 $N$，则 $N$ 包含所有使 $f(\alpha_1, \dots, \alpha_{m-1}, x_m)$ 恒为零的点 [DNU23, §2.7, Hypothesis 1]。

---

## 3. Lazard Valuation（核心创新）

### 3.1 定义

设 $K$ 为域，$f \in K[x_1, \dots, x_n]$ 非零，$\alpha = (\alpha_1, \dots, \alpha_n) \in K^n$。$f$ 在 $\alpha$ 处的 **Lazard valuation**（或称 lex-least valuation）定义为 [MPP19, Definition 2.4]：

> "The Lazard valuation $v_\alpha(f)$ of $f$ at $\alpha$ is the element $v = (v_1, \dots, v_n)$ of $\mathbb{N}^n$ least (with respect to $\leq_{lex}$) such that $f$ expanded about $\alpha$ has a term $c(x_1 - \alpha_1)^{v_1} \cdots (x_n - \alpha_n)^{v_n}$ with $c \neq 0$." [MPP19, Definition 2.4]

等价地，可用偏导数刻画 [MPP15, Definition 1]：

> "$v_a(f)$ is the element $(v_1, \dots, v_n)$ of $\mathbb{N}^n$ least (with respect to $\leq_l$) such that the partial derivative $\partial^{v_1+\cdots+v_n} f / \partial x_1^{v_1} \cdots \partial x_n^{v_n}$ does not vanish at $a$." [MPP15, Definition 1]

### 3.2 基本性质

**性质 1（valuation 公理）** [MPP19, Proposition 3.1]：
$$v_\alpha(fg) = v_\alpha(f) + v_\alpha(g), \quad v_\alpha(f+g) \geq_{lex} \min\{v_\alpha(f), v_\alpha(g)\}$$

> "$v_\alpha(fg) = v_\alpha(f) + v_\alpha(g)$ and $v_\alpha(f+g) \geq_{lex} \min\{v_\alpha(f), v_\alpha(g)\}$ (if $f+g \neq 0$)." [MPP19, Proposition 3.1]

**性质 2（上半连续性）** [MPP19, Proposition 3.2]：
集合 $\{\gamma \in K^n : v_\gamma(f) \geq_{lex} v\}$ 是 $K^n$ 的代数子集。特别地，Lazard valuation 在 Zariski 拓扑（任意域）和经典拓扑（$K = \mathbb{R}$ 或 $\mathbb{C}$）中都是上半连续的。

> "The set $\{\gamma \in K^n; v_\gamma(f) \geq_{lex} v\}$ is an algebraic subset of $K^n$. In particular, the Lazard valuation is upper semi-continuous." [MPP19, Proposition 3.2]

**性质 3（乘积的 valuation-不变性）** [MPP19, Proposition 3.4]：
设 $S \subset K^n$ 连通，则 $fg$ 在 $S$ 上 valuation-invariant 当且仅当 $f$ 和 $g$ 都在 $S$ 上 valuation-invariant。

> "$fg$ is valuation-invariant in $S$ if and only if both $f$ and $g$ are valuation-invariant in $S$." [MPP19, Proposition 3.4]

### 3.3 Valuation-invariance vs Order-invariance

**$n=1$**：valuation 与 order 完全相同 [MPP15, Example 1]。

**$n=2$**：valuation-invariance **蕴含** order-invariance [MPP19, Proposition 3.6]：

> "Let $f \in K[x, y]$ be nonzero and $S \subset K^2$ be connected. If $f$ is valuation-invariant in $S$ then $f$ is order-invariant in $S$." [MPP19, Proposition 3.6]

**$n \geq 3$**：valuation-invariance **不蕴含** order-invariance [MPP15, §2]：

> "Let $f(x, y, z) = xz - y^2$ and let $S$ be the $z$-axis in $\mathbb{R}^3$. Now $f$ is valuation-invariant in $S$, since the valuation of $f$ at each point of $S$ is $(0, 2, 0)$. But $f$ is not order-invariant in $S$, since $\text{ord}_{(0,0,0)} f = 2$ and $\text{ord}_{(0,0,\alpha)} f = 1$ for $\alpha \neq 0$." [MPP15, §2]

这正是 McCallum 的 order-based 方法在 $n \geq 3$ 时无法处理某些非 well-oriented 情形的根本原因。

---

## 4. Lazard Evaluation（边界表示的关键）

### 4.1 定义

Lazard evaluation 是 Lazard lifting 的核心操作，它定义了**在 nullification 发生时如何正确地"求值"多项式**。

设 $n \geq 2$，$f \in K[x_1, \dots, x_n]$ 非零，$\alpha = (\alpha_1, \dots, \alpha_{n-1}) \in K^{n-1}$。$f$ 在 $\alpha$ 处的 **Lazard evaluation** $f_\alpha(x_n) \in K[x_n]$ 定义为以下过程的结果 [MPP19, Definition 2.6]：

> "$f_\alpha \leftarrow f$
> For $i \leftarrow 1$ to $n-1$ do
> &nbsp;&nbsp;&nbsp;&nbsp;$v_i \leftarrow$ the greatest integer $v$ such that $(x_i - \alpha_i)^v \mid f_\alpha$
> &nbsp;&nbsp;&nbsp;&nbsp;$f_\alpha \leftarrow f_\alpha / (x_i - \alpha_i)^{v_i}$
> &nbsp;&nbsp;&nbsp;&nbsp;$f_\alpha \leftarrow f_\alpha(\alpha_i, x_{i+1}, \dots, x_n)$" [MPP19, Definition 2.6]

### 4.2 关键观察

**观察 1**：$f_\alpha \neq 0$ 是该过程的不变式 [MPP19, Remark 2.8]。

> "Notice that the assertion '$f_\alpha \neq 0$' is an invariant of the above process." [MPP19, Remark 2.8]

**观察 2**：$f(\alpha, x_n) = 0$（恒为零）当且仅当某个 $v_i > 0$ [MPP19, Remark 2.8]。

> "$f(\alpha, x_n) = 0$ (identically) if and only if $v_i > 0$, for some $i$ in the range $1 \leq i \leq n-1$." [MPP19, Remark 2.8]

**观察 3**：$(v_1, \dots, v_{n-1})$ 正是 $f$ 在 $\alpha$ 上的 Lazard valuation（前 $n-1$ 个坐标） [MPP19, Remark 2.8]。

### 4.3 与普通代入的对比

**例子 1** [MPP19, Example 2.7]：
设 $f(x, y, z) = z^2 + y^2 + x^2 - 1$，$\alpha = (0, 0)$。
- 第一步（$i=1$）：$v_1 = 0$，$f_\alpha(y, z) = z^2 + y^2 - 1$
- 第二步（$i=2$）：$v_2 = 0$，$f_\alpha(z) = z^2 - 1$

此时 $f_\alpha(z) = f(0, 0, z)$，无 nullification。

**例子 2** [MPP19, Example 2.7]：
设 $f(x, y, z) = yz - x$，$\alpha = (0, 0)$。
- 第一步（$i=1$）：$v_1 = 0$，$f_\alpha(y, z) = yz$
- 第二步（$i=2$）：$v_2 = 1$，$f_\alpha(z) = yz / y = z$

此时 $f_\alpha(z) = z \neq f(0, 0, z) = 0$！

**这正是 McCallum 方法会丢失信息的地方，而 Lazard 方法通过除法恢复了正确的求值结果。**

### 4.4 算法伪代码

以下是 [Cow18, Algorithm 5] 中给出的 Lazard evaluation 算法：

```
Algorithm: Lazard evaluation
Input: A polynomial f ∈ R[x_1, ..., x_d], and
       samplepoints = [r_1, ..., r_{d-1}] ∈ R^{d-1}
Output: A univariate polynomial in x_d

Procedure Lazard evaluation:
    for j from 1 to d-1 do
        for i from 1 to degree(f, x_j) do
            if f is divisible by x_j - r_j then
                f ← f / (x_j - r_j)
            else
                break innermost for loop
            end if
        end for
    end for
    f ← Substitute the samplepoints into f
    return f
```
[Cow18, Algorithm 5, pp. 610–611]

---

## 5. Lazard Delineability 定理

### 5.1 Lazard Delineability 的定义

设 $f \in \mathbb{R}[x_1, \dots, x_n]$ 非零，$S \subset \mathbb{R}^{n-1}$。称 $f$ 在 $S$ 上 **Lazard delineable**，如果 [MPP19, Definition 2.10]：

> "1. the Lazard valuation of $f$ on $\alpha$ is the same for each point $\alpha \in S$;
> 2. there exist finitely many continuous functions $\theta_1 < \cdots < \theta_k$ from $S$ to $\mathbb{R}$, with $k \geq 0$, such that, for all $\alpha \in S$, the set of real roots of $f_\alpha(x_n)$ is $\{\theta_1(\alpha), \dots, \theta_k(\alpha)\}$;
> 3. there exist positive integers $m_1, \dots, m_k$ such that, for all $\alpha \in S$ and all $i$, $m_i$ is the multiplicity of $\theta_i(\alpha)$ as a root of $f_\alpha(x_n)$." [MPP19, Definition 2.10]

这些 $\theta_i$ 称为 **Lazard sections**，它们之间的区域称为 **Lazard sectors**。

### 5.2 Lazard 的主断言（Main Claim）

设 $A$ 是 $\mathbb{R}_n$ 中的有限不可约基，$n \geq 2$。设 $S$ 是 $\mathbb{R}^{n-1}$ 的连通子集。假设 $P_L(A)$ 的每个元素在 $S$ 上都是 valuation-invariant 的，则 [MPP19, §2]：

> "Then each element of $A$ is Lazard delineable on $S$, and the Lazard sections over $S$ of the elements of $A$ are pairwise disjoint." [MPP19, §2]

这是 Lazard 方法正确性的核心，由 [MPP19, Theorem 5.1] 及其推论严格证明。

### 5.3 核心定理（MPP 2019, Theorem 5.1）

设 $f(x, x_n) \in \mathbb{R}[x, x_n]$ 关于 $x_n$ 的正次数为 $d$，$D(x), l(x), t(x)$ 分别是 $f$ 的判别式、首项系数和尾项系数。设 $S \subset \mathbb{R}^{n-1}$ 是连通的解析子流形，且 $D, l, t$ 都在 $S$ 上 valuation-invariant。则 [MPP19, Theorem 5.1]：

> "$f$ is Lazard analytic delineable on $S$, hence $f$ is valuation-invariant in every Lazard section and sector over $S$. Moreover, the same conclusion holds for the polynomial $f^*(x, x_n) = x_n f(x, x_n)$." [MPP19, Theorem 5.1]

**注意**：这个定理要求 trailing coefficient $t(x)$ 非零且 valuation-invariant。这正是 trailing coefficient 必须出现在 $P_L$ 中的理论原因。

### 5.4 Brown-McCallum 的弱化版本

[Brown 04]（参见 [DNU23, Theorem 2.1]）给出了一个假设更弱的版本：

> "Suppose that $f(x_1, \dots, x_n) \in \mathbb{R}[x_1, \dots, x_n]$ has positive degree $d$ in $x_n$ and that $\text{disc}_{x_n}(f)$ and $\text{lc}_{x_n}(f)$ are non-zero... Let $S$ be a connected analytic submanifold of $\mathbb{R}^{n-1}$ in which $\text{disc}_{x_n}(f)$ and $\text{lc}_{x_n}(f)$ are lex-least invariant, and at no point of which $f$ vanishes identically. Then $f$ is analytic delineable on $S$..." [DNU23, Theorem 2.1]

这个版本去掉了对 $t(x)$ 的要求，但增加了 "at no point of which $f$ vanishes identically" 的限制——即引入了 **curtain** 的概念 [DNU23, §2.2, Definition 1]。

---

## 6. Lazard vs McCallum：胞腔边界表示对比

### 6.1 核心差异一览

| 特性 | McCallum CAD | Lazard CAD |
|------|-------------|-----------|
| **不变性基础** | Order-invariance | Valuation-invariance |
| **投影算子** | $P_M = \text{lc} \cup \text{all coeffs} \cup \text{disc} \cup \text{res}$ | $P_L = \text{lc} \cup \text{tc} \cup \text{disc} \cup \text{res}$ |
| **nullification 处理** | **算法失败**（要求 well-oriented） | **通过 Lazard evaluation 处理** |
| **边界多项式** | $f(\beta, x_{k+1})$（直接代入） | $f_\beta(x_{k+1})$（Lazard evaluation 后） |
| **根函数定义** | $f(\beta, x_{k+1}) = 0$ 的实根 | $f_\beta(x_{k+1}) = 0$ 的实根 |
| **胞腔类型** | Section / Sector | Lazard Section / Lazard Sector |
| **代数数需求** | Section 需要 | Section 需要（且更复杂） |

### 6.2 边界表示的本质不同

**McCallum 的边界**：
在样本点 $\beta \in \mathbb{R}^k$ 处，第 $k+1$ 维的边界由原始投影多项式 $P_{k+1}[j]$ 直接代入 $\beta$ 后的实根决定：
$$g(x_{k+1}) = P_{k+1}[j](\beta, x_{k+1})$$
如果 $g \equiv 0$，则 McCallum **失败**。

**Lazard 的边界**：
在样本点 $\beta \in \mathbb{R}^k$ 处，第 $k+1$ 维的边界由 Lazard evaluation 后的多项式决定：
$$g_\beta(x_{k+1}) = (P_{k+1}[j])_\beta(x_{k+1})$$
**保证 $g_\beta \not\equiv 0$**，因此总有有限个实根可以隔离。

### 6.3 一个具体例子

考虑 Chen 2023 [Che23, Example 2] 中的经典 non-well-oriented 例子：
$$f = w^2 + (x+y)^2 u - yz \in \mathbb{R}[x, y, z, u, w]$$

在投影到 $u$ 层时，得到 $(x+y)^2 u - yz$。当 $x+y = 0$ 且 $yz = 0$ 时（即 $y = 0, x = 0$ 的某条线上），该多项式在提升时会发生 nullification。

- **McCallum/Brown**：在 $V(x+y, yz)$ 上 nullification 发生，理论不保证正确性 [Che23, §6.1.1]
- **Lazard**：通过 Lazard evaluation，在这些点上用逐变量除法得到非零的 evaluated 多项式，继续正确地进行实根隔离和 lifting [MPP19, Theorem 5.1]

---

## 7. Nullification 的完整处理流程

### 7.1 Curtain 的概念

Davenport 等人在 [DNU23] 中系统引入了 **curtain** 术语来描述 nullification 发生的几何对象：

> "A variety $C \subseteq \mathbb{R}^n$ is called a curtain if, whenever $(x, x_n) \in C$, then $(x, y) \in C$ for all $y \in \mathbb{R}$. In other words, $C$ is a curtain if it is a union of fibres of $\mathbb{R}^n \to \mathbb{R}^{n-1}$." [DNU23, §2.2, Definition 1]

Curtain 分为三类 [DNU23, §2.2, Definition 3]：
- **Explicit curtain**：$f = gh$ 其中 $g \in \mathbb{R}[x_1, \dots, x_{n-1}]$
- **Implicit curtain**：不含 explicit curtain 但作为集合是 curtain
- **Mixed curtain**：包含 explicit curtain 但不是 explicit curtain 本身

### 7.2 Lazard Lifting 中的 Nullification 处理

在 Lazard lifting $L_L$ 中，当多项式 $g \in P^{n-k}(S)$ 在胞腔 $C$ 上 nullify 时 [DNU23, §2.6]：

> "The nullification problem of [McC98] is bypassed by 'Lazard lifting' $L_L$: when a polynomial $g \in P^{n-k}(S)$ nullifies on a cell $C$, its lex-least valuation is non-zero on $C$... The valuation is also constant over $C$ because $C$ is part of a lex-least-invariant cylindrical algebraic decomposition of $\mathbb{R}^k$. We replace $g$ by $g / (x_1 - \alpha_1)^{v_1} \cdots (x_k - \alpha_k)^{v_k}$ when lifting over $C$ if this valuation is $(v_1, \dots, v_k)$." [DNU23, §2.6]

### 7.3 处理流程（算法层面）

对于 Full Lazard CAD，在 lifting 的每一层 $k$：

1. 取当前胞腔 $C \subseteq \mathbb{R}^k$ 的样本点 $\beta = (\beta_1, \dots, \beta_k)$
2. 对第 $k+1$ 层的每个投影多项式 $p_j \in P_{k+1}$，计算 Lazard evaluation $(p_j)_\beta(x_{k+1})$，然后对 $(p_j)_\beta$ 做实根隔离
3. 合并所有 $(p_j)_\beta$ 的实根，排序后构造 Lazard sections 和 Lazard sectors
4. 对每个 sector，选取有理样本点；对每个 section，样本点是代数数（需要精确表示）

---

## 8. Full Lazard CAD 算法伪代码

基于 [MPP19, Algorithm 2.12] 和 [Cow18] 的算法描述，以下是 Full Lazard CAD 的完整流程：

### 8.1 主算法

```
Algorithm: VCADL (Valuation-invariant CAD using Lazard projection)
Input:  A = list of integral polynomials in x_1, ..., x_n
Output: (I, S) = lists of cell indices and sample points of an 
         A-valuation-invariant CAD of R^n

1. If n > 1, go to step 2.
   Otherwise:
   - Isolate real roots of irreducible factors of nonzero elements of A
   - Construct cell indices I and sample points S from the real roots
   - Exit

2. B ← finest squarefree basis for prim(A)
   P ← cont(A) ∪ P_L(B)
   (I', S') ← VCADL(P)   // Recursive call in n-1 dimensions

3. I ← empty list, S ← empty list
   For each α = (α_1, ..., α_{n-1}) in S' do:
       Let i be the index of the cell containing α
       
       // Key: compute Lazard evaluation product
       f* ← ∏_{f ∈ B} f_α   (each f_α constructed using Lazard evaluation)
       
       // If f* is a nonzero constant, no roots to isolate
       If f* ≠ 0 and deg(f*) > 0 then:
           Isolate real roots of f*
       
       Construct cell indices and sample points for Lazard sections 
       and Lazard sectors of elements of B from i, α, and the real roots of f*
       Add cell indices to I and sample points to S
   Exit
```

[改编自 MPP19, Algorithm 2.12, p. 6]

### 8.2 Lazard Lifting 的详细伪代码

以下伪代码是对 8.1 中 `VCADL` 步骤 3（即 lifting 阶段）的详细展开：

```
Algorithm: LazardLift
// 对应 VCADL 步骤 3 的 "For each α in S' do" 循环体
Input:  level_polys = list of projection polynomial lists {P_1, ..., P_n}
        vars = variable ordering [x_1, ..., x_n]
        parent_cell = cell C in R^{k-1} with sample point α = (α_1, ..., α_{k-1})
Output: List of child cells in R^k above C

1. evaluated_polys ← empty list
2. For each p in P_k do:
       p_α ← LazardEvaluation(p, α)   // 对应 VCADL 步骤 3 中的 f_α
       Append p_α to evaluated_polys
3. roots ← RealRootIsolation(evaluated_polys)   // 对应 "Isolate real roots of f*"
4. Sort roots in ascending order: r_1 < r_2 < ... < r_m
5. cells ← empty list
6. // Sector below lowest root
   sample ← ChooseRationalSample((-∞, r_1))
   Append SECTOR(bound=(-∞, r_1), sample=(α, sample)) to cells
7. For i = 1 to m do:
       // Lazard Section
       Append SECTION(bound=r_i, sample=(α, r_i)) to cells
       // Lazard Sector between roots
       If i < m then:
           sample ← ChooseRationalSample((r_i, r_{i+1}))
           Append SECTOR(bound=(r_i, r_{i+1}), sample=(α, sample)) to cells
8. // Sector above highest root
   sample ← ChooseRationalSample((r_m, +∞))
   Append SECTOR(bound=(r_m, +∞), sample=(α, sample)) to cells
9. Return cells   // 对应 VCADL 步骤 3 中的 "Add cell indices to I and sample points to S"
```

---

## 9. 参考文献

### 核心文献

- **[Col75]** G. E. Collins. "Quantifier elimination for real closed fields by cylindrical algebraic decomposition." In *Automata Theory and Formal Languages*, Springer, 1975, pp. 134–183.

- **[Laz94]** D. Lazard. "An improved projection for cylindrical algebraic decomposition." In *Algebraic geometry and its applications*, Springer, 1994, pp. 467–476.

- **[McC98]** S. McCallum. "An improved projection operation for cylindrical algebraic decomposition." In *Quantifier Elimination and Cylindrical Algebraic Decomposition*, Springer, 1998, pp. 242–268.

- **[Bro01]** C. W. Brown. "Improved projection for cylindrical algebraic decomposition." *Journal of Symbolic Computation*, 32(5):447–465, 2001.

- **[Bro04]** C. W. Brown. "McCallum vs. Lazard: an approach to lifting." Unpublished note, 2004. (Referenced in [DNU23] as the source of Theorem 2.1)

- **[MPP15]** S. McCallum and H. Hong. "On Lazard's valuation and CAD construction." arXiv:1501.06563 [math.AG], 2015.

- **[MPP19]** S. McCallum, A. Parusiński, and L. Paunescu. "Validity proof of Lazard's method for CAD construction." *Journal of Symbolic Computation*, 92:52–69, 2019. (Also arXiv:1607.00264 [math.AG])

- **[DNU23]** J. H. Davenport, A. Nair, G. Sankaran, and A. K. Uncu. "Lazard-style CAD and Equational Constraints." In *ISSAC 2023*, ACM, 2023, pp. 1–9. (arXiv:2302.05813 [cs.SC])

- **[Cow18]** A. I. Cowen-Rivers. "Towards Incremental Cylindrical Algebraic Decomposition." Summer Research Report for SC2 Project, arXiv:1804.08564 [cs.SC], 2018.

- **[Che23]** R. Chen. "A Geometric Approach to Cylindrical Algebraic Decomposition." arXiv:2311.10515 [math.AG], 2023.

### CLPoly 内部文档

- `docs/research/cad-cell-research.md` — CAD Cell 管理结构调研报告
- `docs/research/lazard-cad-research.md` — Lazard CAD 实现策略调研
