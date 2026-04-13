# 论文摘要：Deciding Satisfiability of Polynomial Formulas by Sample-Cell Projection

> Haokun Li, Bican Xia, Tianqi Zhao, arXiv:2003.00409 (2020)
> PDF: `li2020_sample_cell.pdf`
> GitHub: https://github.com/lihaokun/LiMbS
> 角色：**CAD/SMT 基准测试用例来源**

---

## 论文概述

本文提出了一种新的用于判定实数域上多项式公式可满足性的算法。关键点是设计了一个新的投影算子——sample-cell projection operator，专为 CDCL 风格的搜索定制。该算子虽然是类 CAD 投影算子，但具有单指数时间复杂度，能有效引导搜索远离冲突状态。

---

## 测试用例集

论文提供了 21 个基准测试用例，收集自多篇相关论文。GitHub 仓库 `LiMbS/Example_smt2/` 中包含所有用例的 SMT-LIB 格式文件。

### 用例信息汇总

| 用例名 | 变量数 | 类型 | LiMbS 时间 | Z3 时间 | CVC4 时间 | 说明 |
|--------|--------|------|------------|---------|-----------|------|
| Han 3 | 3 | SAT | 0.01s | 0.01s | 0.01s | 简单 |
| Han 4 | 4 | UNSAT | 0.08s | 0.01s | > 5h | Han 系列 |
| Han 5 | 5 | UNSAT | 1.26s | > 5h | > 5h | Han 系列 |
| Han 6 | 6 | UNSAT | 60s | > 5h | > 5h | Han 系列 |
| P | 6 | SAT | 1.06s | 0.05s | > 5h | 复杂多项式 |
| Hong 10 | 10 | UNSAT | 222s | 2058s | 0.01s | Hong 系列 |
| Hong 11 | 11 | UNSAT | 806s | 6357s | 0.01s | Hong 系列 |
| Hong2 11 | 11 | SAT | 30.43s | 1997s | 0.01s | Hong2 系列 |
| Hong2 12 | 12 | SAT | 563s | 6693s | 0.01s | Hong2 系列 |
| C 3 1 | 6 | UNSAT | 0.44s | > 5h | 0.62s | C n r 系列 |
| C 3 32 | 6 | UNSAT | 0.48s | > 5h | unknown | C n r 系列 |
| C 3 63 | 6 | UNSAT | 0.48s | > 5h | unknown | C n r 系列 |
| C 3 64 | 6 | SAT | 0.02s | 4682s | unknown | C n r 系列 |
| C 4 1 | 8 | UNSAT | 1.31s | > 5h | 2.28s | C n r 系列 |
| C 4 32 | 8 | UNSAT | 1.42s | > 5h | unknown | C n r 系列 |
| C 4 63 | 8 | UNSAT | 1.42s | > 5h | unknown | C n r 系列 |
| C 4 64 | 8 | SAT | 0.02s | > 5h | unknown | C n r 系列 |
| C 5 1 | 10 | UNSAT | 5.48s | > 5h | 19.33s | C n r 系列 |
| C 5 32 | 10 | UNSAT | 5.73s | > 5h | unknown | C n r 系列 |
| C 5 63 | 10 | UNSAT | 5.68s | > 5h | unknown | C n r 系列 |
| C 5 64 | 10 | SAT | 0.02s | > 5h | unknown | C n r 系列 |

---

## 具体多项式系统（从 GitHub 仓库提取）

### Han n 系列 [9]

判定是否存在 x1, ..., xn 使得：
```
(Σ(i=1 to n) x_i^2)^2 - 4*(Σ(i=1 to n) x_i^2 * x_{i+1}^2) < 0
```
其中 x_{n+1} = x_1。

**Han-3** (SMT-LIB: `han_3.smt2`):
- 变量: x1, x2, x3 (3 个变量)
- 多项式（不等式形式）:
  ```
  (x1^2 + x2^2 + x3^2)^2 - 4*(x1^2*x2^2 + x1^2*x3^2 + x2^2*x3^2) < 0
  ```
- 展开形式:
  ```
  -4*(x1^2*x2^2 + x1^2*x3^2 + x2^2*x3^2) + (x1^2 + x2^2 + x3^2)^2
  ```

**用例规模**: n = 3, 4, 5, 6

---

### P (复杂多项式)

来自 SMT-LIB: `P.smt2`。以下给出其因式分解形式，展开后即为原多项式。

