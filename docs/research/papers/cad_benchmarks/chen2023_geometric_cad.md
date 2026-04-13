# 论文摘要：A Geometric Approach to Cylindrical Algebraic Decomposition

> Rizeng Chen, arXiv:2311.10515 (2023-2025)
> PDF: `chen2023_geometric_cad.pdf`
> GitHub: https://github.com/xiaxueqaq/GeometricCADv2
> 角色：**CAD 基准测试用例来源**

---

## 论文概述

本文从几何角度重新审视 CAD 构造，将 CAD 与实代数簇之间的态射研究联系起来。证明了几何纤维基数决定半代数连续截面的存在性。基于此，所有方程都可以在投影阶段被系统性地利用，导出一个新的简单算法 GeometricCAD。

---

## 测试用例集

论文第 6.2 节提供了 20 个基准测试用例，来自多个文献来源。代码仓库 `GeometricCADv2/Experiments/` 中包含所有用例的 Mathematica 文件。

### 用例信息汇总 (Table 2)

| 用例名 | 变量数 | 多项式数 | 方程数 | 维度 | 来源 |
|--------|--------|----------|--------|------|------|
| 5-VAR | 5 | 1 | 0 | 5 | 本文 Example 2 |
| Hong-4 | 4 | 2 | 0 | 4 | [Hon91] |
| Hong-5 | 5 | 2 | 0 | 5 | [Hon91] |
| Hong-6 | 6 | 2 | 0 | 6 | [Hon91] |
| Hong-7 | 7 | 2 | 0 | 7 | [Hon91] |
| Hong-8 | 8 | 2 | 0 | 8 | [Hon91] |
| cyclic-4 | 4 | 4 | 4 | 0 | 经典循环多项式 |
| cyclic-5 | 5 | 5 | 5 | 0 | 经典循环多项式 |
| cyclic-6 | 6 | 6 | 6 | 0 | 经典循环多项式 |
| EBD-2 | 3 | 3 | 2 | 1 | [EBD20] |
| EBD-5 | 5 | 6 | 4 | 1 | [EBD20] |
| EBD-8 | 4 | 4 | 3 | 1 | [EBD20] |
| FIS-1 | 5 | 2 | 1 | 4 | [FIS15] |
| FIS-2 | 4 | 3 | 3 | 1 | [FIS15] |
| FIS-3 | 7 | 4 | 4 | 3 | [FIS15] |
| FIS-4 | 3 | 3 | 2 | 1 | [FIS15] |
| FIS-5 | 5 | 5 | 5 | 0 | [FIS15] |
| FIS-6 | 5 | 3 | 3 | 2 | [FIS15] |
| FIS-7 | 5 | 6 | 5 | 1 | [FIS15] |
| FIS-8 | 10 | 15 | 12 | 0 | [FIS15] |

### 性能数据 (Table 3)

| 用例 | QEPCAD B 时间 | QEPCAD B 细胞数 | RC-CAD 时间 | RC-CAD 细胞数 | GeometricCAD 时间 | GeometricCAD 细胞数 |
|------|---------------|-----------------|-------------|---------------|-------------------|---------------------|
| 5-VAR | 1.50s | 297 | 0.09s | 75 | 0.08s | 75 |
| Hong-4 | 1.35s | 575 | 1.19s | 575 | 0.56s | 575 |
| Hong-5 | 1.35s | 2241 | 5.16s | 2241 | 2.53s | 2241 |
| Hong-6 | 1.70s | 7967 | 20.00s | 7967 | 9.77s | 7967 |
| Hong-7 | >1h | N/A | 133.86s | N/A | 49.52s | 19743 |
| Hong-8 | >1h | N/A | Error | N/A | 270.33s | 103343 |
| cyclic-4 | 1.56s | 129 | 0.06s | 4 | 0.03s | 4 |
| cyclic-5 | >1h | N/A | 4.58s | N/A | 0.58s | 10 |
| cyclic-6 | >1h | N/A | >1h | N/A | 5.48s | 24 |
| EBD-2 | 1.17s | 41 | 0.02s | 4 | 0.02s | 5 |
| EBD-5 | 1.33s | 107 | 0.27s | 13 | 0.05s | 10 |
| EBD-8 | 107.89s | 257 | >1h | N/A | 0.23s | 4 |
| FIS-1 | 3.68s | 114541 | 40.25s | 5333 | 16.11s | 4063 |
| FIS-2 | 1101.60s | 5467 | 1.59s | 79 | 2.66s | 299 |
| FIS-3 | >1h | N/A | 644.91s | 16928 | 1654.84s | 66704 |
| FIS-4 | 4.76s | 227 | 1.83s | 24 | 0.19s | 16 |
| FIS-5 | >1h | N/A | >1h | N/A | 0.20s | 3 |
| FIS-6 | >1h | N/A | >1h | N/A | 2834.00s | 32875 |
| FIS-7 | Error | N/A | >1h | N/A | 10.67s | 200 |
| FIS-8 | Error | N/A | 122.58s | N/A | 12.75s | 3 |

