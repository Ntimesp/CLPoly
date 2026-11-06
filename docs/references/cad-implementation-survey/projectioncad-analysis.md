# ProjectionCAD 实现细节分析报告

> 历史阅读辅助：保留原整理日期和观点，尚未逐条校正，不作为当前接口、算法选择或正确性结论。OCR、译述和页码以原文为准；当前复核见 [CAD 文档入口](../../cad/README.md)。

> **来源**：ProjectionCAD (Maple) — Matthew England, David Wilson, Russell Bradford, James H. Davenport (University of Bath)
> **分析日期**：2026-04-22
> **用途**：为 CLPoly 完整 CAD 实现提供工程参考
> **配套文档**：[rc-cad-issac2009-analysis.md](projectioncad/CAD-via-Triangular-Decomposition-ISSAC2009-中文详解.md) — 分析 ProjectionCAD 调用的 RegularChains 黑箱（`TRDgenerate_stack`、`TRDisolate_real_zeros`）的算法理论

---

## 目录

> **阅读建议**：§1–§6 是对 ProjectionCAD.mpl 源码的分析（学习参考）；§7 是抽离出来的 CLPoly 设计观察与可借鉴经验。如果只想理解 ProjectionCAD 怎么做，读 §1–§6 即可。

- [1. 核心架构](#1-核心架构)
- [2. 投影阶段](#2-投影阶段)
- [3. 提升阶段（关键）](#3-提升阶段关键)
  - [3.1 主流程 `PCAD_ProjCADLift`](#31-主流程-pcad_projcadlift)
  - [3.2 样本点的 RootOf 编码](#32-样本点的-rootof-编码)
  - [3.3 Nullification 处理](#33-nullification-处理)
  - [3.4 Stack 生成 `PCAD_GenerateStack`](#34-stack-生成-pcad_generatestack)
  - [3.5 函数总表](#35-函数总表)
- [4. 代数数样本点与 cell 数据结构](#4-代数数样本点与-cell-数据结构)
  - [4.1 内部表示：`[regular_chain, isolating_cube]`](#41-内部表示regular_chain-isolating_cube)
  - [4.2 转换为 RootOf：`PCAD_SPtoRootOf`](#42-转换为-rootofpcad_sptorootof)
  - [4.3 完整 cell 数据结构](#43-完整-cell-数据结构)
  - [4.4 设计哲学：cell 的"身份"vs"描述"分离](#44-设计哲学cell-的身份vs描述分离)
  - [4.5 半代数表示的生成与转换](#45-半代数表示的生成与转换)
- [5. RegularChains 调用接口](#5-regularchains-调用接口)
- [6. 各论文算法速查](#6-各论文算法速查)
- [7. 对 CLPoly 的启示](#7-对-clpoly-的启示)
  - [7.1 可直接借鉴的设计](#71-可直接借鉴的设计)
  - [7.2 需要警惕的设计耦合](#72-需要警惕的设计耦合)
  - [7.3 数据结构层面的可能改进](#73-数据结构层面的可能改进)
  - [7.4 函数级映射（参考速查）](#74-函数级映射参考速查)
  - [7.5 整体路线选择](#75-整体路线选择)
- [附录：参考文献](#附录参考文献)

---

## 1. 核心架构

ProjectionCAD 是一个**纯 PL-CAD（Projection & Lifting）框架**的 Maple 实现，与 Maple 内置的 RegularChains CAD（三角分解路线）形成对比。

### 1.1 关键设计决策

> **投影阶段自己实现，提升阶段的代数数运算外包给 RegularChains Library。**

**动机**（Architecture 论文 §2, p.4）：
- 避免大量昂贵的代数数计算，利用三角分解的高效算法
- 自动使用 Maple 中最好的子算法（如改进的实根隔离）
- 输出格式与 RC-CAD 一致，可使用 piecewise 界面

### 1.2 整体流程

**论文**：Architecture 论文 **Algorithm 1: PL-CAD** (p.4)

```
Algorithm 1: PL-CAD
输入：变量序 x₁ ≺ ... ≺ xₙ，F（多项式序列或无量词公式序列）
输出：Rⁿ 的 CAD，对多项式符号不变（或对公式真值不变）；或 FAIL

// 投影阶段
1. 用适当的投影算子对 F 进行投影
2-3. for i = 1,...,n:
       Pᵢ ← 主变量为 xᵢ 的投影多项式集合

// 提升阶段
4. C₁ ← R¹ 的分解：P₁ 中一元多项式的实根隔离
5. for i = 2,...,n:
6.   for 每个 cell c ∈ Cᵢ₋₁:
7.     检查必要的 well-orientedness 条件
8.     if 输入非 well-oriented:
9.       if dim(c) = 0:
10.        L ← Pᵢ 中的多项式 ∪ 任何（非常数）最小 delineating 多项式
11.      else:
12.        return FAIL
13.    else:
14.      L := Pᵢ
15.    S_c ← GenerateStack(c, L)   // Algorithm 2
16.  Cᵢ ← ⋃_c S_c
18. return Cₙ
```

### 1.3 源码结构（ProjectionCAD.mpl）

| Section | 内容 | 关键过程 |
|---------|------|----------|
| 1 | 用户命令 | `CADFull`, `CADProjection`, `CADLifting`, `ECCAD`, `TTICAD` |
| 2 | 杂项工具 | 排序、内容/本原部分、无平方基、GCD、因式分解 |
| 3 | Collins 投影 | `PCAD_CCADProj`, PSC 计算, reductum set |
| 4 | McCallum 投影 | `PCAD_McCallumProj`, 判别式/系数/结式 |
| 5 | **提升阶段** | `PCAD_ProjCADLift`, `PCAD_GenerateStack`, `PCAD_MakeCoprimeOverRC`, `PCAD_MakeSquareFreeOverRC` |
| 6 | ECCAD/TTICAD | 约化投影算子、TTICAD 投影 |
| 7 | 公式化 | 问题形式化、变量序选择 |
| 8 | 启发式 | sotd, ndrr, 变量序启发式 |

---

## 2. 投影阶段

### 2.1 支持的投影算子

| 算子 | 实现 | 适用场景 | 论文位置 |
|------|------|----------|---------|
| **Collins** | `PCAD_CCADProj` | 通用，总是成功，多项式最多 | V1 §1.1, p.2 |
| **McCallum** | `PCAD_McCallumProj` | well-oriented 输入，多项式较少 | V1 §1.1, p.2 |
| **McCallum 约化 (Pf)** | `TTI_ECReducedProjectionOperator` | 含等式约束 (ECCAD) | V1 §2.1, p.3; V2 §2.1 |
| **TTICAD (PE)** | `TTI_TTIProjectionOperator` | 多公式真值表不变 | V2 §2.2, p.5 |

**注意**：ProjectionCAD V2.6 **未实现 Lazard 投影算子**（后续增量 CAD 工作中才加入）。

### 2.2 McCallum 投影算子实现

**理论定义**（V1 §1.1, p.2）：

```
P(A) := cont(A) ∪ coeff(B) ∪ disc(B) ∪ res(B)
```

其中 B 是 A 的 primitive part 的最细不可约基。

**代码实现**（`PCAD_McCallumProj`, ProjectionCAD.mpl:1490-1522）：

```maple
PCAD_McCallumProj(polys, var, lvars):
  1. 取本原部分 → 无平方基
  2. Pset1（单多项式）：对每个多项式计算
     - discriminant(Pol, var)
     - 系数集 PCAD_McCcoeffset(Pol, var, lvars)
  3. Pset2（多项式对）：对每对计算 resultant
  4. 合并内容集 + Pset1 + Pset2，去除常数倍
```

### 2.3 系数集优化：`PCAD_McCcoeffset`

**位置**：ProjectionCAD.mpl:1524-1553
**注意**：V1/V2/Architecture 论文中**均未描述此优化**，属实现层面的工程技巧。

**算法**（源码）：

```maple
PCAD_McCcoeffset(poly, var, lvars):
  deg ← degree(poly, var)
  cflist ← [coeff(poly,var,0), coeff(poly,var,1), ..., coeff(poly,var,deg)]
  crw ← cflist[1]  // 从最低次系数开始

  if crw 是常数: return []
  if lvars 只有一个变量: return [crw]

  for i from 2 to nops(cflist):
    tvar ← crw 中出现的变量
    res ← resultant(crw, cflist[i], tvar)
    if res = 0:
      continue           // cflist[i] 与 crw 有公因子，不独立，跳过
    elif res 是常数:
      return [cflist[1], ..., cflist[i]]  // 独立，截断返回
    else:
      crw ← res          // 更新 crw，继续
  return cflist
```

**"独立"的含义**：若 `resultant(crw, next_coeff, tvar)` 为**常数**，说明 `crw` 和 `next_coeff` **无公共零点**，`next_coeff` 引入了新的零点，需要保留。若结式为 0，说明 `next_coeff` 的零点已被 `crw` 覆盖，可省略。

**注**：V1/V2/Architecture 论文中**均未描述此优化**，属实现层面的工程技巧 —— 取所有非零系数当然可行，但会增加投影多项式数量。

---

## 3. 提升阶段（关键）

提升阶段是 ProjectionCAD 的核心。

> **导读**：[§3.1](#31-主流程-pcad_projcadlift) 主流程伪代码与调用链建立全局视图；[§3.2](#32-样本点的-rootof-编码) 样本点编码、[§3.3](#33-nullification-处理) nullification 处理、[§3.4](#34-stack-生成-pcad_generatestack) Stack 生成展开细节；[§3.5](#35-函数总表) 速查所有函数。

---

### 3.1 主流程 `PCAD_ProjCADLift`

**论文**：Architecture 论文 Algorithm 1 (p.4)
**源码**：[`PCAD_ProjCADLift`](projectioncad/ProjectionCAD.mpl#L1889) (ProjectionCAD.mpl:1889-2018)

```maple
PCAD_ProjCADLift(ProjPolys, vars, method, ...):
  // 1. 按变量分层组织投影多项式
  pset[1..dim] ← 按主变量分类

  // 2. 第 1 层：一元多项式实根隔离
  cad[1] ← RegularChains:-TRDgenerate_stack([], pset[1], R[1])

  // 3. 逐层提升
  for i from 2 to dim:
    for each cell[j] in cad[i-1]:
      alpha ← PCAD_SPtoRootOf(cell[j]的样本点)   // 样本点 → RootOf 替换列表

      tmppset ← {}
      for each poly in pset[i]:
        falpha ← subs(alpha, poly)               // 代数数处求值（Maple 内核）
        if is(falpha = 0) then                   // nullification 检测
          if PCAD_IsCellZeroDim(cellIndex) then
            DP ← PCAD_MinimalDelineatingPolynomial(poly, alpha, R[i])
            if DP::constant then skip
            else tmppset ← tmppset ∪ {DP}
          else:
            handle_failure(...)                  // warn / err / giveFAIL
        else:
          tmppset ← tmppset ∪ {poly}

      // 生成 stack
      stack ← PCAD_GenerateStack(cell[j], tmppset, R[i], output)
      cad[i] ← cad[i] ∪ stack
```

**调用链全景**（每个箭头展开见 §3.2 - §3.4）：

```
PCAD_ProjCADLift (1889)
├── 第 1 层：RegularChains:-TRDgenerate_stack([], pset[1], R[1])
└── 第 i 层 (i≥2)，每个 cell：
    ├── PCAD_SPtoRootOf (1801)                   ← §3.2
    │   └── 嵌套 RootOf 表达式（逐层 subs）
    ├── subs(alpha, poly)                         ← §3.3 Maple 内核求值
    ├── is(falpha = 0)                            ← §3.3 Maple 内核判零
    ├── if nullify:                               ← §3.3
    │   ├── PCAD_IsCellZeroDim (1583)             决定处理策略
    │   └── PCAD_MinimalDelineatingPolynomial (1615)
    │       └── diff(f, var) + gcd                求偏导 + GCD
    └── PCAD_GenerateStack (1843)                 ← §3.4
        ├── RPrceqs ← 偶数 index 维方程            section 约束
        ├── extra ← 奇数 index 维方程              sector 不约束
        ├── PCAD_MakeCoprimeOverRC (1644)
        │   ├── RegularChains:-Triangularize
        │   └── PCAD_MakeMonic (1056)
        ├── PCAD_MakeSquareFreeOverRC (1762)
        │   ├── ChainTools:-SquarefreeFactorization
        │   └── PCAD_WhichRCInSplitHasSolutionAtSP (1710)
        │       ├── RegularChains:-TRDisolate_real_zeros
        │       └── PCAD_DoesIntervalIntersect (1231)
        └── RegularChains:-TRDgenerate_stack
            └── 内部：求值 + 实根隔离 + 构建新 cell
```

---

### 3.2 样本点的 RootOf 编码

> 样本点的存储格式 `(rc, cube)` 完整描述见 [§4.1](#41-内部表示regular_chain-isolating_cube)。本节聚焦提升阶段如何使用它。

**源码**：[`PCAD_SPtoRootOf`](projectioncad/ProjectionCAD.mpl#L1801) (ProjectionCAD.mpl:1801-1825)

`(rc, cube)` 在使用前先转换为 RootOf 替换列表 `alpha`：

```maple
for i = 1 to dim:
  if cube[i].L = cube[i].R:                       // 有理数坐标
    pt += [vars[i] = cube[i].L]
  else:                                           // 代数数坐标
    pt += [vars[i] = RootOf(subs(pt, eqs[i]), L..R)]
```

**关键机制**：
- 第 i 维的 `RootOf` 通过 `subs(pt, eqs[i])` **嵌入了前 i-1 维的 RootOf** —— Lazard 风格的逐层代入
- `alpha` 不是数值，而是 Maple 符号引擎可处理的"懒符号表示"
- 所有后续求值（`subs(alpha, poly)`）依赖 Maple 内核的 RootOf 化简能力

---

### 3.3 Nullification 处理

#### 3.3.1 检测：`subs` + `is(=0)`

源码位置：[PCAD_ProjCADLift line 1933-1934](projectioncad/ProjectionCAD.mpl#L1933)

```maple
falpha := subs(alpha, op(k, pset[i])):           // 多项式在代数数处求值
falpha := is(falpha = 0):                        // RootOf 表达式判零
```

两步全部委托给 Maple 内核 —— `subs` 进行 RootOf 替换，`is(=0)` 调用 RootOf 化简引擎。无 Maple 后端时，需用**三角列商环约化 + 系数判零**实现等价语义。

#### 3.3.2 维度分支：`PCAD_IsCellZeroDim`

**源码**：[`PCAD_IsCellZeroDim`](projectioncad/ProjectionCAD.mpl#L1583) (1583)、[`PCAD_IsCellFullDim`](projectioncad/ProjectionCAD.mpl#L1599) (1599)

通过检查 cellIndex 整数列表的奇偶性判定 cell 维度：

| Index 模式 | Cell 类型 | 维度 |
|-----------|----------|------|
| 全为偶数 | 零维 (point) | 0 |
| 全为奇数 | 全维 (open region) | n |
| 混合 | 部分维度 | 0 < d < n |

**奇偶约定**：偶数 = section（在某根上），奇数 = sector（开区间）。

#### 3.3.3 替代多项式：`PCAD_MinimalDelineatingPolynomial`

**源码**：[`PCAD_MinimalDelineatingPolynomial`](projectioncad/ProjectionCAD.mpl#L1615) (1615-1640)

零维 cell 上 nullify 时，用 **delineating polynomial** 替代原多项式继续提升：

```maple
1. 对 f 计算 vars 中变量的所有 k 阶偏导数（k 从 1 递增）
2. 找到最小的 k，使得存在某 k 阶偏导数在 alpha 处不为 0
3. 取所有在 alpha 处为零的偏导数的 GCD
4. 返回该 GCD 作为界定多项式 d
```

**处理策略汇总**：

| 场景 | 处理方式 |
|------|----------|
| 零维 cell + McCallum | 用 `MinimalDelineatingPolynomial` 替代（结果为常数则丢弃） |
| 正维 cell + McCallum | 报错 / 返回 FAIL / 丢弃（视 `failure` 选项） |
| Collins 投影 | 直接丢弃（Collins 不受 nullification 影响） |
| 最终一层 + SI 模式 | 直接丢弃（只需符号不变） |

参考：McCallum 1998, Brown 2005 技术报告。

---

### 3.4 Stack 生成 `PCAD_GenerateStack`

**论文**：Architecture 论文 **Algorithm 2: GenerateStack** (p.5-6)
**源码**：[`PCAD_GenerateStack`](projectioncad/ProjectionCAD.mpl#L1843) (ProjectionCAD.mpl:1843-1885)

#### 3.4.1 主算法

```
Algorithm 2: GenerateStack
输入：Rᵏ 的 cell c，投影多项式集合 P ⊂ R[x₁,...,xₖ₊₁]（无平方基的一部分）
输出：Rᵏ⁺¹ 的 cell 集合 S，构成 c 上的 stack

1. I, sp ← c 的 index 和 sample point
2. rc, bb ← 编码 sp 的 regular chain 和 isolating cube
3. E ← 定义 rc 的 k 个多项式集合（按主变量递增排序）
4. Ê := {}
5-7. for i = 1,...,k:
       if I 的第 i 个整数是偶数:                  ← 仅 section 维方程进入约束
         将 E 的第 i 个多项式加入 Ê

8. if Ê ≠ {}:
9.   rĉ ← Ê 构成的 regular chain
10.  P := MakeCoprime(P, rĉ, c)                  ← Algorithm 3 (§3.4.2)
11.  P := MakeSquareFree(P, rĉ, c)               ← Algorithm 4 (§3.4.3)

12-13. S := RegularChains:-TRDgenerate_stack(c, P̂, k+1)
14. return S
```

**两组方程的拆分**（line 1860-1866）：
- `RPrceqs` ← cellIndex 偶数维（section）的方程 → 构成 `RPrc`，用于 MakeCoprime/MakeSquareFree
- `extra` ← cellIndex 奇数维（sector）的方程 → 不约束，但在多分支消歧时用作过滤

#### 3.4.2 `MakeCoprime`（Algorithm 3）

**源码**：[`PCAD_MakeCoprimeOverRC`](projectioncad/ProjectionCAD.mpl#L1644) (1644-1706)

```
输入：多项式集合 P，regular chain rĉ，cell c
输出：在 rĉ 上互素的多项式集合 P̂

1. P̂ = []
2. for p ∈ P:
3.   T := RegularChains:-Triangularize([p], P̂, rĉ)
4.   for T 的每个 component C:
5.     if mvar(C) ≠ mvar(p): continue
6-9.   if C 有与 c 的样本点兼容的零点:
         提取 C 中主变量为 p 的多项式 p̂
         P̂ ← P̂ ∪ {p̂}
10. return P̂
```

**实现细节**：使用 [`PCAD_MakeMonic`](projectioncad/ProjectionCAD.mpl#L1056) (1056) 归一化，最后 [`PCAD_MakeInteger`](projectioncad/ProjectionCAD.mpl#L1091) (1091) 整数化。

#### 3.4.3 `MakeSquareFree`（Algorithm 4）

**源码**：[`PCAD_MakeSquareFreeOverRC`](projectioncad/ProjectionCAD.mpl#L1762) (1762-1797)

```
输入：多项式集合 P，regular chain rĉ，cell c（含 extra + SPcube）
输出：无平方的多项式集合 P̂

1. P̂ = []
2. for p ∈ P:
3.   if p 的主变量 ≠ rĉ 主变量: P̂ += p; continue
4.   SFD := ChainTools:-SquarefreeFactorization(p, mvar, rĉ, R, method='src')
5.   if SFD 单分支:
6.     p̂ := ∏ 各无平方因子
7.   else:                                       ← 多分支消歧
8.     idx := PCAD_WhichRCInSplitHasSolutionAtSP(SFD, SPcube, R)
9.     p̂ := SFD[idx] 中各无平方因子的乘积
10.  P̂ ← P̂ ∪ {p̂}
11. return P̂
```

#### 3.4.4 多分支消歧 `PCAD_WhichRCInSplitHasSolutionAtSP`

**源码**：[`PCAD_WhichRCInSplitHasSolutionAtSP`](projectioncad/ProjectionCAD.mpl#L1710) (1710-1758)

当 `SquarefreeFactorization` 在 rc 上分裂出多条 RC 分支时，必须挑出与当前 cell 样本点兼容的那条：

```maple
1. lowerRCeqs ← 所有 RC 的 lower 部分（应当相同）
2. MainPolys  ← 所有 RC 的最高维方程
3. out ← RegularChains:-TRDisolate_real_zeros([lowerRC, lowerCube], MainPolys, R)
4. tRoots ← out 中各根的隔离区间
5. for i in 1..nops(tRoots):
     if PCAD_DoesIntervalIntersect(tRoots[i], Cube[-1]):
       return i                                  ← 与当前 cube 相交的分支即为答案
```

**关键依赖**：
- `RegularChains:-TRDisolate_real_zeros` —— 代数数系数实根隔离（无 RegularChains 后端时是最大瓶颈）
- [`PCAD_DoesIntervalIntersect`](projectioncad/ProjectionCAD.mpl#L1231) (1231) —— 区间相交判定

#### 3.4.5 核心思想

在将多项式代入样本点之前，先用三角分解确保多项式在样本点所在的簇上是**互素且无平方**的。这是 `RegularChains:-TRDgenerate_stack` 的输入前置条件。

---

### 3.5 函数总表

ProjectionCAD.mpl 中所有与代数数处理相关的函数（按角色分组）：

| 角色 | 函数 | 行号 | 说明 |
|------|------|------|------|
| **样本点** | [`PCAD_SPtoRootOf`](projectioncad/ProjectionCAD.mpl#L1801) | 1801 | `(rc, cube)` → RootOf 替换列表 |
| | [`PCAD_SortEqsRC`](projectioncad/ProjectionCAD.mpl#L1030) | 1030 | 三角列方程按主变量升序排序 |
| | [`PCAD_SortCadCell`](projectioncad/ProjectionCAD.mpl#L1829) | 1829 | CAD cell 按 index 排序 |
| **Cell 维度** | [`PCAD_IsCellZeroDim`](projectioncad/ProjectionCAD.mpl#L1583) | 1583 | 全偶 ⇒ 零维 |
| | [`PCAD_IsCellFullDim`](projectioncad/ProjectionCAD.mpl#L1599) | 1599 | 全奇 ⇒ 全维 |
| **Nullification** | `subs(alpha, poly)` | [1933](projectioncad/ProjectionCAD.mpl#L1933) | 多项式代入代数数（Maple 内核） |
| | `is(falpha = 0)` | [1934](projectioncad/ProjectionCAD.mpl#L1934) | RootOf 判零（Maple 内核） |
| | [`PCAD_MinimalDelineatingPolynomial`](projectioncad/ProjectionCAD.mpl#L1615) | 1615 | 零维 cell 替代多项式 |
| **三角列预处理** | [`PCAD_MakeCoprimeOverRC`](projectioncad/ProjectionCAD.mpl#L1644) | 1644 | Algorithm 3 |
| | [`PCAD_MakeSquareFreeOverRC`](projectioncad/ProjectionCAD.mpl#L1762) | 1762 | Algorithm 4 |
| | [`PCAD_WhichRCInSplitHasSolutionAtSP`](projectioncad/ProjectionCAD.mpl#L1710) | 1710 | 多分支消歧 |
| | [`PCAD_MakeMonic`](projectioncad/ProjectionCAD.mpl#L1056) | 1056 | 主变量首项归一 |
| | [`PCAD_MakeInteger`](projectioncad/ProjectionCAD.mpl#L1091) | 1091 | 去分母整数化 |
| **区间运算** | [`PCAD_DoesIntervalIntersect`](projectioncad/ProjectionCAD.mpl#L1231) | 1231 | 区间相交判定 |
| **Stack 入口** | [`PCAD_GenerateStack`](projectioncad/ProjectionCAD.mpl#L1843) | 1843 | 包装 `TRDgenerate_stack` |
| **输出转换** | [`PCAD_LWRCADtoPWCAD`](projectioncad/ProjectionCAD.mpl#L2022) | 2022 | listwithrep → piecewise |
| **RC 后端**（黑箱） | `RegularChains:-TRDgenerate_stack` | — | **核心**：求值 + 实根隔离 + 构建新 cell |
| | `RegularChains:-TRDisolate_real_zeros` | — | **核心**：代数数系数实根隔离 |
| | `RegularChains:-Triangularize` | — | 模 rc 三角化（MakeCoprime 用） |
| | `ChainTools:-SquarefreeFactorization` | — | 模 rc 无平方分解（MakeSquareFree 用） |
| | `ChainTools:-Chain / Empty / IsIncluded` | — | 三角链构造与包含判定 |
| | `Equations / MainVariable / Initial` | — | 三角链查询接口 |

---

## 4. 代数数样本点与 cell 数据结构

### 4.1 内部表示：`[regular_chain, isolating_cube]`

ProjectionCAD 的样本点**不直接使用代数数**，而是用 **regular chain + isolating cube** 的二元组：

```
样本点 = [ regular_chain, isolating_cube ]
```

- **regular_chain**：定义样本点的多项式方程链（按主变量升序排列）
- **isolating_cube**：区间列表 `[[L₁,R₁], [L₂,R₂], ..., [Lₙ,Rₙ]]`
  - `Lᵢ = Rᵢ`：第 i 个坐标是有理数
  - `Lᵢ < Rᵢ`：第 i 个坐标是代数数，根在 `(Lᵢ, Rᵢ)` 内

### 4.2 转换为 RootOf：`PCAD_SPtoRootOf`

**源码**：[`PCAD_SPtoRootOf`](projectioncad/ProjectionCAD.mpl#L1801) (ProjectionCAD.mpl:1801-1825)

```maple
PCAD_SPtoRootOf(rc, cube, R):
  eqs ← Equations(rc, R)  // 获取方程列表
  pt ← []
  for i from 1 to dim:
    if op(i,cube)[1] = op(i,cube)[2] then
      // 有理数样本点
      pt ← [op(pt), vars[i] = op(i,cube)[1]]
    else
      // 代数数样本点：RootOf(poly, L..R)
      pt ← [op(pt), vars[i] = RootOf(subs(pt, eqs[i]), op(i,cube)[1]..op(i,cube)[2])]
  return pt
```

**关键观察**：
- 代数数不是"计算"出来的，而是"表示"出来的
- 求值时通过 `subs(RootOf(...), poly)` 让 Maple 的符号引擎处理
- 不需要显式的代数数运算（加减乘除），全部通过符号替换完成

---

### 4.3 完整 cell 数据结构

[`CADGenerateStack`](projectioncad/ProjectionCAD.mpl#L303) line 338-340 给出严格类型断言：

```maple
cellIndex := cell[1]:
cellSP    := cell[-1]:
if not cellIndex::'list(integer)' or nops(cellSP) <> 2 then
    error(...)
```

源码中所有 `cell[*]` 访问只涉及 `cell[1]` 和 `cell[-1]`，cell 长度严格为 2：

```
cell = [ cellIndex,  [SPrc, SPcube] ]
        ↑              ↑      ↑
        list(int)      RC     list(interval)
```

#### 字段类型

```
cellIndex : list(integer)
            奇数 = sector（开区间）
            偶数 = section（在某根上）
            列表长度 = 当前 cell 维度

SPrc      : RegularChains 的 regular_chain table
            通过 RegularChains:-Equations(SPrc, R) 取方程列表
            方程数 = nops(R['variables']) - 1（不含当前变量）

SPcube    : list of intervals  [[L₁,R₁], ..., [Lₙ,Rₙ]]
            Lᵢ = Rᵢ ⇒ 第 i 维有理数样本点
            Lᵢ < Rᵢ ⇒ 第 i 维代数数，根在 (Lᵢ, Rᵢ) 内
```

#### 长度不变量

| 提升到第 k 层 | cellIndex 长度 | SPrc 方程数 | SPcube 长度 |
|---------------|----------------|-------------|-------------|
| k = 1 (R¹)    | 1              | 0（空 RC）  | 1           |
| k = i         | i              | i - 1       | i           |
| k = n (Rⁿ)    | n              | n - 1       | n           |

注意 SPrc 比 cellIndex 短一个 —— 当前层的位置由 cube 的最后一个区间表示，**不在 RC 里**。

#### 'listwithrep' 输出格式的扩展字段

[`PCAD_LWRCADtoPWCAD`](projectioncad/ProjectionCAD.mpl#L2022) 揭示 listwithrep 格式有 3 元素：

```
cell_lwr = [ index_list, repr_list, leaf ]
```

- `repr_list[i]`：第 i 维变量的代数表示（RootOf 或有理数），用于 piecewise 输出
- `leaf`：per-cell 标签

但这是**输出格式**，对算法逻辑没有作用，核心数据结构仍是 2 元素。

---

### 4.4 设计哲学：cell 的"身份"vs"描述"分离

ProjectionCAD 的 cell 数据**只存样本点**，**不存半代数描述**。这是一个深刻的设计选择。

#### 4.4.1 cell 中实际存储与缺失的信息

| 信息 | 是否存储 | 来源 |
|------|----------|------|
| cell 的"身份证"（样本点位置） | ✅ 显式存储 | `[SPrc, SPcube]` |
| 结构骨架（哪一层第几个 cell） | ✅ 显式存储 | `cellIndex` 奇偶序列 |
| **section 维的代数方程** | ✅ 隐含在 SPrc 中 | `SPrc` 的对应方程 |
| **sector 维的边界根**（左右两个 sections） | ❌ **不存储** | 需查父 cell 的兄弟 cell 重构 |
| cell 的不等式系统（半代数描述） | ❌ **不存储** | 需要时由 listwithrep + 后处理重构 |

**关键观察**：cube 在 sector 维的 `[L, R]` 是**样本点的位置精度**，不是 cell 的左右边界！cell 的左右边界是父层投影多项式在样本点处求值得到的两个相邻根 —— 这些根的隔离区间没有存在 cell 里。

#### 4.4.2 为什么 ProjectionCAD 这样设计

**原因 1：QE 用例不需要 cell 描述**

PL-CAD 的初衷是量词消去 / 真值表分析：
- "是否存在 $(x_1,\ldots,x_n)$ 满足公式 $F$？"
- 只需检查每个 cell 的样本点上 $F$ 的真值（McCallum 投影保证 cell 上符号不变）
- 不需要写出 cell 的不等式系统

**原因 2：cell 数量爆炸时内存友好**

CAD 的 cell 数量在 $n$ 维下可能达到 $O(d^{2^n})$。仅存样本点（$O(n)$ 个有理数 + 一条共享 RC）远比存完整描述（$O(n^2)$ 个边界多项式）紧凑。

**原因 3：边界根可被共享**

同一父 cell 上方的 stack 中，相邻 cells 共享分界根：sector_k 的右边界 = section_{k+1} 的位置 = sector_{k+2} 的左边界。如果在每个 cell 里都存边界，会重复 3 次。ProjectionCAD 通过"按需重构"避免冗余。

**原因 4：半代数描述按需重构（listwithrep + LWRCADtoPWCAD）**

需要 cell 的具体描述时（如 piecewise 输出），运行后处理：
- `listwithrep` 格式补充 `repr_list`（每维的 RootOf 表示）
- `PCAD_LWRCADtoPWCAD` 把 cellIndex 翻译成 "x_1 < α_1 / x_1 = α_1 / α_1 < x_1 < α_2 / ..." 的 piecewise 表达式

#### 4.4.3 半代数边界的语义身份 vs 计算状态（局限观察）

ProjectionCAD 用 `RootOf(p, L..R)` 表示代数数样本点（[`PCAD_SPtoRootOf`](projectioncad/ProjectionCAD.mpl#L1801)）。这个表示**适合做样本点**，但若想把它**同时用作半代数边界**，会暴露设计上的耦合：

| 维度 | 内容 | 是否可变 |
|------|------|----------|
| **语义身份** | (父 cell, 投影多项式 $p$, 根索引 $k$) | **不可变** |
| **计算状态** | 隔离区间 `[L, R]`、精度 | 可变（精化） |
| **代数环境** | 三角列 $T$（父 cell 的样本点环境） | 由父 cell 决定 |

`RootOf(p, L..R)` 把这三者**揉在一个符号表达式里**：
- L..R 会随实根隔离精化而收窄
- p 的系数依赖**低维样本点**，而样本点本身也可能精化
- 同一根在不同时刻、不同精度下，`RootOf(p, L..R)` 表达**不相等**，但**指代相同**
- 兄弟 cell 共享同一边界根时，若各自存了不同精度的快照，无法直接判等

**ProjectionCAD 之所以能容忍这种耦合**：Maple 内核做了符号化简 + memoization，能在符号层面把不同精度的 `RootOf` 视为相等。Maple 的 `is(RootOf(p, L1..R1) = RootOf(p, L2..R2))` 会检查两区间是否同时包含某根，并答 `true`。

**根索引为什么是良定义的**：McCallum 投影保证投影多项式在每个 cell 上是 **delineable**（实根作为参数的连续函数，不交叉）。在父 cell 上方，可以把 $p$ 的实根按值升序编号 $r_1 < r_2 < \cdots < r_m$，**编号在整个父 cell 上稳定**。所以 "(父 cell, $p$, $k$)" 已经唯一确定一个 section —— 隔离区间只是计算时的精度状态，不是身份的一部分。

ProjectionCAD 通过"按需 RootOf 重构"+ Maple 符号引擎把这个细节藏起来；外部库（如 CLPoly）若要参考此设计，需要清楚意识到：**身份与状态在 `RootOf(p, L..R)` 中是耦合的**，必要时应显式分离。

---

### 4.5 半代数表示的生成与转换

ProjectionCAD 自身**不生成**半代数表示，而是依赖 RegularChains 的 `TRDgenerate_stack_general` 在 stack 生成时同步产出，然后由 PCAD 端做格式转换与组装。

#### 4.5.1 四种输出格式

[`PCAD_ProjCADLift`](projectioncad/ProjectionCAD.mpl#L1889) line 2009-2017 给出输出格式分派：

```maple
if out='list' or out='listwithrep' then
    return( cad[dim] ):
elif out='rootof' then
    tmpcad := map(X -> X[2], cad[dim]):                              # 提取 repr_list
    tmpcad := map(Y -> map(X -> `if`(op(0,X)=And, op(X), X), Y), tmpcad):  # 展平 And
    return(tmpcad):
elif out='piecewise' then
    return( PCAD_LWRCADtoPWCAD( cad[dim] ) ):                       # 转 piecewise
fi:
```

四种格式对比：

| 格式 | cell 结构 | 用途 |
|------|-----------|------|
| `list` | `[cellIndex, [SPrc, SPcube]]`（长度 2） | 仅身份，QE / 真值分析够用 |
| `listwithrep` | `[cellIndex, repr_list, [SPrc, SPcube]]`（长度 3） | 身份 + 半代数描述（**核心**） |
| `rootof` | `repr_list`（仅描述，含 `And` 展平） | 仅半代数描述，无样本点 |
| `piecewise` | Maple `piecewise(c₁, v₁, c₂, v₂, ...)` 表达式 | 嵌套条件树，可直接喂给 Maple 求值 |

#### 4.5.2 `repr_list` 的格式（推测）

源码没有显式文档，但从 [PCAD_LWRCADtoPWCAD](projectioncad/ProjectionCAD.mpl#L2022) 与 `rootof` 分支的处理推断：

```
repr_list = [ cond_1, cond_2, ..., cond_n ]   # 长度 = dim，每维一个条件
```

每维 `cond_i` 是一个 Maple 表达式，可能形式：

| cellIndex[i] 类型 | cond_i 形式 | 含义 |
|-------------------|-------------|------|
| 偶数（section） | `x_i = RootOf(p, L..R)` 或 `x_i = q`（有理数） | 等式约束 |
| 奇数（首 sector） | `x_i < RootOf(p, L..R)` 或 `x_i < q` | 单边开 |
| 奇数（尾 sector） | `x_i > RootOf(p, L..R)` | 单边开 |
| 奇数（中间 sector） | `And(x_i > RootOf(p, ...), x_i < RootOf(q, ...))` | 双边开区间 |

`rootof` 输出时（line 2013）`map(X -> if op(0,X)=And then op(X) else X)` 把 `And(c1, c2)` 展平成 `c1, c2` 两个独立条件，得到一个非合取的扁平条件列表。

`piecewise` 输出时则保留嵌套 `And` —— 因为 Maple 的 `piecewise` 子句需要单个条件表达式。

#### 4.5.3 listwithrep → piecewise 转换：`PCAD_LWRCADtoPWCAD`

**源码**：[`PCAD_LWRCADtoPWCAD`](projectioncad/ProjectionCAD.mpl#L2022) (ProjectionCAD.mpl:2022-2053)

递归把 listwithrep CAD 转成嵌套 piecewise 表达式：

```maple
PCAD_LWRCADtoPWCAD(LCAD):                       # LCAD: list of [index, repr, leaf]
  dim := nops(LCAD[1][1])                       # 当前剩余维度

  if dim = 0:                                   # 终止条件
    return LCAD[1][3]                           # 返回 leaf

  NumRoots := max(map(X -> X[1,1], LCAD))       # 第一维的 index 最大值
  Tree := []
  for i from 1 to NumRoots:
    SubCAD := select(X -> X[1][1] = i, LCAD)    # 同 index 的 cells 分组
    Rep := unique({X[2,1] for X in SubCAD})     # 同组共享 repr_list[1]
    if Rep 不唯一: error                         # 一致性检查

    # 剥掉第一维，递归构建子树
    SubCAD' := map(X -> [X[1][2..dim], X[2][2..dim], X[3]], SubCAD)
    Tree := Tree ∪ [Rep, PCAD_LWRCADtoPWCAD(SubCAD')]

  return piecewise(op(Tree))                    # piecewise(c1, v1, c2, v2, ...)
```

**关键设计**：
1. **按第一维 cellIndex 分组**：所有第一维 index 相同的 cells 共享同一个 `repr_list[1]` —— 一致性检查保证此不变量
2. **递归构造嵌套 piecewise**：每层一个 piecewise，最终深度 = dim
3. **共享分支条件**：同组 cells 共享 representation，避免冗余

**输出形态示例**（dim=2）：

```maple
piecewise(
    x_1 < RootOf(p_1, ...),                              # 第 1 维条件 (sector)
    piecewise(                                            # 进入子分支
        x_2 < RootOf(q_1, ...),  Leaf_{1,1},
        x_2 = RootOf(q_1, ...),  Leaf_{1,2},
        And(x_2 > RootOf(q_1, ...), x_2 < RootOf(q_2, ...)),  Leaf_{1,3},
        ...
    ),
    x_1 = RootOf(p_1, ...),                              # 第 1 维条件 (section)
    piecewise(...),                                       # 该 section 上方的子分支
    ...
)
```

#### 4.5.4 `NumCellsInPiecewiseCAD`：递归数 cell

**源码**：[`NumCellsInPiecewiseCAD`](projectioncad/ProjectionCAD.mpl#L268) (ProjectionCAD.mpl:268-285)

```maple
NumCellsInPiecewiseCAD(pwcad):
  size := nops(pwcad) / 2                        # piecewise 项数
  num := 0
  for i from 1 to size:
    nest := op(2*i, pwcad)                       # 取第 i 项的"值"部分
    if nest::piecewise then
      num += NumCellsInPiecewiseCAD(nest)        # 递归
    else
      num += 1                                   # leaf 计数
    fi
  return num
```

#### 4.5.5 半代数表示相关函数清单

| 函数 | 行号 | 功能 |
|------|------|------|
| [`PCAD_ProjCADLift` 的 'rootof' 分支](projectioncad/ProjectionCAD.mpl#L2011) | 2011-2014 | 提取 `repr_list`，展平 `And` |
| [`PCAD_LWRCADtoPWCAD`](projectioncad/ProjectionCAD.mpl#L2022) | 2022 | listwithrep → piecewise 递归转换 |
| [`NumCellsInPiecewiseCAD`](projectioncad/ProjectionCAD.mpl#L268) | 268 | 递归数 piecewise CAD 中的 cell |
| [`NumCellsInCAD`](projectioncad/ProjectionCAD.mpl#L289) | 289 | 自动识别格式分派 cell 计数 |
| [`PCAD_SPtoRootOf`](projectioncad/ProjectionCAD.mpl#L1801) | 1801 | 样本点 → RootOf 替换列表（不是 cell 描述，是样本点描述） |
| `RegularChains:-TRDgenerate_stack_general` | — | **核心黑箱**：实际生成 `repr_list` 的引擎 |

**关键事实**：PCAD 端只做"格式转换"，不做"描述生成"。所有半代数条件（`x = RootOf(...)`, `x < RootOf(...)`, `And(...)`）都来自 RegularChains 的内部计算 —— 它在 stack 生成时同步产生样本点和描述。


## 5. RegularChains 调用接口

ProjectionCAD 调用的 RegularChains 命令（完整列表）：

| 命令 | 用途 |
|------|------|
| `PolynomialRing(vars)` | 创建多项式环 |
| `MainVariable(poly, R)` | 获取主变量 |
| `Initial(poly, R)` | 获取首项系数 |
| `Equations(rc, R)` | 获取正则链的方程 |
| `Triangularize(polys, rc, R)` | 三角分解 |
| `ChainTools:-Empty(R)` | 空正则链 |
| `ChainTools:-Chain(eqs, rc, R)` | 构建正则链 |
| `ChainTools:-SquarefreeFactorization(poly, var, rc, R)` | 模 rc 的无平方分解 |
| `ChainTools:-IsIncluded(rc1, rc2, R)` | 包含关系检查 |
| `TRDgenerate_stack(cell, polys, R)` | **生成 stack** |
| `TRDgenerate_stack_general(...)` | 生成 stack（高级输出，含 representation） |
| `TRDisolate_real_zeros([rc,cube], polys, R)` | **实根隔离**（代数数系数） |

### 5.1 `TRDgenerate_stack` 的功能推测

根据 ProjectionCAD 的调用方式（Architecture 论文 §2, p.5-6）：

```
输入：
  - cell_info: [cell, index] 或 []（顶层）
  - polys: 主变量为 xₖ 的多项式集合（已互素、无平方）
  - R: 多项式环
输出：
  - 在 cell 上的 stack：一组新 cell（sections + sectors）
```

内部可能的工作流程：
1. 提取 cell 的样本点（regular chain + cube）
2. 将 polys 中每个多项式在样本点处求值（代入 lower 变量）
3. 得到一元多项式（系数可能含 RootOf）
4. 用实根隔离找到所有实根
5. 生成 sections（根处）和 sectors（区间）
6. 每个新 cell 的样本点 = 父 cell 样本点 + 新坐标

**论文引用**：Architecture 论文 p.5 提到 `RegularChains:-GenerateStack` 的算法描述在 Chen et al. ISSAC 2009 的 Section 5.2。**详见 rc-cad-issac2009-analysis.md §7（可选本地附件：`rc-cad-issac2009-analysis.md`）**。

### 5.2 `TRDisolate_real_zeros` 的功能推测

```
输入：
  - [lowerRC, lowerCube]: 定义 lower 维度样本点的 regular chain + cube
  - polys: 主变量为 xₖ 的多项式集合
  - R: 多项式环
输出：
  - 隔离区间和索引
```

这是**代数数系数多项式的实根隔离**，是 RegularChains CAD 的核心算法。

**论文引用**：Architecture 论文 p.6 提到使用 `RealRootIsolate` 命令，参考 Boulier et al. ASCM 2009。**详见 rc-cad-issac2009-analysis.md §6（可选本地附件：`rc-cad-issac2009-analysis.md`）**。

---

## 6. 各论文算法速查

### 6.1 Architecture 论文 (arXiv:1405.6090, ICMS 2014)

| 算法 | 页码 | 内容 | 关键调用 |
|------|------|------|----------|
| **Algorithm 1: PL-CAD** | p.4 | 完整 PL-CAD 框架 | Algorithm 2 |
| **Algorithm 2: GenerateStack** | p.5-6 | Stack 生成 | Algorithm 3, 4, `RegularChains:-GenerateStack` |
| **Algorithm 3: MakeCoprime** | p.6 | 模 rc 互素化 | `Triangularize` |
| **Algorithm 4: MakeSquareFree** | p.6 | 模 rc 无平方分解 | `SquarefreeFactorization` |

### 6.2 V1 论文 (arXiv:1306.3062, 2013)

| 算法 | 页码 | 内容 |
|------|------|------|
| **Algorithm 1: ECCAD** | p.3 | 等式约束 CAD |
| **Algorithm 2: LiftingSet** | p.3 | 确定最终提升的多项式集合 |
| CADFull | 引用 [15] Algorithm 3 | 完整 CAD |
| CADGenerateStack | 引用 [15] Algorithm 4 | Stack 生成 |

### 6.3 V2 论文 (arXiv:1401.0645, 2014)

| 算法 | 页码 | 内容 |
|------|------|------|
| **Algorithm 1: TTICAD** | p.7 | 真值表不变 CAD |
| **Lifting 改进** | §3, p.8 | 更精细的 well-orientedness 检查、更小的提升集 |

### 6.4 V3 论文 (arXiv:1503.06599, 2015)

| 算法 | 页码 | 内容 |
|------|------|------|
| **Algorithm 1: LCAD** | p.4 | ℓ-layered sub-CAD |
| **Algorithm 2: VCAD** | p.5 | Variety sub-CAD |

---

## 7. 对 CLPoly 的启示

> 本节是 ProjectionCAD 分析的"读后感"，聚焦于**可借鉴的设计经验**与**需要警惕的设计耦合**。CLPoly 是否复现 Full CAD、复现到何种程度，是独立的工程决策；本节不假设此目标，仅提取 ProjectionCAD 这套实现给我们的参考价值。

### 7.1 可直接借鉴的设计

| 设计 | ProjectionCAD 做法 | 借鉴价值 |
|------|--------------------|---------|
| 样本点表示 | `(三角列 RC, 隔离 cube)` 二元组 | 避免显式代数数运算；与 CLPoly `charset.hh` 基础设施天然契合 |
| cell index | 整数列表，奇=sector / 偶=section | 紧凑、可序列化、便于按层分组；CLPoly `cad_node` 可直接采用 |
| 父样本点共享 | SPrc 嵌入子 cell（即父 cell 的样本点直接传递） | 避免重复存储；CLPoly 可改进为引用计数共享池，进一步去冗余 |
| 投影 / 提升解耦 | 投影自己做，提升委托代数数后端 | CLPoly 已有 OpenCAD 的投影实现，与此架构对齐 |
| 多分支消歧用 cube | `WhichRCInSplitHasSolutionAtSP` 用区间相交挑分支 | "样本点是身份，cube 是消歧"的设计可直接照搬 |
| Cell parity 约定 | 偶 ⇒ section（约束方程进入 RPrc）；奇 ⇒ sector（不约束） | 算法核心不变量，与 RC-CAD ISSAC 2009 §5.2 的 GenerateStack 一致 |

### 7.2 需要警惕的设计耦合

ProjectionCAD 之所以能"按需重构 + 不存边界"，是因为它依赖两个 Maple 内核能力：
- **RootOf 符号引擎**：能比较不同精度的 `RootOf(p, L1..R1)` 与 `RootOf(p, L2..R2)`、做化简、做 `is(... = 0)`
- **memoization**：避免重复昂贵计算

C++ 库没有这些能力，照抄会出问题：

#### 7.2.1 `RootOf(p, L..R)` 的耦合

[§4.4.3](#443-半代数边界的语义身份-vs-计算状态局限观察) 详述：身份（父 cell + p + 根索引）与计算状态（隔离区间）耦合在符号表达式里。结果：
- 同一根的不同精度副本不能直接判等
- 兄弟 cell 共享分界根时，无统一的精化机制

C++ 库需要**显式分离**身份与状态：
```
身份 (parent_cell, projection_poly, root_index) — 永不变
状态 (current_interval, precision_bits)         — 原地精化
```
两者用全局共享池关联，cell 持引用而非快照。

#### 7.2.2 "按需重构"的代价

ProjectionCAD 不存 cell 边界、不存投影多项式（输入参数），所有半代数描述靠 listwithrep + `PCAD_LWRCADtoPWCAD` 后处理。Maple 重算便宜（符号引擎缓存）；C++ 重算每次都是全量计算 → **CLPoly 应主动存边界引用**，避免重复展开。

#### 7.2.3 兄弟 cell 重复计算

ProjectionCAD `MakeCoprimeOverRC` / `MakeSquareFreeOverRC` 在每个 cell 上独立调用 `Triangularize` / `SquarefreeFactorization`，没有兄弟级缓存（详见上文关于 GenerateStack 的讨论）。Maple 内部可能有缓存；CLPoly 自己实现时应**显式按 (父 SPrc, 奇偶模式, polys 哈希) 缓存**。

### 7.3 数据结构层面的可能改进

如果 CLPoly 决定参考 ProjectionCAD 实现 Full CAD，建议在保持 cell 身份与父样本点共享的基础上，增加以下扩展：

```cpp
// 投影多项式池（按层组织）
struct projection_poly_entry {
    polynomial_ZZ poly;
    int level;
};

// 边界根：身份与状态分离
struct boundary_root_id {        // 不可变
    size_t parent_cell_ref;
    size_t projection_poly_ref;
    int root_index;              // 父 cell 上方按值升序的第 k 个根（delineable 保证良定义）
};

struct boundary_root_state {     // 可精化
    interval_QQ current_interval;
    int precision_bits;
};

struct boundary_root {
    boundary_root_id id;
    mutable boundary_root_state state;
};

struct cad_node {
    // 与 ProjectionCAD 对齐的部分（cell 身份）
    std::vector<int> cell_index;
    size_t sample_triangular_set_ref;
    std::vector<interval_QQ> isolating_cube;

    // CLPoly 扩展（半代数描述，每维一对边界根引用）
    // sector 维：(left_root, right_root)；首/尾用 SIZE_MAX 表 ±∞
    // section 维：(root, root) 同一引用
    std::vector<std::pair<size_t, size_t>> boundary_refs;

    // 树结构
    size_t parent_ref;
    std::vector<size_t> children;
};

class cad_tree {
    std::vector<cad_node> _nodes;
    std::vector<std::vector<polynomial_ZZ>> _triangular_sets;     // 共享样本点 RC
    std::vector<projection_poly_entry> _projection_polys;          // 按层
    std::vector<boundary_root> _boundary_roots;                    // 全局共享根池
};
```

**核心收益**：
- 兄弟 cell 共享分界根（池化），不冗余存储
- 半代数描述随时可用，无需后处理重构
- 跨精度判等永远靠 `id`，状态精化不破坏一致性
- Lazard CAD 的多项式 valuation 信息有挂载点

### 7.4 函数级映射（参考速查）

若实现 Full CAD，下表给出从 ProjectionCAD 函数到 CLPoly 已有 / 需补充模块的映射：

| ProjectionCAD 函数 | CLPoly 已有积木 | 需要补充 |
|-------------------|-----------------|----------|
| `PCAD_SPtoRootOf`（样本点→ RootOf） | 三角列 `std::vector<polynomial_>` | 三角列商环求值 |
| `subs(alpha, p)` + `is(= 0)` | — | 三角列商环约化 + 系数判零 |
| `PCAD_MinimalDelineatingPolynomial` | `derivative`、`gcd` | 直接组装 |
| `PCAD_MakeCoprimeOverRC` | `wrsd`, `ZDtoRC` (`charset.hh`) | 包装成"接受 cell + polys"的接口 |
| `PCAD_MakeSquareFreeOverRC` | `squarefreefactorize`（QQ 系数） | 扩展到三角列商环系数 |
| `PCAD_WhichRCInSplitHasSolutionAtSP` | — | 依赖代数数系数实根隔离 |
| `RegularChains:-TRDisolate_real_zeros` | `realroot.hh`（仅 QQ） | **代数数系数实根隔离**（最大缺口） |
| `RegularChains:-TRDgenerate_stack` | — | Stack 生成（依赖上面） |
| `PCAD_LWRCADtoPWCAD` | — | piecewise 输出（CLPoly 用嵌套条件树即可） |

**关键路径**：代数数系数一元多项式实根隔离是整个 Full CAD 的瓶颈，多个其它缺口都依赖它。理论参考：rc-cad-issac2009-analysis.md §6 IsolateZeros（可选本地附件：`rc-cad-issac2009-analysis.md`）。

### 7.5 整体路线选择

如果未来要实现 Full CAD，主要有两条路线：

**路线 A：照搬 ProjectionCAD 的 PL-CAD 架构**
- 投影（CLPoly 已有 OpenCAD）+ 提升（补充 GenerateStack + IsolateZeros）
- 优势：CLPoly 投影部分已实现，增量开发成本低；cell 数据结构天然契合
- 参考蓝本：[projectioncad-analysis.md §3](#3-提升阶段关键)（编排）+ rc-cad-issac2009-analysis.md §7（可选本地附件：`rc-cad-issac2009-analysis.md`）（GenerateStack 算法）

**路线 B：完整复刻 RC-CAD 的三步法**
- InitialPartition → MakeCylindrical → MakeSemiAlgebraic
- 优势：理论清晰，能利用三角分解机器
- 劣势：需实现 SeparateZeros / SMPD / MakeCylindrical 9 步，工作量大
- 参考蓝本：[rc-cad-issac2009-analysis.md](projectioncad/CAD-via-Triangular-Decomposition-ISSAC2009-中文详解.md)

**推荐**：路线 A —— 与 CLPoly 现有架构对齐，且 GenerateStack + IsolateZeros 是相对自洽的算法单元，不需要先实现整个复 CAD 机器。

---

## 附录：参考文献

| 论文 | 作者 | 年份 | 核心内容 | 文件 |
|------|------|------|----------|------|
| arXiv:1306.3062 | England et al. | 2013 | ProjectionCAD V1：Collins + McCallum，order-invariant CAD | `ProjectionCAD-V1-Collins-McCallum.pdf` |
| arXiv:1401.0645 | England et al. | 2014 | ProjectionCAD V2：ECCAD + TTICAD，lifting 改进 | `ProjectionCAD-V2-EC-TTICAD.pdf` |
| arXiv:1405.6090 | England et al. | 2014 | **架构论文**：RegularChains 调用，Algorithm 1-4 | `ProjectionCAD-Architecture-ICMS2014.pdf` |
| arXiv:1503.06599 | England et al. | 2015 | ProjectionCAD V3：Layered/Variety sub-CAD | `ProjectionCAD-V3-SubCAD.pdf` |
| arXiv:1805.10136 | Cowen-Rivers & England | 2018 | 增量 CAD + Lazard 投影 | `Incremental-CAD-Maple.pdf` |
| ISSAC 2009 | Chen, Moreno Maza, Xia, Yang | 2009 | RegularChains CAD 理论基础，TRDgenerate_stack 算法 | — |
| ASCM 2009 | Boulier et al. | 2009 | Real root isolation of regular chains | — |
