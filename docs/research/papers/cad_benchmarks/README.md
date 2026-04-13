# CAD Benchmark 论文集合

本目录收集与 CAD (Cylindrical Algebraic Decomposition) benchmark 相关的学术论文，用于 CLPoly 项目的测试用例设计和性能对比。

---

## 论文列表

| 文件 | 作者 | 年份 | 主题 | 测试用例数 |
|------|------|------|------|-----------|
| [chen2023_geometric_cad.md](./chen2023_geometric_cad.md) | Chen, Luo, Wang, Xiao | 2023 | GeometricCAD: 几何驱动的 CAD | 20 |
| [li2020_sample_cell.md](./li2020_sample_cell.md) | Li, Xia, Zhao, Zheng, Zhang, Zhan | 2020 | Sample-Cell: 基于样本的 CAD | 21 |
| [delrio2022_smt_cad.md](./delrio2022_smt_cad.md) | del Río, England | 2022 | SMT-LIB CAD 启发式对比 | 1019 |
| [jing2026_variable_ordering.md](./jing2026_variable_ordering.md) | Jing, Li, Chen, Zhi | 2026 | 基于强化学习的变量排序 | 多个数据集 |
| [greuel1998_primary_decomposition.md](./greuel1998_primary_decomposition.md) | Decker, Greuel, Pfister | 1998 | 准素分解算法对比 | 34 |
| [lee2025_macaulay2_cad.md](./lee2025_macaulay2_cad.md) | Lee, del Río, Rahkooy | 2025 | Macaulay2 CAD 实现 | 超球面测试 |

---

## 测试用例文件

- **[cad_test_cases.txt](./cad_test_cases.txt)** - 通用测试用例格式，可被 Mathematica/Maple/CLPoly 读取
  - 当前包含 45 个测试用例（4 Lee2025 + 20 Chen2023 + 21 Li2020 [Han-3/4/5/6, P, Hong-10/11, Hong2-11/12, C-3/4/5 系列]）
  - 格式：`[BEGIN name]`, `VARS: x, y, ...`, `POLYS:`, `[END name]`

---

## 按主题分类

### CAD 实现
- Chen2023 - GeometricCAD (Maple)
- Li2020 - Sample-Cell (C++)
- Lee2025 - Macaulay2 CAD package

### 变量排序启发式
- delRio2022 - gmods 启发式
- Jing2026 - 强化学习方法

### 相关算法
- Greuel1998 - 准素分解 (多项式系统求解)

---

## 文件命名约定

```
<作者><year>_<descriptive_keyword>.md
<作者><year>_<descriptive_keyword>.pdf
```

例如：
- `chen2023_geometric_cad.md`
- `lee2025_macaulay2_cad.pdf`
