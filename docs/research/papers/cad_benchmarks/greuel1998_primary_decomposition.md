# 论文摘要：Primary Decomposition: Algorithms and Comparisons

> Wolfram Decker, Gert-Martin Greuel, Gerhard Pfister  
> Universitäten des Saarlandes und Kaiserslautern (1998)  
> PDF: `greuel1998_primary_decomposition.pdf`  
> 角色：**多项式系统测试用例来源（可用于 CAD 测试）**

---

## 论文概述

本文是关于多项式理想准素分解（Primary Decomposition）的经典论文，比较了四种主要算法：
- **GTZ**: Gianni, Trager, Zacharias (1988)
- **EHV**: Eisenbud, Huneke, Vasconcelos (1992)
- **SY**: Shimoyama, Yokoyama
- **特征集方法**: 基于 Ritt-Wu 过程

论文提供了 34 个测试用例，虽然原本用于测试准素分解算法，但这些多项式系统也可以用于测试 CAD。

---

## 测试用例集（34 个例子）

### 例子列表

| # | 名称 | 变量数 | 说明 | 来源 |
|---|------|--------|------|------|
| 1 | Chemistry | 7 | 玻璃熔化中的化学过程 | - |
| 2 | Sturmfels-Eisenbud | 9 | 3×3 矩阵的 2×2 永久子式 | [ES] |
| 3 | Shimoyama-Yokoyama J | 3 | SY 论文例子 J | [SY] |
| 4 | Shimoyama-Yokoyama St | 6 | SY 论文例子 St | [SY] |
| 5 | Butcher | 8 | POSSO 测试集 | [W], [BGK] |
| 6 | Gonnet | 19 | POSSO 测试集 | [BGK] |
| 7 | Horrocks | 6 | P⁵ 上的 Horrocks 丛 | [DMS] |
| 8 | Arnborg-Lazard | 3 | POSSO 测试集 | - |
| 9 | Schwarz | 5 | 群论中的幂等元构造 | - |
| 10 | Katsura4 | 5 | POSSO 测试集 | - |
| 11 | Katsura5 | 6 | POSSO 测试集 | - |
| 12 | Cyclic-roots-5-homog | 6 | 5 次循环根（齐次） | [BF] |
| 13 | Cyclic-roots-5 | 5 | 5 次循环根 | [BF], POSSO |
| 14 | Cyclic-roots-4 | 4 | 4 次循环根 | [BF], POSSO |
| 15 | Roczen | 15 | 正特征奇点分类 | - |
| 16 | De Jong | 10 | 有理四重点的形变 | - |
| 17 | Becker-Niermann | 3 | FGLM 测试 | - |
| 18 | Caprasse4 | 4 | POSSO 测试集 | - |
| 19 | Cassou | 4 | POSSO 测试集 | - |
| 20 | mat32 | 9 | 3×3 矩阵的平方 | POSSO |
| 21 | Shimoyama-Yokoyama I8 | 11 | SY 论文例子 I8 | [SY] |
| 22 | Gerdt | 7 | POSSO 测试集 | [BGK] |
| 23 | Möller | 7 | - | [Moe] |
| 24 | Riemenschneider | 8 | 商奇点形变 | - |
| 25 | Mikro | 4 | 模拟电路分析 | - |
| 26 | Amrhein | 6 | - | [AGK] |
| 27 | Buchberger | 4 | POSSO 测试集 | - |
| 28 | Lanconelli | 11 | POSSO 测试集 | - |
| 29 | Huneke | 4 | - | - |
| 30 | Wang1 | 11 | - | [W] |
| 31 | Wang2 | 4 | - | [W] |
| 32 | Siebert | 5 | - | - |
| 33 | Macaulay | 4 | Macaulay2 教程 | - |
| 34 | Amrhein2 | 7 | - | [AGK] |

---

## 性能数据（Table 1）

在 HP 720 上，使用 SINGULAR 计算 34 个例子的性能（秒）：

| 算法 | 说明 | 特点 |
|------|------|------|
| CharSet | 特征集方法 | 基于 Ritt-Wu 过程 |
| EHV | Eisenbud-Huneke-Vasconcelos | 使用同调代数 |
| GTZ | Gianni-Trager-Zacharias | 归约到零维情形 |
| SY | Shimoyama-Yokoyama | 从极小素理想构造 |

**关键观察**:
- 没有单一最佳策略
- 特征集方法在需要大量扩域分解时有问题
- GTZ 在需要随机坐标变换时有问题
- 通常应尽可能使用因式分解

---

## 对 CAD 的意义

虽然这些例子原本用于准素分解，但它们具有以下特点，适合测试 CAD：

1. **多样性**: 来自不同领域（化学、代数几何、电路分析等）
2. **难度分级**: 从简单（<1秒）到困难（>3小时）
3. **变量数范围**: 3-19 个变量
4. **结构多样**: 稀疏、稠密、齐次、非齐次等
5. **经典用例**: 许多来自 POSSO 测试集，被广泛研究

### 特别适合 CAD 测试的例子

| 例子 | 特点 | CAD 测试价值 |
|------|------|-------------|
| Cyclic-roots-n | 循环多项式 | 经典测试用例 |
| Katsura-n | 物理系统 | 实际应用背景 |
| Caprasse4 | 4 变量复杂系统 | 中等难度 |
| Butcher | 8 变量 | 高维测试 |

---

## 数据获取

- **论文**: https://arxiv.org/abs/math/9809000（或原始版本）
- **SINGULAR**: https://www.singular.uni-kl.de/
- **primdec.lib**: SINGULAR 中的准素分解库

---

## 参考文献

- [AGK] Amrhein, Gloor, Kächlin: Walking faster. DISCO 1996
- [BF] Backelin, Fröberg: Cyclic 7-roots. ISSAC 1991
- [BGK] Boege, Gebauer, Kredel: Solving systems of algebraic equations. J. Symb. Comp. 1987
- [DMS] Decker, Manolache, Schreyer: Geometry of the Horrocks-bundle on P⁵
- [ES] Eisenbud, Sturmfels: Binomial Ideals. Duke Math. J. 1996
- [Moe] Möller: Solving of algebraic equations
- [SY] Shimoyama, Yokoyama: Primary decomposition
- [W] Wang: Elimination methods

---

## 备注

这些测试用例原本用于准素分解，但多项式系统本身可以用于：
- **CAD 分解**: 测试柱形代数分解的效率
- **Gröbner 基**: 测试 Gröbner 基计算
- **因式分解**: 测试多变量多项式因式分解
- **实根隔离**: 测试实代数几何算法

建议从变量数较少的例子开始（如 #3, #8, #14, #17, #31），逐步测试更复杂的系统。