**变量**: a, b, c, d, e, f (6 个变量)

**多项式**（不等式 < 0）:
```
P = (a*b + c)*(c - e*f + c*f^2)*(1 + b^2 + d^2
    + a*(-2*(b*c + d*e) + a*(1 + c^2 + e^2))
    + 2*(b - a*c)*(-d + a*e)*f
    + (b - a*c)^2*f^2)
  - a*b*(d*f - b*(1 + f^2) + a*(c - e*f + c*f^2))
      *(-d*e + c*d*f - b*(c - e*f + c*f^2)
        + a*(1 + e^2 + c*(c - 2*e*f + c*f^2)))
```

这是一个非常复杂的 6 变量多项式，用于测试求解器处理复杂表达式的能力。

---

### Hong n 和 Hong2 n 系列 [12]

来自 SMT-LIB，是 Hong 1991 年论文中经典问题的参数化版本。

**Hong 10** (SMT-LIB: `hong_10.smt2`):
- 变量: x_0, x_1, ..., x_9 (10 个变量)
- 约束:
  ```
  x_0^2 + x_1^2 + ... + x_9^2 < 1
  x_0 * x_1 * ... * x_9 > 1
  ```

**Hong 11** (SMT-LIB: `hong_11.smt2`):
- 变量: x_0, x_1, ..., x_10 (11 个变量)
- 约束:
  ```
  x_0^2 + x_1^2 + ... + x_10^2 < 1
  x_0 * x_1 * ... * x_10 > 1
  ```

**Hong2 11** (SMT-LIB: `hong2_11.smt2`):
- 变量: x_0, x_1, ..., x_10 (11 个变量)
- 约束:
  ```
  x_0^2 + x_1^2 + ... + x_10^2 < 1
  x_0 > 1 ∧ x_1 > 1 ∧ ... ∧ x_10 > 1
  ```

**一般形式**:
- **Hong n**: `Σ xi^2 < 1 ∧ Π xi > 1`
- **Hong2 n**: `Σ xi^2 < 1 ∧ ⋀(xi > 1)`

---

### C n r 系列

判定球 B_r(x̄) 与 B_8(ȳ) 的补集之间的距离是否小于 1/1000。

**C 3 1** (SMT-LIB: `C_3_1.smt2`):
- 变量: x1, x2, x3, y1, y2, y3 (6 个变量)
- 参数: n=3, r=1
- 约束:
  ```
  x1^2 + x2^2 + x3^2 < 1          (球 B_1)
  y1^2 + y2^2 + y3^2 >= 8         (B_8 的补集)
  (x1-y1)^2 + (x2-y2)^2 + (x3-y3)^2 < 1/1000^2
  ```

**用例规模**: 
- n = 3, r ∈ {1, 32, 63, 64}
- n = 4, r ∈ {1, 32, 63, 64}
- n = 5, r ∈ {1, 32, 63, 64}

---

## 文件位置

GitHub 仓库中的测试用例文件：
- `LiMbS/Example_smt2/*.smt2` - SMT-LIB 格式测试用例

---

## 求解器对比

测试环境: 6-Core Intel Core i7-8750H@2.20GHz, 32GB RAM, Arch Linux
超时设置: 5 小时

对比求解器:
- **LiMbS**: 本文原型求解器 (<1000 行代码)
- **Z3**: Microsoft 的 SMT 求解器
- **CVC4**: 知名 SMT 求解器
- **MathSAT5**: 工业级 SMT 求解器
- **Yices**: SMT 求解器

**关键结果**:
- LiMbS 解决了全部 21 个用例
- LiMbS 在 15 个用例上最快
- 9 个用例只有 LiMbS 能在合理时间内解决
- Hong n 和 Hong2 n 系列具有对称性，CVC4 利用对称性优化表现最佳

---

## 对 CLPoly 的意义

1. **SMT 风格测试**: 这些用例以存在量词公式形式给出，适合测试 OpenCAD 在判定问题上的应用
2. **可扩展系列**: Han n、Hong n、Hong2 n、C n r 都是可扩展的系列
3. **难度分级**: 从简单 (Han 3, 0.01s) 到困难 (Hong2 12, 563s)
4. **已知结果**: 每个用例都有明确的 SAT/UNSAT 结果
5. **SMT-LIB 格式**: 提供标准格式的测试用例，可直接使用
6. **与其他求解器对比**: 提供了与 Z3、CVC4 等工业级求解器的性能对比基准