---

## 具体多项式系统（从 GitHub 仓库提取）

### 5-VAR (Example 2 in paper)
**变量**: x, y, z, u, w (5 个变量)

**多项式**:
```
f = w^2 + (x + y)^2 * u - y*z
```

构建 f-符号不变 CAD of R^5。这是论文中的关键示例，展示了 nullification problem。

---

### Hong-n 系列 [Hon91]
来自 SMT-LIB 的经典测试用例。

**Hong-4**:
- 变量: x0, x1, x2, x3 (4 个变量)
- 多项式:
  ```
  f1 = x0^2 + x1^2 + x2^2 + x3^2 - 1
  f2 = x0 * x1 * x2 * x3 - 1
  ```

**Hong-5**:
- 变量: x0, x1, x2, x3, x4 (5 个变量)
- 多项式:
  ```
  f1 = x0^2 + x1^2 + x2^2 + x3^2 + x4^2 - 1
  f2 = x0 * x1 * x2 * x3 * x4 - 1
  ```

**一般形式 Hong-n**:
```
∃x1,...,∃xn: Σ(i=1 to n) xi^2 < 1 ∧ Π(i=1 to n) xi > 1
```

---

### cyclic-n 系列
经典循环多项式系统，n 个变量 n 个方程。

**cyclic-4**:
- 变量: a, b, c, d (4 个变量)
- 多项式:
  ```
  f1 = a + b + c + d
  f2 = ab + bc + cd + da
  f3 = abc + bcd + cda + dab
  f4 = abcd - 1
  ```

**一般形式 cyclic-n**:
```
f1 = Σ xi
f2 = Σ xi*xi+1 (循环)
...
fn = Π xi - 1
```

---

### EBD 系列 [EBD20]
来自 England, Bradford, Davenport 的论文《Cylindrical algebraic decomposition with equational constraints》(JSC 2020)。Chen2023 从中选取了 3 个独立例子作为 benchmark，编号 2/5/8 不代表参数化族。

**EBD-2** (3 变量, 3 多项式, 2 方程):
- 变量: x, y, z
- 多项式:
  ```
  f1 = x + y^2 + z
  f2 = x - y^2 + z
  f3 = x^2 + y^2 + z^2 - 1
  ```

**EBD-5** (5 变量, 6 多项式, 4 方程):
- 变量: v, u, x, y, z
- 多项式:
  ```
  f1 = x - y + z^2
  f2 = z^2 - u^2 + v^2 - 1
  f3 = x + y + z^2
  f4 = z^2 + u^2 - v^2 - 1
  f5 = x^2 - 1
  f6 = z
  ```

**EBD-8** (4 变量, 4 多项式, 3 方程):
- 变量: w, x, y, z
- 多项式:
  ```
  f1 = x*y - z^2 - w^2 + 2*z
  f2 = x^2 + y^2 + z^2 + w + z
  f3 = -w^2 - y^2 - z^2 + x + z
  f4 = z + w
  ```

---

### FIS 系列 [FIS15]
来自 Fukui, Iwamoto, Sun 的论文，用于测试 equational constraints。以下从 GeometricCADv2 仓库提取。

**FIS-1** (5 变量, 2 多项式, 1 方程):
- 变量: a, b, c, x, y
- 多项式:
  ```
  f1 = b^2 (x - c)^2 + a^2 y^2 - a^2 b^2
  f2 = x^2 + y^2 - 1
  ```

**FIS-2** (4 变量, 3 多项式, 3 方程):
- 变量: s, x, y, z
- 多项式:
  ```
  f1 = x s (1 - x/400)/200 + y s (1 - x/400) - 35 x/2
  f2 = 250 x s (1 - y/600) (z + 3/250) - 55 y/2
  f3 = 500 (y + x/20) (1 - z/700) - 5 z
  ```

**FIS-3** (7 变量, 4 多项式, 4 方程):
- 变量: l, z, r, s1, c1, s2, c2
- 多项式:
  ```
  f1 = r - c1 + l (s1 s2 - c1 c2)
  f2 = z - s1 - l (s1 c2 + s2 c1)
  f3 = s1^2 + c1^2 - 1
  f4 = s2^2 + c2^2 - 1
  ```

**FIS-4** (3 变量, 3 多项式, 2 方程):
- 变量: x, y, z
- 多项式:
  ```
  f1 = x^2 + y^2 z + z^3
  f2 = 3 x^2 + 3 y^2 + z^2 - 1
  f3 = x^2 + z^2 - y^3 (y - 1)^3
  ```

