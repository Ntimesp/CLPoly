> **2026-09-24：** 下文是历史 OCR 笔记，应以原 PDF 为准。现行规范已恢复严格采用 XZ06 路线，见 [Lazard revision 2](../../cad/theory/lazard-evaluation.pdf)。QQ 界多项式不等于目标根定义式；正半轴、正则平方自由及 NSHR 证书条件必须完整追踪。

> 历史阅读辅助：保留原整理日期和观点，尚未逐条校正，不作为当前接口、算法选择或正确性结论。OCR、译述和页码以原文为准；当前复核见 [CAD 文档入口](../../cad/README.md)。

# Xia & Zhang 2006 — 区间算术实解隔离（定理与算法笔记）

> 论文：Bican Xia, Ting Zhang, "Real Solution Isolation Using Interval Arithmetic",
> Computers and Mathematics with Applications 52: 853–860, 2006.
> PDF：`Xia-Zhang-2006-Real-Solution-Isolation-Using-Interval-Arithmetic.pdf`（扫描版，本文基于 OCR）
> 定位：**CLPoly 代数数模块实根隔离与精化的主参考**（架构 v2.1 确立）

---

## 1. 设定与记号

- **SAS**（半代数系统）：方程 P + 不等式 G₁(≥)、G₂(>) + 不等方程 H(≠)，零维（有限个复零点）。
- **TSA**（三角半代数系统）：方程部分已化为三角列/正规升列（= 正则链，[12][13][14] 同义）。
- 记 $\operatorname{lc}(f,x)$ 为首项系数，$\operatorname{dis}(f,x)$ 为判别式。

## 2. 可隔离条件：正则 TSA（§3，核心答案）

TSA 称为**正则的**，当且仅当：

- **(a)** $\operatorname{lc}(f_1,x_1) \neq 0$ 且 $\operatorname{dis}(f_1,x_1) \neq 0$
- **(b)** $\{f_1,\ldots,f_{i-1}\}$ 的**每个**零点都不是 $\operatorname{lc}(f_i,x_i)\cdot\operatorname{dis}(f_i,x_i)$ 的零点（i = 2..s）
- **(c)** $\{f_1,\ldots,f_s\}$ 的每个零点都不是任何 $g_j$、$h_k$ 的零点（不等式部分；纯方程系统/CAD 情形为空）

正则性在算法中的两个关键作用（§3 正文明确）：
1. $\operatorname{lc}(f_{i+1},x_{i+1})(x^*) \neq 0$ ⟹ 反复缩小 X 后必有 $\operatorname{lc}(\_f_{i+1})\cdot\operatorname{lc}(*f_{i+1}) > 0$（根界构造的前提，终止）
2. $f_{i+1}(x^*, x_{i+1}) = 0$ **无重根** ⟹ 缩小后 S₁ 与 S₂ 的根数相同且区间对应（夹层夹逼出精确隔离）

非正则 TSA 到正则 TSA 的分解由 [XY02] 给出（本文 §4 三步流程第一步之外的第二步）。

## 3. 界多项式（Def 3.1 + Prop 3.1，算法核心）

设 $q \in \mathbb{Z}[x_1,\ldots,x_{i+1}]$ 表为 $q = q_l x_{i+1}^l + \cdots + q_0$。对盒 $X = ([a_1,b_1],\ldots,[a_i,b_i])$，取各系数的区间估值 $Q_j$，定义下界/上界多项式

$$\_q = \textstyle\sum \underline{c}_j x_{i+1}^j,\qquad *q = \textstyle\sum \overline{d}_j x_{i+1}^j$$

**Prop 3.1（夹层）**：若 X 是 $\{f_1,\ldots,f_i\}$ 某零点 $x^*$ 的隔离盒，则对一切 $n \in \mathbb{N}$ 与 $x_{i+1} > 0$：

