# Xia & Yang 2016 — 第2章 多项式系统的零点分解

> 历史阅读辅助：保留原整理日期和观点，尚未逐条校正，不作为当前接口、算法选择或正确性结论。OCR、译述和页码以原文为准；当前复核见 [CAD 文档入口](../../cad/README.md)。

> 来源：Xia & Yang, *Automated Inequality Proving and Discovering*, World Scientific, 2016
>
> 本文档按原书顺序整理 Chapter 2 的所有定义、定理、引理、命题、推论和算法，每一处均标注原文位置（章节/页码）。

---

## §2.1 Notations

### 基本记号（原文 §2.1, p.25）

变量序：$u_1 \prec \ldots \prec u_d \prec x_1 \prec \ldots \prec x_n$，其中 $u = \{u_1, \ldots, u_d\}$ 为参数，$x = \{x_1, \ldots, x_n\}$ 为变量。

对 $f \in K[u,x] \setminus \{0\}$，若 $x$ 出现在 $f$ 中，可写为
$$f = c_0 x^m + c_1 x^{m-1} + \ldots + c_m$$
其中 $c_0 \neq 0$。

- **$\deg(f, x)$**：$f$ 关于 $x$ 的首项次数（leading degree）
- **$\text{cls}(f)$**：$f$ 的类（class），即满足 $\deg(f, x_k) > 0$ 的最大下标 $k$；若所有 $\deg(f, x_i) = 0$ 则 $\text{cls}(f) = 0$
- **$\text{mvar}(f)$ 或 $\text{lv}(f)$**：$f$ 的主变元（main variable），即 $x_k$ 其中 $k = \text{cls}(f)$
- **$\text{lc}(f)$ 或 $\text{lc}_f$**：$f$ 的首项系数（leading coefficient / initial）
- **$\text{rank}(f)$**：$f$ 的秩，即 $x_k^m$ 其中 $k = \text{cls}(f)$，$m = \deg(f, x_k)$

对多项式集 $P$：$\text{lc}_P = \prod_{p \in P} \text{lc}_p$，$\text{mvar}(P) = \{\text{mvar}(p) \mid p \in P\}$，$\text{rank}(P) = \{\text{rank}(p) \mid p \in P\}$。

