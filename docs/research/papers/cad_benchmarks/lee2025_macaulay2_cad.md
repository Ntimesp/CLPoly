# Cylindrical Algebraic Decomposition in Macaulay2

**Authors:** Alexei A. Lee, Tereso del Río, Hamid Rahkooy  
**Year:** 2025  
**arXiv:** 2503.21731  
**PDF:** [lee2025_macaulay2_cad.pdf](./lee2025_macaulay2_cad.pdf)

---

## 摘要

本文介绍了 **CylindricalAlgebraicDecomposition** —— 首个在 Macaulay2 计算机代数系统中实现的圆柱代数分解 (CAD) 软件包。该包实现了 Open CAD 算法（仅计算全维单元），使用 Lazard 投影算子，并集成了 gmods 变量排序启发式。

---

## 核心功能

### 主要函数

| 函数 | 功能描述 |
|------|----------|
| `findPositiveSolution` | 检查是否存在使所有输入多项式严格为正的点，若存在则返回该点 |
| `openCAD` | 构建给定多项式列表的 Open CAD，以样本点树的形式返回 |
| `positivePoint` | 在 CAD 样本点树中查找使所有多项式为正的点 |
| `projectionPhase` | 执行投影阶段，返回投影多项式列表和变量序 |
| `gmodsHeuristic` | 使用 gmods 启发式选择投影变量 |
| `lazardProjection` | 执行 Lazard 投影算子 |
| `liftingPoint` | 执行提升阶段，构建样本点树 |
| `samplePoints` | 对一元多项式进行实根隔离，返回开样本点 |
| `hashify` | 将可变哈希表转换为不可变哈希表以便查看 |

### 对 RealRoots 包的改进

- **多多项式支持**: 对输入多项式的乘积进行实根隔离
- **根界错误修复**: 修复了首项系数为负时根界计算错误
- **高效根界**: 改进了根界算法，更好地处理大系数
- **相近根优化**: 避免对已经充分隔离的根继续细化
- **有序输出**: 根区间始终按从低到高排序

---

## 算法细节

### Lazard 投影算子

对于多项式集合 $F_n$ 关于变量 $x_n$ 的投影包括：
1. 所有多项式的首项系数
2. 所有多项式的尾项系数（与 $x_n$ 无关的系数）
3. 所有多项式的判别式
4. 所有不同多项式对的结式

### gmods 启发式

基于 [dRE22] 的贪心模启发式，选择使投影多项式总次数模最小的变量进行投影。

### Open CAD 结构

输出为嵌套哈希表形式的树结构：
- 第 $k$ 层（$1 \leq k < n$）表示 $\mathbb{R}^k$ 的 CAD
- 包含投影多项式 $F_k$、下方单元（样本点）和 $x_k$ 的样本点集合
- 叶节点（第 $n$ 层）表示 $n$ 维样本点

---

## 示例

### 经典示例 (来自 [Jir95])

```macaulay2
R = QQ[x1,x2];
f1 = x1^2 + x2^2 - 1;
f2 = x1^3 - x2^2;
F = {f1, f2};

-- 查找使所有多项式为正的点
findPositiveSolution(F)
-- 输出: (true, HashTable{x1 => 17/8, x2 => 0})

-- 构建 Open CAD
openCAD(F)

-- 查看完整结构
peek openCAD(F)
```

### 单变量示例

```macaulay2
R = QQ[x];
findPositiveSolution({3-x^2, (7*x-12)*(x^2+x+1)})
-- 输出: (true, HashTable{x => 1539/896})
```

---

## 性能测试

### 超球面相交问题

测试问题：$n$ 维空间中两个相交球面的 CAD

$$F_n = \left\{ \sum_{i=1}^n (x_i - 1)^2 - 4, \quad \sum_{i=1}^n (x_i + 1)^2 - 4 \right\}$$

| 变量数 | 运行时间 (秒) | n-单元数 |
|--------|---------------|----------|
| 1 | 0.031 | 5 |
| 2 | 0.335 | 29 |
| 3 | 6.068 | 467 |
| 4 | 1883.13 | 7370 |

**结论**: 验证了 CAD 的双指数复杂度，4变量问题已接近可计算极限。

---

## 未来工作

1. **改进输出**: 在顶层样本点处评估输入多项式，简化符号检查
2. **扩展到完整 CAD**: 实现全维 CAD，支持量词消去
3. **连通性分析**: 结合邻接算法确定代数簇的连通分支数

---

## 相关论文

- **[Jir95]** Mats Jirstrand. "Cylindrical Algebraic Decomposition - an Introduction." Technical Report 1807, Linköping University, 1995.
  - 本文使用的经典示例来源
  
- **[dRE22]** Tereso del Río and Matthew England. "New Heuristic to Choose a Cylindrical Algebraic Decomposition Variable Ordering Motivated by Complexity Analysis." CASC 2022.
  - gmods 启发式的原始论文

- **[Laz94]** D. Lazard. "An Improved Projection for Cylindrical Algebraic Decomposition." 1994.
  - Lazard 投影算子

- **[MPP19]** Scott McCallum, Adam Parusiński, and Laurentiu Paunescu. "Validity proof of Lazard's method for CAD construction." Journal of Symbolic Computation, 2019.
  - Lazard 方法的正确性证明

---

## 引用信息

```bibtex
@article{lee2025macaulay2cad,
  title={Cylindrical Algebraic Decomposition in Macaulay2},
  author={Lee, Alexei A. and del R{\'i}o, Tereso and Rahkooy, Hamid},
  journal={arXiv preprint arXiv:2503.21731},
  year={2025}
}
```

---

## 备注

- **软件**: Macaulay2 的 CylindricalAlgebraicDecomposition 包 (版本 1.0.3)
- **算法类型**: Open CAD（仅全维单元）
- **投影算子**: Lazard projection
- **变量排序**: gmods 启发式
- **依赖包**: RealRoots (改进版)
