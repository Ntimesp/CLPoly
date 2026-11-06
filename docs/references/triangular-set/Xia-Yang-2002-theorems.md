# Xia & Yang 2002 — 半代数系统实解隔离算法

> 历史阅读辅助：保留原整理日期和观点，尚未逐条校正，不作为当前接口、算法选择或正确性结论。OCR、译述和页码以原文为准；当前复核见 [CAD 文档入口](../../cad/README.md)。

> **来源**：Xia B., Yang L. *An Algorithm for Isolating the Real Solutions of Semi-algebraic Systems*. J. Symbolic Computation (2002) 34, 461–477
> **原文提取**：`Xia-Yang-2002-raw.txt`（pdftotext）
> **整理日期**：2026-04-21

---

## 目录

- [§1 引言](#1-引言)
- [§2 基本算法](#2-基本算法)
  - [定义 2.1 — 判别式](#定义-21--判别式)
  - [定义 2.2 — 三角列上的结式与伪余式](#定义-22--三角列上的结式与伪余式)
  - [定义 2.3 — 正规升列](#定义-23--正规升列)
  - [定义 2.4 — 临界多项式](#定义-24--临界多项式)
  - [定义 2.5 — 正则三角半代数系统](#定义-25--正则三角半代数系统)
  - [§2.1 正则三角半代数系统](#21-正则三角半代数系统)
    - [定理 2.1 — 实解个数不变性](#定理-21--实解个数不变性)
    - [算法 nearzero](#算法-nearzero)
  - [§2.2 非正则三角半代数系统](#22-非正则三角半代数系统)
    - [定义 2.6 — 单纯升列](#定义-26--单纯升列)
    - [定理 2.2 — wr 分解](#定理-22--wr-分解)
    - [定义 2.7 — 判别矩阵](#定义-27--判别矩阵)
    - [定义 2.8 — 判别式序列](#定义-28--判别式序列)
    - [定义 2.9 — 主子结式](#定义-29--主子结式)
    - [定义 2.10 — 子结式多项式链](#定义-210--子结式多项式链)
    - [定理 2.3 — gcd 与伪余主子结式](#定理-23--gcd-与伪余主子结式)
    - [定理 2.4 — 非正则系统分解为正则系统](#定理-24--非正则系统分解为正则系统)
- [§3 提升与递归](#3-提升与递归)
  - [算法 REALZERO](#算法-realzero)
  - [子算法 SHR](#子算法-shr)
- [§4 程序 realzero 与实例](#4-程序-realzero-与实例)
- [附录](#附录)

---

## §1 引言

> **原文位置**：§1, p.461

**半代数系统**（semi-algebraic system, sas）：由多项式方程、不等式（≥, >）和不等方程（≠）组成的系统：

$$\begin{cases}
p_1(x_1, \ldots, x_s) = 0, \ldots, p_n(x_1, \ldots, x_s) = 0, \\
g_1(x_1, \ldots, x_s) \geq 0, \ldots, g_r(x_1, \ldots, x_s) \geq 0, \\
g_{r+1}(x_1, \ldots, x_s) > 0, \ldots, g_t(x_1, \ldots, x_s) > 0, \\
h_1(x_1, \ldots, x_s) \neq 0, \ldots, h_m(x_1, \ldots, x_s) \neq 0
\end{cases}$$

其中 $n, s \geq 1$，$r, t, m \geq 0$，所有多项式系数为整数。假设 $\{p_1, \ldots, p_n\}$ 只有有限多个公共零点。

**三角半代数系统**（triangular semi-algebraic system, tsa）：方程部分已为三角列形式：

$$\begin{cases}
f_1(x_1) = 0, \\
f_2(x_1, x_2) = 0, \\
\cdots \cdots \\
f_s(x_1, \ldots, x_s) = 0, \\
g_1 \geq 0, \ldots, g_r \geq 0, \\
g_{r+1} > 0, \ldots, g_t > 0, \\
h_1 \neq 0, \ldots, h_m \neq 0
\end{cases}$$

其中 $\{f_1, \ldots, f_s\}$ 是**正规升列**。

---

## §2 基本算法

> **原文位置**：§2, p.462–466

### 定义 2.1 — 判别式

> **原文位置**：§2, p.462

给定多项式 $g(x)$，$g$ 与 $g'_x$ 关于 $x$ 的 Sylvester 结式称为 $g$ 关于 $x$ 的**判别式**，记为 $\text{dis}(g, x)$ 或 $\text{dis}(g)$。

---

### 定义 2.2 — 三角列上的结式与伪余式

> **原文位置**：§2, p.462

给定多项式 $g$ 和三角列 $\{f_1, f_2, \ldots, f_s\}$，定义：

$$r_s := g, \quad r_{s-i} := \text{resultant}(r_{s-i+1}, f_{s-i+1}, x_{s-i+1}), \quad i = 1, 2, \ldots, s$$

$$q_s := g, \quad q_{s-i} := \text{prem}(q_{s-i+1}, f_{s-i+1}, x_{s-i+1}), \quad i = 1, 2, \ldots, s$$

$r_{i-1}$ 和 $q_{i-1}$ 分别称为 $g$ 关于三角列 $\{f_i, f_{i+1}, \ldots, f_s\}$ 的**结式**和**伪余式**，记为 $\text{res}(g, f_s, \ldots, f_i)$ 和 $\text{prem}(g, f_s, \ldots, f_i)$。

---

### 定义 2.3 — 正规升列

> **原文位置**：§2, p.462

给定三角列 $\{f_1, f_2, \ldots, f_s\}$，记 $I_i$ 为 $f_i$ 在 $x_i$ 中的首项系数。如果

$$\text{res}(I_i, f_{i-1}, \ldots, f_1) \neq 0, \quad i = 2, \ldots, s$$

则称该三角列为**正规升列**（normal ascending chain）。

**注**：正规升列也称为正则列（regular chain, Kalkbrener 1993）或正则集（regular set, D. M. Wang 2000）。

---

### 定义 2.4 — 临界多项式

> **原文位置**：§2, p.462–463

给定 tsa $T$，对每个 $f_i$（$i \geq 2$）：

$$\text{CP}_{f_2} = \text{dis}(f_2, x_2), \quad \text{CP}_{f_i} = \text{res}(\text{dis}(f_i, x_i), f_{i-1}, f_{i-2}, \ldots, f_2), \quad i > 2$$

对任意 $q \in \{g_j \mid 1 \leq j \leq t\} \cup \{h_k \mid 1 \leq k \leq m\}$：

$$\text{CP}_q = \begin{cases} \text{res}(q, f_s, f_{s-1}, \ldots, f_2), & s > 1, \\ q, & s = 1. \end{cases}$$

**临界多项式**：

$$\text{CP}_T(x_1) = \prod_{2 \leq i \leq s} \text{CP}_{f_i} \cdot \prod_{1 \leq j \leq t} \text{CP}_{g_j} \cdot \prod_{1 \leq k \leq m} \text{CP}_{h_k}$$

**意义**：$x_1$ 在不含 $\text{CP}_T(x_1)$ 实零点的区间上变化时，$T$ 去掉 $f_1$ 后的实解个数保持不变。

---

### 定义 2.5 — 正则三角半代数系统

> **原文位置**：§2, p.463

tsa $T$ 称为**正则的**，如果

$$\text{resultant}(f_1(x_1), \text{CP}(x_1), x_1) \neq 0$$

**注**：正则 tsa 中，所有 $h_k \neq 0$ 自动满足，可省略；所有 $g_j \geq 0$ 可视为 $g_j > 0$。

---

### §2.1 正则三角半代数系统

> **原文位置**：§2.1, p.463–464

#### 算法 nearzero

> **原文位置**：§2.1, p.463

**输入**：$p(x), q(x) \in \mathbb{Z}[x]$，$p$ 与 $q$ 无公共零点，变元 $x$，可选精度 $\epsilon$
**输出**：$p(x)$ 的所有互异实零点 $\alpha_1 < \alpha_2 < \cdots < \alpha_n$ 的隔离区间 $[a_1, b_1], \ldots, [a_n, b_n]$，满足：

1. $\alpha_i \in [a_i, b_i]$
2. $[a_i, b_i] \cap [a_j, b_j] = \emptyset$（$i \neq j$）
3. $a_i, b_i$ 均为有理数
4. 区间长度可小于任意预设正数
5. 每个区间内不含 $q(x)$ 的零点

基于**改进的 Uspensky 算法**。

---

#### 定理 2.1 — 实解个数不变性

> **原文位置**：§2.1, p.464

设 $T$ 为正则 tsa，$f_1(x_1)$ 有 $n$ 个互异实零点。调用 $\text{nearzero}(f_1, \text{CP}, x_1)$ 得到隔离区间 $[a_1, b_1], \ldots, [a_n, b_n]$。

对任意 $[a_i, b_i]$ 和任意 $\beta, \gamma \in [a_i, b_i]$：

**(1)** 若 $s > 1$，则系统
$$\begin{cases} f_2(\beta, x_2) = 0, \ldots, f_s(\beta, x_2, \ldots, x_s) = 0, \\ g_1(\beta, x_2, \ldots, x_s) > 0, \ldots, g_t(\beta, x_2, \ldots, x_s) > 0 \end{cases}$$
与将 $\beta$ 换为 $\gamma$ 的系统有**相同个数的互异实解**。

**(2)** 若 $s = 1$，则对任意 $g_j$，$\text{sign}(g_j(\beta)) = \text{sign}(g_j(\gamma))$。

---

### §2.2 非正则三角半代数系统

> **原文位置**：§2.2, p.464–466

#### 定义 2.6 — 单纯升列

> **原文位置**：§2.2, p.464

正规升列 $\{f_1, \ldots, f_s\}$ 关于多项式 $g$ 是**单纯的**（simplicial），如果

$$\text{prem}(g, f_s, \ldots, f_1) = 0 \quad \text{或} \quad \text{res}(g, f_s, \ldots, f_1) \neq 0$$

---

#### 定理 2.2 — wr 分解

> **原文位置**：§2.2, p.464

对三角列 $AS: \{f_1, \ldots, f_s\}$ 和多项式 $g$，存在算法将 $AS$ 分解为若干正规升列 $AS_i: \{f_{i1}, f_{i2}, \ldots, f_{is}\}$（$1 \leq i \leq n$），使得每个升列关于 $g$ 都是单纯的，且

$$\text{Zero}(AS) = \bigcup_{1 \leq i \leq n} \text{Zero}(AS_i)$$

---

#### 定义 2.7 — 判别矩阵

> **原文位置**：§2.2, p.464

对 $f(x) = a_0 x^n + a_1 x^{n-1} + \cdots + a_n$，其**判别矩阵** $\text{Discr}(f)$ 为 $2n \times 2n$ 矩阵：

$$\begin{bmatrix}
a_0 & a_1 & a_2 & \cdots & a_n \\
0 & na_0 & (n-1)a_1 & \cdots & a_{n-1} \\
& a_0 & a_1 & \cdots & a_{n-1} & a_n \\
& 0 & na_0 & \cdots & 2a_{n-2} & a_{n-1} \\
& & \cdots & \cdots \\
a_0 & a_1 & a_2 & \cdots & a_n \\
0 & na_0 & (n-1)a_1 & \cdots & a_{n-1}
\end{bmatrix}$$

---

#### 定义 2.8 — 判别式序列

> **原文位置**：§2.2, p.465

设 $d_k$ 为 $\text{Discr}(f)$ 的前 $k$ 行和前 $k$ 列构成的子矩阵的行列式。令 $D_k = d_{2k}$（$k = 1, \ldots, n$），称 $[D_1, D_2, \ldots, D_n]$ 为 $f(x)$ 的**判别式序列**。$D_n = \text{dis}(f, x)$。

---

#### 定义 2.9 — 主子结式

> **原文位置**：§2.2, p.465

$D_{kt}$ 为 $\text{Discr}(f)$ 的前 $2n-2k$ 行、前 $2n-2k-1$ 列和第 $(2n-2k+t)$ 列构成的子矩阵（$0 \leq k \leq n-1, 0 \leq t \leq 2k$）。$|D_{k0}|$ 称为 $f(x)$ 的**第 $k$ 主子结式**，$|D_{k0}| = D_{n-k}$。

---

#### 定义 2.10 — 子结式多项式链

> **原文位置**：§2.2, p.465

$Q_{n+1}(f, x) = f(x)$，$Q_n(f, x) = f'(x)$，对 $k = 0, 1, \ldots, n-1$：

$$Q_k(f, x) = \sum_{t=0}^k |D_{kt}| x^{k-t} = |D_{k0}| x^k + |D_{k1}| x^{k-1} + \cdots + |D_{kk}|$$

称 $\{Q_0(f, x), \ldots, Q_{n+1}(f, x)\}$ 为 $f(x)$ 的**子结式多项式链**。

---

#### 定理 2.3 — gcd 与伪余主子结式

> **原文位置**：§2.2, p.465

设 $\{f_1, \ldots, f_j\}$ 是正规升列，$f(x) = a_0 x^n + \cdots + a_n \in K[x_1, \ldots, x_j][x]$。令 $PD_k = \text{prem}(|D_{k0}|, f_j, \ldots, f_1) = \text{prem}(D_{n-k}, f_j, \ldots, f_1)$。

若存在 $k_0 \geq 0$ 使得 $\text{res}(a_0, f_j, \ldots, f_1) \neq 0$，$PD_0 = \cdots = PD_{k_0-1} = 0$，$\text{res}(|D_{k_00}|, f_j, \ldots, f_1) \neq 0$，则

$$\gcd(f, f'_x) = Q_{k_0}(f, x) \quad \text{在} \quad K[x_1, \ldots, x_j]/(f_1, \ldots, f_j) \text{中}$$

---

#### 定理 2.4 — 非正则系统分解为正则系统

> **原文位置**：§2.2, p.465–466

对非正则 tsa $T$，存在算法将 $T$ 分解为正则系统 $T_i$，使得

$$\text{Rzero}(T) = \bigcup \text{Rzero}(T_i)$$

**证明思路**：
- 若某 $\text{CP}_{h_k}$ 与 $f_1$ 有公共零点，对 $\{f_1, \ldots, f_s\}$ 做 wr 分解（关于 $h_k$），取 $h_k \neq 0$ 的分支
- 若某 $\text{CP}_{g_j}$ 与 $f_1$ 有公共零点，做 wr 分解（关于 $g_j$），$g_j > 0$ 时取 $g_j \neq 0$ 分支；$g_j \geq 0$ 时拆分为 $g_j > 0$ 和 $g_j = 0$ 两个分支
- 若某 $\text{CP}_{f_i}$ 与 $f_1$ 有公共零点，利用判别式序列和定理 2.3 求 $\gcd(f_i, f'_{i,x_i})$，用伪商替换 $f_i$

---

## §3 提升与递归

> **原文位置**：§3, p.466–469

### 算法 REALZERO

> **原文位置**：§3, p.467–468

**输入**：正则 tsa $T$ 和可选参数 $w$（输出区间最大长度）
**输出**：$T$ 的实解隔离区间，或报告失败

**Step 1**. $i \leftarrow 1$

若 $\text{resultant}(f_i(x_i), \text{MPT}^{(i)}(x_i), x_i) = 0$，返回 fail；否则

$$S^{(i)} \leftarrow \text{nearzero}(f_i(x_i), \text{CP}_T^{(i)} \cdot \text{MPT}^{(i)}, x_i)$$

**Step 2**. 对 $S^{(i)}$ 中每个区间 $I$ 执行：

**Step 2a**. 设 $I = [a^{(1)}, b^{(1)}] \times \cdots \times [a^{(i)}, b^{(i)}]$，令 $V_I$ 为 $I$ 的所有顶点。

**Step 2b**. 对 $V_I$ 中每个顶点 $(v_j^{(1)}, \ldots, v_j^{(i)})$：
- 代入 $x_1 = v_j^{(1)}, \ldots, x_i = v_j^{(i)}$ 到 $T$ 中
- 删除前 $i$ 个方程
- 记新系统为 $T_j^{(i+1)}$
- 若 $\text{resultant}(f_{i+1}, \text{MPT}^{(i+1)}, x_{i+1}) = 0$，返回 fail
- 否则 $R_j^{(i+1)} \leftarrow \text{nearzero}(f_{i+1}, \text{CP}_T^{(i+1)} \cdot \text{MPT}^{(i+1)}, x_{i+1})$

**Step 2c**. 合并所有 $R_j^{(i+1)}$ 为 $R^{(i+1)}$：

$$\alpha_k^{(i+1)} = \min_j \alpha_{j,k}^{(i+1)}, \quad \beta_k^{(i+1)} = \max_j \beta_{j,k}^{(i+1)}$$

若 $R^{(i+1)}$ 中有相交区间或最大长度 $> w$，则 $I \leftarrow \text{SHR}(I)$，返回 Step 2a；否则 $S_I^{(i+1)} \leftarrow I \times R^{(i+1)}$。

**Step 3**. $S^{(i+1)} \leftarrow \bigcup_{I \in S^{(i)}} S_I^{(i+1)}$，$i \leftarrow i + 1$。若 $i < s$，返回 Step 2。

**Step 4**. 对 $S^{(s)}$ 中每个 $s$ 维立方体，代入顶点检验不等式，满足则输出。

---

### 子算法 SHR

> **原文位置**：§3, p.468–469

**输入**：$k$ 维立方体 $I_0 = [a_1, b_1] \times \cdots \times [a_k, b_k]$
**输出**：$I \subset I_0$

**Step 0**. 取 $[a'_1, b'_1] \subset [a_1, b_1]$ 含 $f_1$ 的唯一零点，$b'_1 - a'_1 = (b_1 - a_1)/10$。

**Step 1–4**. 逐维提升：对每维 $i$，在顶点处代入计算 $Q_j^{(i+1)}$，合并得到 $Q^{(i+1)}$，取对应区间 $[a'_{i+1}, b'_{i+1}]$，精度为上一维的 $1/10$。

---

## §4 程序 realzero 与实例

> **原文位置**：§4, p.469–474

**realzero** 是 Maple 实现的程序，调用方式：

```maple
realzero([p1,...,pn], [q1,...,qr], [g1,...,gt], [h1,...,hm], [x1,...,xs]);
realzero([...], [..], [..], [..], [..], width);       # 指定最大宽度
realzero([...], [..], [..], [..], [..], [w1,...,ws]); # 每维指定宽度
```

**三步骤**：
1. 用 Ritt-Wu 方法（wsolve）将方程组化为三角形式
2. 对每个分支，若非正则则用定理 2.4 化为正则 tsa
3. 对每个正则 tsa 调用 REALZERO

**实例**：化学反应、神经网络、Cyclic-5、Lotka-Volterra、几何约束、p-3-p 问题等。

---

## 附录

### 附录 A：计时数据

> **原文位置**：Appendix A, p.475

Pentium 933 MHz, 128 MB, Maple 5.4

| 例号 | 三角化 | 正则化 | REALZERO | 总计 |
|-----|--------|--------|----------|------|
| Ex.2 (化学反应) | 0.107s | 0.005s | 0.396s | 0.508s |
| Ex.3 (神经网络) | 0.623s | 1.578s | 15.382s | 17.583s |
| Ex.4 (Cyclic-5) | 2.011s | 2.772s | 2.889s | 7.672s |
| Ex.5 (竞争扩散) | 0.384s | 2.682s | 3.070s | 6.136s |
| Ex.6 (几何约束) | 0.014s | 0.015s | 0.450s | 0.479s |
| Ex.7 (p-3-p) | 0.137s | 1.774s | 33.840s | 35.751s |

### 附录 B：正则三角半代数系统结果

> **原文位置**：Appendix B, p.475–477

各实例分解后的正则 tsa 的具体多项式方程。

---

> **与 CLPoly 的关联**：
> - `charset.hh` 中的 `charset<Tc>` 对应 Ritt-Wu 方法（步骤 1）
> - `wrsd<Tc>` / `Wucharset<Tc>` 对应 wr 分解（步骤 2）
> - `ZDtoRC<Tc>` 对应正则列转换
> - **REALZERO 算法尚未在 CLPoly 中实现**——这是 CAD 路径上的关键缺失
> - `nearzero` 对应单变量实根隔离（Uspensky 算法），CLPoly 需要类似的实根隔离模块