**FIS-5** (5 变量, 5 多项式, 5 方程):
- 变量: a, b, x, y, z
- 多项式:
  ```
  f1 = x y + a x z + y z - 1
  f2 = x y z + x z + x y - a
  f3 = x z + y z - a z - x - y - 1
  f4 = a x y - b y z
  f5 = a y z - b z x
  ```

**FIS-6** (5 变量, 3 多项式, 3 方程):
- 变量: a, b, x, y, z
- 多项式:
  ```
  f1 = x y + a x z + y z - 1
  f2 = x y z + x z + x y - b
  f3 = x z + y z - a z - x - y - 1
  ```

**FIS-7** (5 变量, 5 多项式, 5 方程):
- 变量: x4, x1, x3, x2, x0
- 多项式:
  ```
  f1 = x0 x2 - 1
  f2 = x0^2 - 2 x0 x1 + x2^2 - 2 x2 x3 - x4
  f3 = -16 x1 x4^2 - 800 x3^3 - 1240 x3^2 x4 - 408 x3 x4^2 - 40 x4^3 + 240 x1 x3 - 532 x1 x4 - 17720 x3^2 - 6214 x3 x4 - 550 x4^2 - 4480 x1 + 25240 x3 + 5695 x4 + 1050
  f4 = 32 x1^2 + 168 x1 x3 + 40 x1 x4 + 8 x3^2 + 20 x3 x4 + 4 x4^2 - 270 x1 - 390 x3 - 105 x4 + 450
  f5 = 320 x1 x3 x4 + 32 x1 x4^2 + 16 x3 x4^2 + 8320 x1 x3 + 264 x1 x4 + 240 x3^2 - 372 x3 x4 - 140 x4^2 - 14840 x1 - 23380 x3 - 2575 x4 + 36750
  ```

**FIS-8** (10 变量, 13 多项式, 12 方程):
- 变量: x8, x9, x0, x1, x2, x3, x4, x5, x6, x7
- 多项式:
  ```
  f1  = x4
  f2  = x5
  f3  = x0 x8 + x1 x9 - 1
  f4  = 2 x0 x2 + 2 x1 x3 + 2 x4 x5 - x0 - x1 - x4
  f5  = x0 x6 + x0 x7 - x1 x7 - x4 x6 - x0 + x8
  f6  = x2 x6 + x2 x7 - x3 x7 - x5 x6 - x2 + x9
  f7  = 2 x0 x2 - 2 x0 x4 + 2 x1 x3 - 2 x1 x5 - x2^2 - x3^2 + x4^2 + x5^2
  f8  = x0^2 + x1^2 - x2^2 - x3^2 + x4^2 - x5^2 + x2 + x3 + x5
  f9  = 14 x9^3 + 2 x8^2 - 12 x8 x9 - 16 x9^2 + 4 x8 + 17 x9 - 10
  f10 = 14 x8 x9^2 - 3 x8^2 - 10 x8 x9 - 11 x9^2 + x8 + 20 x9 - 6
  f11 = 14 x8^2 x9 - 6 x8^2 - 20 x8 x9 + 20 x9^2 + 16 x8 - 9 x9 + 2
  f12 = 14 x8^3 - 19 x8^2 + 58 x8 x9 + 5 x9^2 - 17 x8 - 32 x9 + 46
  f13 = x2 x3 (x2 - x3)
  ```

---

## 文件位置

GitHub 仓库中的测试用例文件：
- `GeometricCADv2/Experiments/GeometricCAD/*.nb` - GeometricCAD 用例 (Mathematica)
- `GeometricCADv2/Experiments/QEPCAD/*.txt` - QEPCAD 格式用例

---

## 参考文献

- [BF91] Babuška, I., & Faille, I. (1991). 
- [EBD20] England, M., Bradford, R., & Davenport, J. H. (2020).
- [FIS15] Fukui, T., Iwamoto, S., & Sun, X. (2015).
- [Hon91] Hong, H. (1991). 
- [McC98] McCallum, S. (1998).
- [Bro01] Brown, C. W. (2001).
- [CMM14a] Chen, C., Moreno Maza, M. (2014). Maple RegularChains.
- [CMM14b] Chen, C., et al. (2014). CAD by Regular Chains.

---

## 对 CLPoly 的意义

1. **基准测试来源**: 20 个测试用例覆盖 3-10 个变量，包含低维和高维簇
2. **可扩展用例**: Hong-n 和 cyclic-n 系列可按需扩展
3. **细胞数统计**: 可用于验证 CAD 实现的正确性（细胞数应匹配）
4. **GitHub 仓库**: 提供可直接使用的 Mathematica 格式测试用例