$$[\_f_{i+1}]^{(n)} \;\le\; f_{i+1}^{(n)}(x^*, x_{i+1}) \;\le\; [*f_{i+1}]^{(n)}$$

（即各阶导数都被两个**整系数**一元多项式夹住；系数区间单调性来自 Thm 2.1 区间估值单调。）

## 4. 算法 NREALZERO（§3）

输入：正则 TSA（式 (4) 形）。输出：正实解的隔离立方体列表。

- **Step 1**（i=0）：一元 $f_1$ 正根隔离（uspensky 类）。
- **Step 2**（0<i<s）：对每个 $X \in L_i$：
  计算 $\_f_{i+1}$、$*f_{i+1}$（系数取 X 的估值）；
  S₁ ← $\_f_{i+1}$ 正根隔离；S₂ ← $*f_{i+1}$ 正根隔离；S ← S₁∧S₂；
  $L_{i+1} \cup=\{X \times [c,d] : [c,d] \in S\}$。
- **Step 3**（i=s）：对每个 X 计算 $g_j$ 的区间估值 $G_j(X)$：$\operatorname{sign}<0$ 删去；$=0$ 反复缩 X 直到符号确定；$>0$ 保留。

**移位技巧**：先把根平移到正轴——取根界 $B > 0$（由 $\_f$、$*f$ 的正根界最大值给出），令 $f_{i+1}(x, x_{i+1}) := f_{i+1}(x, x_{i+1} - B)$。

**S = S₁∧S₂ 的三条件（Remark 3.1）**：
1. $m_1 = m_2$（两族根数相同）
2. $f_{i+1}(x^*,\cdot)$ 在每个 $[\alpha_k,\beta_k]$ 上单调
3. S 的相邻区间不交的

不满足则缩 X（有限次内必成立——正则性保证）。

## 5. 子算法 NSHR（§3，自底向上精化）

输入：立方体 X（含 $x^*$ = X 中唯一解）。输出：$X' \subset X$ 仍含 $x^*$。

- **Step 1**（j=0）：由 IVT 得 $[a'_1,b'_1] \subset [a_1,b_1]$ 含 $x_1^*$ 且宽度 ≤ 一半。
- **Step 2**（0<j<i）：对已缩小的 X' 重算 $\_f_{j+1}$、$*f_{j+1}$；分别由 IVT 在 $[a_{j+1},b_{j+1}]$ 内取含各自唯一零点的子区间（宽度 1/8），$[a'_{j+1},b'_{j+1}] = [\min(a,c),\max(b,d)]$（取并——保证真零点 $x^*_{j+1}$ 仍在内）。
- **Step 3**（j=i）：输出 X'。

正确性关键：界多项式随盒缩小而夹逼收紧（Thm 2.1 单调性），故 IVT 收缩保持唯一零点与单调性。

## 6. 与 CLPoly 实现的对应

| 论文 | CLPoly 代数数模块 |
|------|-------------------|
| 正则 TSA 条件 (a)(b) | 不变量 I1（正则）+ I3（点上无平方且 lc 不消去） |
| NREALZERO Step 2 | `realroot_over` 主循环（v2.1 路线） |
| 界多项式 _f/*f | 待实现（evaluate 系数 → 盒上估值 → ZZ 上下界多项式） |
| NSHR | `refine` 的自底向上协议（v2.1 对齐） |
| Step 3 不等式判定 | CAD 符号判定（sign_of 的区间规则同源） |
| [XY02] 正则化分解 | AN-3 开放项 1（extend_at_root 分裂选支）蓝本 |

## 7. 备注

- Thm 2.1：多项式的区间估值是单调区间估值（包含性）。——rational_interval.hh 与 polynomial_interval.hh 的数学依据。
- Remark 3.2：NSHR 的 1/8 是经验因子，理论可取 (0,1) 内任何有理数。
- 复杂度：论文未给渐近界；实现（Maple Nrealzero）在 6 个算例上优于 [XY02] 的 realzero。
