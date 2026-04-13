# 论文摘要：Breaking the Data Barrier in Learning Symbolic Computation

> Rui-Juan Jing, Yuegang Zhao, Changbo Chen
> arXiv:2601.13731 [cs.SC] (2026)
> PDF: `jing2026_variable_ordering.pdf`
> 角色：**CAD 变量排序数据集参考**

---

## 论文概述

本文研究了如何使用 Transformer 模型来学习圆柱代数分解（CAD）的变量排序。主要贡献是设计了与 CAD 密切相关的预训练任务，通过这些任务可以生成大量标注数据，从而解决数据稀缺问题。

---

## 数据集

论文使用了多个公开数据集：

### 1. DQ-3 和 DQ-4（来自 Chen et al. 2024）

这两个是随机生成的 CAD 变量排序数据集：
- **DQ-3**: 3 个变量，12 个子数据集
- **DQ-4**: 4 个变量，10 个子数据集

数据集命名规则（例如 `REdEn4rCv3`）：
- `RE`: 默认设置（Random Example）
- `dE`: 总次数范围 1..2（degree range: E）
- `n4`: 4 个约束（n constraints）
- `rC`: 纯多项式（pure polynomial）
- `v3`: 3 个变量（variables）

**DQ-3 子数据集列表**:
- REcMdMn2p0tMv3
- REcMeEn2p0rCv3
- REcMeEn2tMv3
- REdEn4rCv3
- REdEn5rCv3
- REdEn6rCv3
- REdHn2v3
- REdMn2rCv3
- REdMn2v3
- REeEn2tMv3
- REeEn3p0rHv3
- REeMn2tMv3

**DQ-4 子数据集列表**:
- REdEn2rCv4
- REdEn2rHv4
- REdEn2rMv4
- REdEn2v4
- REdEn3rCv4
- REdEn3rHv4
- REdEn3rMv4
- REdEn3v4
- REdEn4rHv4
- REeEn2rHv4

### 2. DQ-4b（本文新增）

增强的 4 变量数据集，包含：
- 扩展的 REeEn2rHv4 子数据集
- 新增 REeEn3rHv4 子数据集
- 新增 REcMeEn3p0rCv4 子数据集

### 3. SMT CAD Order Data（来自 del Rio Almajan and England 2022）

- 1,019 个实例
- 3 个变量
- 来自 SMT 求解器的实际用例
- 用于评估模型在真实 SMT 输入上的泛化能力

---

## 预训练任务

论文设计了多个预训练任务：

| 任务 | 描述 |
|------|------|
| task_c | 约束数量预测 |
| task_e | 变量指数预测 |
| task_f | 特征向量预测 |
| task_m | 单项式乘积预测 |
| task_p | 投影因子数量预测 |
| task_r | 结果式计算 |
| task_s | 子结式链计算 |

---

## 关键结论

1. **预训练有效**: 通过预训练任务，模型在 CAD 变量排序任务上显著优于启发式方法
2. **数据规模**: 使用大规模预训练数据（数十万样本）可以突破数据稀缺瓶颈
3. **泛化能力**: 在随机数据上训练的模型可以泛化到真实 SMT 输入

---

## 数据获取

论文提到数据集可在以下位置获取：
- DQ-3, DQ-4: Chen et al. (2024)
- DQ-4b: https://doi.org/10.57760/sciencedb.31989
- SMT CAD Order Data: del Rio Almajan and England (2022)

---

## 对 CLPoly 的意义

1. **数据集参考**: 提供了随机生成 CAD 测试用例的方法论
2. **变量排序**: 论文中的变量排序数据集可用于测试 CLPoly 的投影阶段性能
3. **难度分级**: 数据集按 max CAD 运行时间分为 5 个难度区间
4. **预训练思路**: 可借鉴预训练任务的思想来优化 CLPoly 的算法选择

---

## 备注

这篇论文主要关注**变量排序**问题，而不是提供具体的 CAD 测试用例。实际的测试用例需要从上述数据集（DQ-3, DQ-4, DQ-4b, SMT）中获取。