零集记号：
- $V(P) = \{(v_1, \ldots, v_d, a_1, \ldots, a_n) \in \tilde{K}^{d+n} \mid f(v_1, \ldots, v_d, a_1, \ldots, a_n) = 0, \forall f \in P\}$
- $V(P \setminus Q) = V(P) \setminus V(Q)$
- $V_{K'}(P)$：在域 $K'$ 中的零集

---

## §2.2 Wu's Zero Decomposition

### Definition 2.1 — Triangular Set（三角列）

> **原文位置**：§2.2, p.26

设 $T = [f_1, f_2, \ldots, f_t]$ 是 $K[u,x]$ 中有限非空非零多项式列表。$T$ 称为**三角列**（triangular set），如果
$$0 < \text{cls}(f_1) < \text{cls}(f_2) < \cdots < \text{cls}(f_t).$$

**Remark 2.1**（原文 p.26）：此定义与其他文献略有不同——若多项式列表包含 $K[u]$ 中的元素，则它不是三角列。

一般形式（公式 2.1，原文 p.26）：
$$T = \begin{bmatrix} f_1(u, x_1, \ldots, x_{k_1}) \\ f_2(u, x_1, \ldots, x_{k_1}, \ldots, x_{k_2}) \\ \vdots \\ f_t(u, x_1, \ldots, x_{k_1}, \ldots, x_{k_2}, \ldots, x_{k_t}) \end{bmatrix}$$
其中 $0 < k_1 < k_2 < \cdots < k_t \leq n$，$k_i = \text{cls}(f_i)$，$x_{k_i} = \text{lv}(f_i)$。

重命名后主变量为 $y_i = x_{k_i}$（公式 2.2，原文 p.26）：
$$T = [f_1(u', y_1), f_2(u', y_1, y_2), \ldots, f_t(u', y_1, \ldots, y_t)]$$
其中 $u'$ 是 $u$ 与非主变元的并集。

### Definition 2.2 — Successive Pseudo-remainder（逐次伪余式）

> **原文位置**：§2.2, p.26

设 $f \in K[u,x]$，$T$ 为形如 (2.1) 的三角列。定义：
$$\begin{aligned}
R_t &= f, \\
R_{t-1} &= \text{prem}(R_t, f_t, x_{k_t}), \\
R_{t-2} &= \text{prem}(R_{t-1}, f_{t-1}, x_{k_{t-1}}), \\
&\vdots \\
R_0 &= \text{prem}(R_1, f_1, x_{k_1}).
\end{aligned}$$

最后的伪余式 $R_0$ 称为 $f$ 关于 $T$ 的**逐次伪余式**（successive pseudo-remainder），记为 $\text{prem}(f; T)$ 或 $\text{prem}(f; f_t, \ldots, f_1)$。上述计算过程称为**逐次伪除法**（successive pseudo-division）。

对多项式集 $P$：$\text{prem}(P; T) = \{\text{prem}(p; T) \mid p \in P\}$。

**伪余式公式**（公式 2.3，原文 p.27）：
存在多项式 $q_i$ $(1 \leq i \leq t)$ 和整数 $d_i$ 使得
$$I_1^{d_1} \cdots I_t^{d_t} \cdot f = \sum_{i=1}^{t} q_i f_i + R_0$$
其中 $I_i$ 是 $f_i$ 的初式（initial）。

**约化**（reduced，原文 p.27）：若 $\deg(g, x_k) < \deg(f, x_k)$，则称 $g$ 关于 $f$ **约化**。若 $g$ 关于 $T$ 中每个多项式都约化，则称 $g$ 关于 $T$ **约化**。

逐次伪除法保证：$\deg(R_0, x_{k_i}) < \deg(f_i, x_{k_i})$ $(1 \leq i \leq t)$，即 $R_0$ 关于 $T$ 约化。

### Definition 2.3 — Ascending Set（升列）

> **原文位置**：§2.2, p.27

- **矛盾升列**（contradictory ascending set）：$T$ 是 $K[u]$ 中单个非零元素
- **非矛盾升列**（non-contradictory ascending set / non-contradictory ascending chain）：三角列 $T = [f_1, f_2, \ldots, f_t]$ 满足每个 $f_j$ 关于 $f_i$ 约化 $(1 \leq i < j \leq t)$

升列要么是矛盾升列，要么是非矛盾升列。

**Remark 2.3**（原文 p.27）：矛盾升列按 Definition 2.1 不是三角列。

### Definition 2.4 — Characteristic Set（特征列）

> **原文位置**：§2.2, p.27

非矛盾升列 $C$ 称为多项式集 $P \subseteq K[u,x]$ 的**特征列**，如果
$$C \subseteq \langle P \rangle_{K[u,x]} \quad \text{且} \quad \text{prem}(P; C) = \{0\}.$$

### Theorem 2.1 — Wu's Well-ordering Principle（吴文俊整序原理）

> **原文位置**：§2.2, p.28
>
> **引用**：[Wu (1978, 1994a)]

**定理**：存在一个算法，对输入多项式集 $P \subset K[u,x]$，输出要么一个矛盾升列（意味着 $V(P) = \emptyset$），要么一个（非矛盾的）特征列 $C: [f_1, \ldots, f_t]$ 使得
$$V(P) \subseteq V(C), \quad V(C \setminus \text{lc}(C)) \subseteq V(P).$$

由整序原理，显然有（公式 2.4，原文 p.28）：
$$V(P) = V(C \setminus \text{lc}(C)) \cup \bigcup_{j=1}^{t} V(P, I_j)$$
其中 $I_j$ 是 $f_j$ 的初式。

反复调用该算法分解所有 $V(P, I_i)$，最终得到升列 $C_1, \ldots, C_m$ 满足（公式 2.5，原文 p.28）：
- $C_i$ 要么是矛盾升列，要么是（非矛盾的）特征列
- 若 $m = 1$，则 $V(P) = \emptyset$；否则设 $S = \{C_i \mid 1 \leq i \leq m, C_i \text{ 是非矛盾特征列}\}$，则
$$V(P) = \bigcup_{C \in S} V(C \setminus \text{lc}(C))$$

升列集 $C_1, \ldots, C_m$ 或公式 (2.5) 称为 $P$ 在 $K[u,x]$ 中的**吴零点分解**（Wu's zero decomposition），算法记为 **WuCharSet**。

---

## §2.3 Relatively Simplicial Decomposition (RSD)

### 基本关系（原文 §2.3, p.29）

对多项式集 $P$ 和多项式 $f \in K[u,x]$，关注 $V(P)$ 与 $V(f)$ 的关系：
1. $V(P) \cap V(f) = \emptyset$：$P$ 与 $f$ **互素**（coprime）
2. $V(P) \subseteq V(f)$：$P$ 关于 $f$ **整相关**（integrally dependent）
3. 否则：$P$ 与 $f$ **相关**（dependent）

情形 (1) 或 (2) 中，$P$ 称为关于 $f$ **相对单纯的**（relatively simplicial）。

### Definition 2.5 — Successive Resultant（逐次结式）

> **原文位置**：§2.3.1, p.30

设 $f \in K[u,x]$，$T$ 为形如 (2.1) 的三角列。定义：
$$\begin{aligned}
R_t &= f, \\
R_{t-1} &= \text{res}(R_t, f_t), \\
R_{t-2} &= \text{res}(R_{t-1}, f_{t-1}), \\
&\vdots \\
R_0 &= \text{res}(R_1, f_1).
\end{aligned}$$

最后的结式 $R_0$ 称为 $f$ 关于 $T$ 的**逐次结式**（successive resultant），记为 $\text{res}(f; T)$ 或 $\text{res}(f; f_t, \ldots, f_1)$。

由 Lemma 1.1，存在非零多项式 $g_i$ $(0 \leq i \leq t)$ 使得（公式 2.6，原文 p.30）：
$$R_0 = \text{res}(f; f_t, \ldots, f_1) = g_0 f + \sum_{i=1}^{t} g_i f_i.$$

$T$ 的主变元不在 $R_0$ 中出现。若 $t = n$，则 $R_0 \in K[u]$。

### Definition 2.6 — Regular Chain（正则链）

> **原文位置**：§2.3.1, p.30

三角列 $T = \{f_1, \ldots, f_r\} \subset K[u,x]$ 称为 $K[u,x]$ 中的**正则链**，如果 $\text{lc}_{f_1} \neq 0$ 且对每个 $i$ $(1 < i \leq r)$，
$$\text{res}(\text{lc}_{f_i}; f_{i-1}, \ldots, f_1) \neq 0.$$

若 $\text{mvar}(T) = x$，则 $T$ 称为**零维正则链**（zero-dimensional regular chain）。

### Definition 2.7 — Squarefree（无平方）

> **原文位置**：§2.3.1, p.30

三角列 $T = \{f_1, \ldots, f_r\} \subset K[u,x]$ 称为 $K[u,x]$ 中**无平方的**，如果 $\text{discrim}(f_1) \neq 0$ 且对每个 $i$ $(1 < i \leq r)$，
$$\text{res}(\text{discrim}(f_i); f_{i-1}, \ldots, f_1) \neq 0.$$

### Proposition 2.1 — 正则链的非空性

> **原文位置**：§2.3.1, p.30

若 $T$ 是 $K[u,x]$ 中的正则链，$\tilde{K}$ 是 $K$ 的代数闭扩域，则 $V_{\tilde{K}}(T \setminus \text{lc}_T) \neq \emptyset$。

**推论**：正则链分解不存在"冗余分支问题"，因为正则链总有解。

### Theorem 2.2 — 多项式与正则链的互素判定

> **原文位置**：§2.3.1, p.30–31

设 $f \in K[u,x]$，$T$ 为形如 (2.2) 的正则链：
$$T = [f_1(u', y_1), f_2(u', y_1, y_2), \ldots, f_t(u', y_1, \ldots, y_t)]$$
$\tilde{K}$ 是 $K(u')$ 的代数闭扩域，则
$$V_{\tilde{K}}(T) \cap V_{\tilde{K}}(f) = \emptyset \iff \text{res}(f; T) \neq 0.$$

### Corollary 2.1 — 正则链的等价判定

> **原文位置**：§2.3.1, p.31

设 $T$ 为形如 (2.2) 的三角列，$\tilde{K}$ 是 $K(u')$ 的代数闭扩域，则 $T$ 是正则链当且仅当 $\text{lc}_{T_1} \neq 0$ 且对每个 $i$ $(1 < i \leq t)$，
$$V_{\tilde{K}}(\{f_1, \ldots, f_{i-1}\}) \cap V_{\tilde{K}}(\text{lc}_{T_i}) = \emptyset.$$

### Lemma 2.1 — 伪余式为零的推论

> **原文位置**：§2.3.1, p.31

设 $T$ 为形如 (2.2) 的三角列，$f \in K[u,x]$，则
$$\text{prem}(f; T) = 0 \implies V(T \setminus \text{lc}_T) \subseteq V(f).$$

**注**：$\text{prem}(f; T) = 0$ 不是 $V(T \setminus \text{lc}_T) \subseteq V(f)$ 的充要条件（见 Theorem 2.3）。

### Definition 2.8 — Saturated Ideal（饱和理想）

> **原文位置**：§2.3.1, p.31

设 $I$ 是环 $R$ 中的理想：
- **根理想**（radical）：$\sqrt{I} = \{g \in R \mid g^m \in I \text{ for some } m \geq 1\}$
- **关于 $f$ 的饱和理想**：$I : f^\infty = \{g \in R \mid f^s g \in I \text{ for some } s \geq 1\}$

三角列 $T$ 在 $K[u,x]$ 中的饱和理想定义为：
$$\text{sat}(T)_{K[u,x]} = \langle T \rangle_{K[u,x]} : \text{lc}_T^\infty.$$

### Theorem 2.3 — 伪余式与饱和理想的等价性

> **原文位置**：§2.3.1, p.31–32
>
> **引用**：[Wang (2002)] (pp. 72–73, pp. 180–182)

设 $T$ 为形如 (2.2) 的正则链，$f \in K[u,x]$，$\tilde{K}$ 是 $K(u')$ 的代数闭扩域，则
$$\text{prem}(f; T) = 0 \iff f \in \text{sat}(T),$$
$$V_{\tilde{K}}(T \setminus \text{lc}_T) \subseteq V_{\tilde{K}}(f) \iff f \in \sqrt{\text{sat}(T)}.$$

换言之：
- $\text{prem}(f; T) = 0 \iff \text{lc}_T^s f \in \langle T \rangle_{K[u,x]}$ 对某个正整数 $s$
- $V_{\tilde{K}}(T \setminus \text{lc}_T) \subseteq V_{\tilde{K}}(f) \iff \text{prem}(f^d; T) = 0$ 对某个正整数 $d$

### §2.3.2 RSD

#### Theorem 2.4 — 子结式与最大公因子

> **原文位置**：§2.3.2, p.32

设 $f \in K[x_1, \ldots, x_k]$，$T = [f_1(x_1), f_2(x_1, x_2), \ldots, f_k(x_1, \ldots, x_k)]$ 为 $K[x_1, \ldots, x_k]$ 中的零维正则链。将 $f$ 和 $f_k$ 视为 $x_k$ 的多项式，计算其关于 $x_k$ 的子结式链 $S_{\mu+1}, S_\mu, \ldots, S_0$，对应的主子结式系数（PSC）为 $R_j$ $(0 \leq j \leq \mu+1)$。

若
$$\text{prem}(R_0; f_{k-1}, \ldots, f_1) = \cdots = \text{prem}(R_{i-1}; f_{k-1}, \ldots, f_1) = 0$$
但
$$\text{res}(R_i; f_{k-1}, \ldots, f_1) \neq 0,$$
则 $S_i$ 是 $f$ 和 $f_k$ 在 $K[x_1, \ldots, x_k]/\text{sat}([f_1, \ldots, f_{k-1}])$ 中的**最高次公因子**。

**注**：若 $k = 1$，条件应理解为 $R_0 = \cdots = R_{i-1} = 0$ 但 $R_i \neq 0$。

#### Algorithm 2.1 — RSD (Relatively Simplicial Decomposition)

> **原文位置**：§2.3.2, p.33

**输入**：多项式 $f \in K[x_1, \ldots, x_t]$，零维正则链 $T: [f_1, \ldots, f_t] \subset K[x_1, \ldots, x_t]$。

**输出**：$T$ 的零点分解 $V(T) = \bigcup_{i \in A} V(T_i)$，使得每个 $T_i$ 是正则链且关于 $f$ 相对单纯。

```
1: if prem(f; T) = 0 or res(f; T) ≠ 0 then
2:     return {T}
3: end if
4: Let j be the least nonnegative integer such that
      prem(R_j(f, f_t); f_{t-1}, ..., f_1) ≠ 0,
      where R_j(f, f_t) is the principal subresultant coefficient
      corresponding to the j-th subresultant S_j(f, f_t) of f and f_t
      with respect to x_t;
5: if res(R_j(f, f_t); f_{t-1}, ..., f_1) ≠ 0 then
6:     compute, by Theorem 2.4, a common divisor with highest degree of
      f and f_t in K[x_1, ..., x_t]/sat([f_1, ..., f_{t-1}]) and denote it by f_{t1};
7:     f_{t2} ← pquo(f_t, f_{t1}, x_t);
8:     T_1 ← [f_1, ..., f_{t-1}, f_{t1}]; T_2 ← [f_1, ..., f_{t-1}, f_{t2}];
9:     return RSD(T_1, f) ∪ RSD(T_2, f);
10: else
11:     L' ← RSD([f_1, ..., f_{t-1}], R_j(f, f_t));
12:     return ⋃_{[f̄_1,...,f̄_{t-1}] ∈ L'} RSD([f̄_1, ..., f̄_{t-1}, f_t], f);
13: end if
```

#### Theorem 2.5 — RSD 算法的正确终止性

> **原文位置**：§2.3.2, p.33–34

**定理**：算法 RSD 正确终止。

**证明要点**：
- 第 6–9 行：当进入第 6 行时，$\text{prem}(f; T) \neq 0$ 但 $\text{res}(f; T) = 0$。由 Theorem 2.2，$f$ 与 $f_t$ 有非平凡公因子；由 $\text{prem}(f; T) \neq 0$，$f_t$ 在商环中不整除 $f$，故公因子 $f_{t1}$ 是真因子，$\deg(f_{t1}), \deg(f_{t2}) < \deg(f_t)$。
- 第 11 行：递归调用时升列中多项式个数减少。
- 每次递归调用，要么升列中多项式个数减少，要么多项式次数降低，故算法必终止。

---

## §2.4 Weakly RSD

### 来源说明（原文 §2.4, p.37）

本节内容主要来自 [Tang et al. (2014); Chen et al. (2014, 2015)]。

### Definition 2.9 — Parametric Triangular Decomposition（参数三角分解）

> **原文位置**：§2.4.1, p.37

设 $P$ 是 $K[u,x]$ 中的系统，$\tilde{K}$ 是 $K(u)$ 的代数闭扩域。

$K[u,x]$ 中三角列的有限集 $T$ 称为 $P$ 在 $K[u,x]$ 中的**参数三角分解**，如果
$$V_{\tilde{K}}(P) = \bigcup_{T \in T} V_{\tilde{K}}(T \setminus \text{lc}_T).$$

若 $T = \emptyset$ 或对所有 $T \in T$ 有 $V_{\tilde{K}}(T \setminus \text{lc}_T) \neq \emptyset$，则称该分解**非冗余**。

若 $T$ 是正则链的有限集，则称为**参数正则分解**。

### Definition 2.10 — Generic Zero-dimensional System（一般零维系统）

> **原文位置**：§2.4.1, p.37

设 $T$ 是 $K[u,x]$ 中系统 $P$ 的参数三角分解。若对每个 $T \in T$ 有 $\text{mvar}(T) = x$，则 $P$ 称为**一般零维系统**；否则称为**一般正维系统**。

### Definition 2.11 — WRSD (Weakly Relatively Simplicial Decomposition)

> **原文位置**：§2.4.1, p.38

设 $T$ 是 $K[u,x]$ 中的零维正则链，$f \in K[u,x]$，$\tilde{K}$ 是 $K(u)$ 的代数闭扩域。设 $H$ 和 $G$ 是 $K[u,x]$ 中零维正则链的两个有限集。若
1. $V_{\tilde{K}}(T, f) = \bigcup_{H \in H} V_{\tilde{K}}(H)$，且
2. $V_{\tilde{K}}(T \setminus f) = \bigcup_{G \in G} V_{\tilde{K}}(G)$，

则 $(H, G)$ 称为 $T$ 关于 $f$ 在 $\tilde{K}$ 上的**弱相对单纯分解**（WRSD）。

**Remark 2.5**（原文 p.38）：RSD 是 WRSD，但反之不成立。

### Definition 2.12 — Specializes Well（良好特化）

> **原文位置**：§2.4.1, p.38
>
> **引用**：[Chen et al. (2007)]

设 $T$ 是 $K[u,x]$ 中的正则链，$a \in \hat{K}^d$。若 $T(a)$ 是 $\hat{K}[x]$ 中的正则链且 $\text{rank}(T(a)) = \text{rank}(T)$，则称正则链 $T$ 在 $a$ 处**良好特化**。

对 $G \subset K[u]$，$V_u(G) = \{(a_1, \ldots, a_d) \in \hat{K}^d \mid g(a_1, \ldots, a_d) = 0, \forall g \in G\}$。

对 $f \in K[u,x]$，$V_u(f)$ 表示 $f$ 在 $x$ 中所有系数 $c_1, \ldots, c_t \in K[u]$ 的 $V_u(\{c_1, \ldots, c_t\})$。

### Lemma 2.2 — 良好特化的充要条件

> **原文位置**：§2.4.1, p.38
>
> **引用**：[Chen et al. (2007)]

设 $T$ 是 $K[u,x]$ 中的正则链。则 $T$ 在 $a$ 处良好特化当且仅当
$$a \in \hat{K}^d \setminus V_u(\text{res}(\text{lc}_T; T)).$$

### Definition 2.13 — Stable WRSD（稳定的 WRSD）

> **原文位置**：§2.4.1, p.38–39

设 $(H, G)$ 是零维正则链 $T$ 关于 $f \in K[u,x]$ 的 WRSD，$\hat{K}$ 是 $K$ 的代数闭扩域。WRSD $(H, G)$ 在 $a \in \hat{K}^d$ 处**稳定**，如果：
1. $T$ 在 $a$ 处良好特化；
2. $V_{\hat{K}}(T(a), f(a)) = \bigcup_{H \in H} V_{\hat{K}}(H(a))$ 且每个 $H \in H$ 在 $a$ 处良好特化；
3. $V_{\hat{K}}(T(a) \setminus f(a)) = \bigcup_{G \in G} V_{\hat{K}}(G(a))$ 且每个 $G \in G$ 在 $a$ 处良好特化。

### Definition 2.14 — Stable Decomposition（稳定分解）

> **原文位置**：§2.4.1, p.39

设 $T$ 是一般零维系统 $P$ 的参数三角分解，满足 $V_{\tilde{K}}(P) = \bigcup_{T \in T} V_{\tilde{K}}(T \setminus \text{lc}_T)$。对 $a \in \hat{K}^d$，若
$$V_{\hat{K}}(P(a)) = \bigcup_{T \in T} V_{\hat{K}}(T(a) \setminus \text{lc}_{T(a)}) \quad \text{且} \quad \text{rank}(T) = \text{rank}(T(a)) \; \forall T \in T,$$
则称分解 $T$ 在 $a$ 处**稳定**。

若 $T$ 是参数正则分解，$T$ 在 $a$ 处稳定指每个 $T \in T$ 在 $a$ 处良好特化且 $V_{\hat{K}}(P(a)) = \bigcup_{T \in T} V_{\hat{K}}(T(a) \setminus \text{lc}_{T(a)})$。

### Definition 2.15 — Generic Regular Decomposition (GRD)（一般正则分解）

> **原文位置**：§2.4.1, p.39

设 $T$ 是一般零维系统 $P$ 在 $K[u,x]$ 中的参数正则分解，$V$ 是 $\hat{K}^d$ 中仿射簇且 $\dim(V) < d$。若 $T$ 在任意 $a \in \hat{K}^d \setminus V$ 处稳定，则 $T$ 称为 $P$ 的**一般正则分解**（GRD），$V$ 称为 $T$ 的**正则分解不稳定簇**（RDU）。

对 $[T, p]$：$T$ 是上述分解且 $V_u(p)$ 是 RDU，即：
1. $T$ 是 $K[u,x]$ 中零维正则链的有限集，$V_{\tilde{K}}(P) = \bigcup_{T \in T} V_{\tilde{K}}(T)$；
2. $p \in K[u]$ 使得对任意 $a \in \hat{K}^d \setminus V_u(p)$，$V(P(a)) = \bigcup_{T \in T} V(T(a))$ 且每个 $T \in T$ 在 $a$ 处良好特化。

### Algorithm 2.2 — WRSD

> **原文位置**：§2.4.2, p.40

**输入**：零维正则链 $T = \{T_1, \ldots, T_n\} \subset K[u,x]$，多项式 $f \in K[u,x]$，变量 $x = \{x_1, \ldots, x_n\}$。

**输出**：$[H, G, p]$，其中
1. $(H, G)$ 是 $T$ 关于 $f$ 的 WRSD；
2. $p \in K[u]$ 使得对任意 $a \in \hat{K}^d \setminus V_u(p)$，$(H, G)$ 在 $a$ 处稳定。

```
 1: H ← ∅; G ← ∅; p ← res(lc_T; T);
 2: if f is not reduced w.r.t. T then return WRSD(T, prem(f; T), x); end if
 3: if f = 0 then return [{T}, ∅, p]; end if
 4: if cls(f) = 0 then return [∅, {T}, f · p]; end if
 5: if cls(f) ≠ n then
 6:     W ← WRSD({T_1, ..., T_cls(f)}, f, {x_1, ..., x_cls(f)});
 7:     H ← map(t → t ∪ {T_cls(f)+1, ..., T_n}, W_1);
 8:     G ← map(t → t ∪ {T_cls(f)+1, ..., T_n}, W_2); return [H, G, p · W_3];
 9: end if
10: if res(f; T) ≠ 0 then
11:     p ← p · res(f; T); G ← {T}; return [H, G, p];
12: end if
13: compute the regular subresultant chain S_dυ, ..., S_d0 of T_n and f w.r.t. x_n;
14: if n = 1 then
15:     q ← pquo(T_1, S_d1, x_1); W ← WRSD({q}, f, x);
16:     H ← {{S_d1}}; G ← W_2; p ← W_3;
17: else
18:     x_{n-1} ← x \ {x_n}; W ← WRSD({T_1, ..., T_{n-1}}, S_d0, x_{n-1});
19:     H_0 ← W_1; G_0 ← W_2; p ← W_3; G ← G ∪ map(t → t ∪ {T_n}, G_0);
20:     i ← 0; S_d(υ+1) ← T_n;
21:     while H_i ≠ ∅ do
22:         i ← i + 1; H_i ← ∅; G_i ← ∅;
23:         Let R_di be the d_i-th psc of T_n and f w.r.t. x_n;
24:         for H ∈ H_{i-1} do
25:             W ← WRSD(H, R_di, x_{n-1}); H_i ← H_i ∪ W_1; G_i ← G_i ∪ W_2; p ← p · W_3;
26:         end for
27:         for G ∈ G_i do
28:             H ← H ∪ {G ∪ {S_di}}; q ← pquo(T_n, S_di, x_n);
29:             if deg(q, x_n) > 0 then
30:                 W ← WRSD(G ∪ {q}, f, x_{n-1}); G ← G ∪ W_2; p ← p · W_3;
31:             end if
32:         end for
33:     end while
34: end if
35: return [H, G, p]
```

### §2.4.3 Correctness of Algorithm WRSD

#### Lemma 2.3 — WRSD 的核心引理

> **原文位置**：§2.4.3, p.41
>
> **引用**：灵感来自 [Wang (2000, 2001)]，但结果未被覆盖

设 $f, g \in K[u,x]$，$0 < \deg(g, x_n) < \deg(f, x_n)$。设 $S_{d_\upsilon}, \ldots, S_{d_1}, S_{d_0}$ 是 $f$ 和 $g$ 关于 $x_n$ 的**正则子结式链**。设 $S_{d_{\upsilon+1}} = f$，$R_{d_i}$ 是 $f$ 和 $g$ 关于 $x_n$ 的第 $d_i$ 个主子结式系数（PSC）$(0 \leq i \leq \upsilon+1)$。设 $q_{d_i}$ 是 $f$ 关于 $S_{d_i}$ 关于 $x_n$ 的伪商 $(1 \leq i \leq \upsilon)$。则：

**(1)** $V_{\tilde{K}}(\{f, g\} \setminus \text{lc}_f) = \bigcup_{i=1}^{\upsilon+1} V_{\tilde{K}}(\{S_{d_i}, R_{d_{i-1}}, \ldots, R_{d_0}\} \setminus \text{lc}_f R_{d_i})$

**(2)** $V_{\tilde{K}}(f \setminus g \cdot \text{lc}_f) = V_{\tilde{K}}(f \setminus g \cdot \text{lc}_f R_{d_0}) \cup \bigcup_{i=1}^{\upsilon} V_{\tilde{K}}(\{q_{d_i}, R_{d_{i-1}}, \ldots, R_{d_0}\} \setminus g \cdot \text{lc}_f R_{d_i})$

#### Lemma 2.4 — 结式非零的特化保持

> **原文位置**：§2.4.3, p.42

设 $f \in K[u,x]$，$T = \{T_1, \ldots, T_n\}$ 是 $K[u,x]$ 中的零维正则链。若 $S_0 = \text{res}(f; T) \neq 0$，则对任意 $a \in \hat{K}^d \setminus V_u(S_0 \cdot \text{res}(\text{lc}_T; T))$，有 $\text{res}(f(a); T(a)) \neq 0$。

#### Lemma 2.5 — 伪余式的零集等价

> **原文位置**：§2.4.3, p.42

设 $f \in K[u,x]$，$T = \{T_1, \ldots, T_n\}$ 是 $K[u,x]$ 中的零维正则链。设 $f_1 = \text{prem}(f; T)$，则
$$V_{\tilde{K}}(T, f) = V_{\tilde{K}}(T, f_1) \quad \text{且} \quad V_{\tilde{K}}(T \setminus f) = V_{\tilde{K}}(T \setminus f_1).$$

进一步，对任意 $a \in \hat{K}^d \setminus V_u(\text{res}(\text{lc}_T; T))$：
$$V_{\hat{K}}(T(a), f(a)) = V_{\hat{K}}(T(a), f_1(a)) \quad \text{且} \quad V_{\hat{K}}(T(a) \setminus f(a)) = V_{\hat{K}}(T(a) \setminus f_1(a)).$$

#### Theorem 2.6 — WRSD 算法的正确终止性

> **原文位置**：§2.4.3, p.42–43

**定理**：算法 WRSD 正确终止。

**证明概要**：对递归深度 $s$ 归纳。$s = 0$ 时由 Lemma 2.2 和 Lemma 2.4 得证。$s = N$ 时：
- 第 2 行或第 6 行的递归：由归纳假设和 Lemma 2.5
- 第 15 行或第 18 行的递归：$f$ 关于 $T$ 约化，$\text{mvar}(f) = x_n$，$\text{res}(f; T) = 0$

利用子结式链和 Lemma 2.3 证明条件 (1)(2)(3) 在 Definition 2.11 和 Definition 2.13 中成立。

---

## §2.5 Generic Regular Decomposition

### Algorithm 2.3 — ZDtoRC (Zero-dimensional Triangular set to Regular Chain)

> **原文位置**：§2.5, p.47

**输入**：三角列 $T = \{T_1, \ldots, T_n\} \subset K[u,x]$，满足 $\text{mvar}(T) = x$，变量 $x = \{x_1, \ldots, x_n\}$。

**输出**：$[G, p]$，其中
1. $G$ 是 $K[u,x]$ 中零维正则链的有限集，$V_{\tilde{K}}(T \setminus \text{lc}_T) = \bigcup_{G \in G} V_{\tilde{K}}(G)$；
2. $p \in K[u]$ 使得对任意 $a \in \hat{K}^d \setminus V_u(p)$，$V(T(a) \setminus \text{lc}_{T(a)}) = \bigcup_{G \in G} V(G(a))$ 且每个 $G \in G$ 在 $a$ 处良好特化。

```
 1: if T is a regular chain then return [{T}, res(lc_T, T)] end if;
 2: Find the minimal integer k (1 ≤ k < n) such that T_k = {T_1, ..., T_k}
      is a regular chain but T_{k+1} = {T_1, ..., T_k, T_{k+1}} is not;
 3: W ← WRSD(T_k, lc_{T_{k+1}}, {x_1, ..., x_k});
 4: if W_2 = ∅ then return [∅, W_3] end if;
 5: p ← W_3; G ← ∅;
 6: for R in W_2 do
 7:     R ← {op(R), T_{k+1}, ..., T_n}; Z ← ZDtoRC(R, x);
 8:     G ← G ∪ Z_1; p ← p · Z_2;
 9: end for
10: return [G, p]
```

### Theorem 2.7 — ZDtoRC 的正确终止性

> **原文位置**：§2.5, p.47–48

**定理**：算法 ZDtoRC 正确终止。

**证明**：若输入 $T$ 是正则链，显然终止。设 $T$ 不是正则链，$k$ 是最小整数使 $T_k$ 正则但 $T_{k+1}$ 不正则。假设不终止，则存在 $R \in \text{WRSD}(T_k, \text{lc}_{T_{k+1}}, \{x_1, \ldots, x_k\})_2$ 使 $\text{ZDtoRC}(R \cup T_{>k}, x)$ 不终止，进而得无穷序列 $k = k_1 < k_2 < \ldots$，但所有 $k_i \leq n$，矛盾。

### Algorithm 2.4 — GRDforZD (Generic Regular Decomposition for Zero-Dimensional systems)

> **原文位置**：§2.5, p.48

**输入**：一般零维系统 $P$ 在 $K[u][x]$ 中，变量 $x$。

**输出**：$[T, p]$，$P$ 的一般正则分解。

```
 1: Compute a Wu's decomposition {C_1, ..., C_m} of P in K[u][x];
 2: T ← ∅; p ← 1;
 3: for i = 1 to m do
 4:     if C_i is a contradictory ascending chain then
 5:         p ← p · op(C_i);
 6:     else
 7:         W ← ZDtoRC(C_i, x);
 8:         T ← T ∪ W_1; p ← p · W_2;
 9:     end if
10: end for
11: return [T, p]
```

### Lemma 2.6 — 正则链的包含保持

> **原文位置**：§2.5, p.48

设 $T$ 是 $K[u][x]$ 中的零维正则链，$P \subset K[u][x]$ 为有限多项式集，$V_{\tilde{K}}(T) \subset V_{\tilde{K}}(P)$。则对任意 $a \in \hat{K}^d \setminus V_u(\text{res}(\text{lc}_T, T))$，
$$V(T(a)) \subset V(P(a)).$$

### Theorem 2.8 — GRDforZD 的正确终止性

> **原文位置**：§2.5, p.48–49

**定理**：算法 GRDforZD 正确终止。

---

## §2.6 Zero Decomposition Keeping Multiplicity

### §2.6.1 Multiplicity

#### Definition 2.16 — Local Multiplicity（局部重数，经典定义）

> **原文位置**：§2.6.1, p.52

设 $I$ 是 $C[x]$ 中的零维理想，$\xi \in V(I)$。$\xi$ 作为 $V(I)$ 中点的**重数**定义为
$$\dim_C C[x]_{M_\xi} / I C[x]_{M_\xi}$$
即商空间 $C[x]_{M_\xi} / I C[x]_{M_\xi}$ 作为 $C$ 上向量空间的维数，其中 $M_\xi = \langle x_1 - \xi_1, \ldots, x_n - \xi_n \rangle$。

#### Definition 2.17 — Local Multiplicity via Dual Space（对偶空间定义）

> **原文位置**：§2.6.1, p.52
>
> **引用**：[Dayton and Zeng (2005)]

零维理想 $I \subseteq C[x]$ 的零点 $\xi$ 的**局部重数**为 $m$，如果对偶空间 $D_\xi(I)$ 的维数为 $m$。

**Remark 2.6**（原文 p.52）：Definition 2.17 与 Definition 2.16 等价。

#### Proposition 2.2 — 对偶空间的有限截断

> **原文位置**：§2.6.1, p.52
>
> **引用**：[Dayton and Zeng (2005)]

设 $\sigma$ 是满足 $\dim(D_\xi^\alpha(I)) = \dim(D_\xi^{\alpha+1}(I))$ 的最小 $\alpha$，则 $D_\xi(I) = D_\xi^\sigma(I)$。进一步，$\sigma < m = \dim(D_\xi(I))$。

#### Lemma 2.7 — 高次幂的消没

> **原文位置**：§2.6.1, p.52

设 $\xi$ 是 $I$ 的孤立零点，局部重数为 $m$。若 $\xi \in V(g(x))$ 其中 $g(x) \in C[x]$，则对任意 $v \in D_\xi(I)$，$h(x) \in C[x]$ 和 $l \geq m$，有 $v(h(x)g(x)^l) = 0$。

#### Definition 2.18 — Multiplicity for Triangular System（三角系统的重数）

> **原文位置**：§2.6.1, p.53
>
> **引用**：[Zhang et al. (2011)]

对零维三角系统
$$\begin{cases} f_1(x_1) = 0 \\ f_2(x_1, x_2) = 0 \\ \vdots \\ f_n(x_1, \ldots, x_n) = 0 \end{cases}$$
及其零点 $\xi = (\xi_1, \ldots, \xi_n)$，$\xi$ 的重数定义为 $\prod_{i=1}^{n} m_i$，其中 $m_i$ 是 $x_i = \xi_i$ 作为一元多项式 $f_i(\xi_1, \ldots, \xi_{i-1}, x_i)$ 的零点的重数。

**Remark 2.7**（原文 p.53）：Definition 2.18 对零维三角系统与 Definition 2.16 等价。

### §2.6.2 Zero Decomposition Keeping Multiplicity

#### Lemma 2.8 — 特征列保持重数

> **原文位置**：§2.6.2, p.53–54

设 $I = \langle P \rangle$ 是零维理想，$C = [C_1(x), \ldots, C_t(x)]$ 是 $P$ 的特征列满足 (2.4)，$J = \prod_{j=1}^{t} I_j$ 是所有 $C_i$ 初式的积。则对每个 $\xi \in V(C \setminus J)$，$\xi$ 作为 $V(I)$ 中点的局部重数与作为 $V(\langle C \rangle)$ 中点的局部重数相同。

#### Lemma 2.9 — 高次幂保持重数

> **原文位置**：§2.6.2, p.54

设 $\xi \in V(P, g(x))$ 是 $P = \{f_1(x), \ldots, f_s(x)\}$ 的孤立零点，$m$ 是 $\xi$ 作为 $V(P)$ 中点的局部重数。则 $\xi$ 作为 $V(P, g(x)^l)$ 中点的局部重数也是 $m$（若 $l \geq m$）。

#### Theorem 2.9 — Zero Decomposition with Multiplicity（保持重数的零点分解）

> **原文位置**：§2.6.2, p.54

设 $P$ 是 $C[x]$ 中有限非空非零多项式集，$\langle P \rangle$ 是零维理想。设 $C = [C_1(x), \ldots, C_n(x)]$ 是 $P$ 的特征列，$I_i$ 是 $C_i(x)$ 的初式，$J(i) = \prod_{k=1}^{i} I_k$ $(1 \leq i \leq n)$。

存在 $e_i \in \mathbb{N}$ $(2 \leq i \leq n)$ 使得
$$\text{MZero}(P) = \text{MZero}(C \setminus J(n)) \cup \bigcup_{i=2}^{n} \text{MZero}(P_i \setminus J(i-1))$$
是 $\text{MZero}(P)$ 的无交分解，其中 $P_i = P \cup \{I_i^{e_i}\}$（公式 2.7）。

**Remark 2.8**（原文 p.54）：$e_i$ 的存在性显然。例如可取 $e_i$ 为 Bézout 界，或设 $e_i \geq m$ 其中 $m = \dim_C C[x_1, \ldots, x_n] / \langle P \rangle$ 是 $\text{MZero}(P)$ 中点的个数（计重数），可通过计算 $P$ 的 Gröbner 基得到。

**Remark 2.9**（原文 p.54）：可对 Theorem 2.9 中每个 $P_i$ 递归应用 WuCharSet 得到三角分解。但如 [Li et al. (2010)] 指出，仅当满足某些额外约束时才能得到三角分解。

---

## 索引

### 定义索引

| 编号 | 名称 | 原文位置 |
|-----|------|---------|
| Definition 2.1 | Triangular Set（三角列） | §2.2, p.26, l.59 |
| Definition 2.2 | Successive Pseudo-remainder（逐次伪余式） | §2.2, p.26, l.100 |
| Definition 2.3 | Ascending Set（升列） | §2.2, p.27, l.134 |
| Definition 2.4 | Characteristic Set（特征列） | §2.2, p.27, l.141 |
| Definition 2.5 | Successive Resultant（逐次结式） | §2.3.1, p.30, l.215 |
| Definition 2.6 | Regular Chain（正则链） | §2.3.1, p.30, l.240 |
| Definition 2.7 | Squarefree（无平方） | §2.3.1, p.30, l.244 |
| Definition 2.8 | Saturated Ideal（饱和理想） | §2.3.1, p.31, l.295 |
| Definition 2.9 | Parametric Triangular Decomposition | §2.4.1, p.37, l.515 |
| Definition 2.10 | Generic Zero-dimensional System | §2.4.1, p.37, l.524 |
| Definition 2.11 | WRSD | §2.4.1, p.38, l.530 |
| Definition 2.12 | Specializes Well（良好特化） | §2.4.1, p.38, l.549 |
| Definition 2.13 | Stable WRSD | §2.4.1, p.38, l.563 |
| Definition 2.14 | Stable Decomposition | §2.4.1, p.39, l.571 |
| Definition 2.15 | Generic Regular Decomposition (GRD) | §2.4.1, p.39, l.579 |
| Definition 2.16 | Local Multiplicity (经典) | §2.6.1, p.52, l.1149 |
| Definition 2.17 | Local Multiplicity (对偶空间) | §2.6.1, p.52, l.1185 |
| Definition 2.18 | Multiplicity for Triangular System | §2.6.1, p.53, l.1203 |

### 定理索引

| 编号 | 名称 | 原文位置 |
|-----|------|---------|
| Theorem 2.1 | Wu's Well-ordering Principle | §2.2, p.28, l.147 |
| Theorem 2.2 | 多项式与正则链互素判定 | §2.3.1, p.30, l.252 |
| Theorem 2.3 | 伪余式与饱和理想等价 | §2.3.1, p.31, l.310 |
| Theorem 2.4 | 子结式与最大公因子 | §2.3.2, p.32, l.349 |
| Theorem 2.5 | RSD 正确终止 | §2.3.2, p.33, l.359 |
| Theorem 2.6 | WRSD 正确终止 | §2.4.3, p.42, l.739 |
| Theorem 2.7 | ZDtoRC 正确终止 | §2.5, p.47, l.958 |
| Theorem 2.8 | GRDforZD 正确终止 | §2.5, p.48, l.1030 |
| Theorem 2.9 | Zero Decomposition with Multiplicity | §2.6.2, p.54, l.1321 |

### 算法索引

| 编号 | 名称 | 原文位置 |
|-----|------|---------|
| Algorithm 2.1 | RSD | §2.3.2, p.33, l.370 |
| Algorithm 2.2 | WRSD | §2.4.2, p.40, l.595 |
| Algorithm 2.3 | ZDtoRC | §2.5, p.47, l.935 |
| Algorithm 2.4 | GRDforZD | §2.5, p.48, l.980 |

### 引理索引

| 编号 | 名称 | 原文位置 |
|-----|------|---------|
| Lemma 2.1 | 伪余式为零的推论 | §2.3.1, p.31, l.287 |
| Lemma 2.2 | 良好特化的充要条件 | §2.4.1, p.38, l.559 |
| Lemma 2.3 | WRSD 核心引理 | §2.4.3, p.41, l.676 |
| Lemma 2.4 | 结式非零的特化保持 | §2.4.3, p.42, l.721 |
| Lemma 2.5 | 伪余式的零集等价 | §2.4.3, p.42, l.730 |
| Lemma 2.6 | 正则链的包含保持 | §2.5, p.48, l.1001 |
| Lemma 2.7 | 高次幂的消没 | §2.6.1, p.52, l.1193 |
| Lemma 2.8 | 特征列保持重数 | §2.6.2, p.53, l.1236 |
| Lemma 2.9 | 高次幂保持重数 | §2.6.2, p.54, l.1283 |

### 命题索引

| 编号 | 名称 | 原文位置 |
|-----|------|---------|
| Proposition 2.1 | 正则链非空性 | §2.3.1, p.30, l.248 |
| Proposition 2.2 | 对偶空间有限截断 | §2.6.1, p.52, l.1190 |

### 推论索引

| 编号 | 名称 | 原文位置 |
|-----|------|---------|
| Corollary 2.1 | 正则链等价判定 | §2.3.1, p.31, l.274 |
