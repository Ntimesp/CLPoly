# 调研笔记：理论对照与代码问题记录

> 历史阅读辅助：保留原整理日期和观点，尚未逐条校正，不作为当前接口、算法选择或正确性结论。OCR、译述和页码以原文为准；当前复核见 [CAD 文档入口](../../cad/README.md)。

> **用途**：记录三角列/特征列理论调研过程中发现的 CLPoly 代码小问题、理论定义边界情况、以及需要后续验证的疑点。
> **更新日期**：2026-04-20

---

## 目录

- [charset.hh](#charsethh)
  - [Issue 1: `is_reduced` 对常数多项式的处理](#issue-1-is_reduced-对常数多项式的处理)
  - [Issue 2: `basset` 常数终止与扫描合并](#issue-2-basset-常数终止与扫描合并)
  - [Issue 3: `basset` 按 rank 排序的一遍扫描优化](#issue-3-basset-按-rank-排序的一遍扫描优化)
  - [Issue 6: `std::set` 的 `operator<` 与 rank 序的一致性](#issue-6-stdset-的-operator-与-rank-序的一致性)
- [cad.hh](#cadhh)
  - [Issue 4: Lazard 投影算子尾系数优化](#issue-4-lazard-投影算子尾系数优化)
  - [Issue 5: OpenCAD 可采用 Brown 投影算子](#issue-5-opencad-可采用-brown-投影算子)
  - [Issue 7: McCallum 投影算子系数集优化（结式截断）](#issue-7-mccallum-投影算子系数集优化结式截断)
- [待验证](#待验证)

---

## charset.hh

### Issue 1: `is_reduced` 对常数多项式的处理

> **发现日期**：2026-04-20
> **相关代码**：`clpoly/charset.hh:37-50`
> **相关理论**：定义 6.1.1（王东明 p.134）、定义 2.3（Xia & Yang p.27）

#### 问题描述

两本书对"约化"的定义都只针对**非常数**多项式：

- **王东明 p.134**：设 $T = [T_1, \ldots, T_r]$ 为三角列，$P$ 为任一多项式。若 $\deg(P, x_{p_i}) < \text{ldeg}(T_i)$ 对所有 $i$ 成立，则称 $P$ 关于 $T$ 是约化的。
- **Xia & Yang p.27**：若 $\deg(g, x_k) < \deg(f, x_k)$，则称 $g$ 关于 $f$ 约化。

**关键**：定义的前提是 $f$（或 $T_i$）有导元，即 $\text{cls}(f) > 0$。常数多项式没有导元，定义本身不适用。

CLPoly 的 `is_reduced(F, G)` 处理了两个边界情况：

```cpp
bool is_reduced(const polynomial_& F, const polynomial_& G)
{
    if(F.empty()) return true;           // 0 多项式
    if(is_number(G)) return false;       // G 是常数
    if(is_number(F)) return false;       // F 是常数 ← 有疑问

    if(degree(F, get_first_var(G)) < get_first_deg(G))
        return true;
    else
        return false;
}
```

#### 分析

| 情况 | 书中定义 | CLPoly 行为 | 影响 |
|------|---------|------------|------|
| G 是常数 | **未定义** | 返回 `false` | `basset` 中：常数作为基列终点，筛选为空，返回 `[constant]`。行为正确。 |
| F 是常数，G 非常数 | **未明说**，但常数次数为 0 < 任何正次数 | 返回 `false` | 常数不会被筛选为"约化"，但由于 `rankcompare` 中常数秩最低，会先被选入基列。不影响整体正确性，只是语义上应该是 `true`。 |

#### 结论

- `is_number(G) return false`：防御性处理，在基列算法语境下行为正确。
- `is_number(F) return false`：**语义小问题**。常数 F 对任何非常数 G 应该被认为是约化的（因为 $\deg(F, \text{lv}(G)) = 0 < \text{ldeg}(G)$）。但由于常数在 `rankcompare` 中秩最低，会先被选入基列，不影响整体正确性。

**优先级**：低。不影响正确性，但语义上可改进为 `if(is_number(F)) return true;`。

---

### Issue 2: `basset` 常数终止与扫描合并

> **发现日期**：2026-04-20
> **相关代码**：`clpoly/charset.hh:76-97`
> **相关理论**：算法 BasSet（王东明 p.138）

#### 问题描述

当前 `basset` 实现每轮循环**两遍扫描** F：

```cpp
while(!F.empty())
{
    // 第一遍：找最小秩
    std::size_t min=0;
    for(std::size_t i=1;i<F.size();++i)
        if(rankcompare(F[i],F[min]))    min=i;
    B.push_back(F[min]);

    // 第二遍：筛选约化的多项式
    Temp.clear();
    for(auto i:F)
        if(is_reduced(i,F[min]))  Temp.push_back(i);
    F=std::move(Temp);
}
```

#### 优化点 1：常数直接终止

书中算法 B2.3 明确："如果 cls(B) = 0, 则命 F := ∅"。当前实现缺少这个快速路径——虽然 `is_reduced(i, constant)` 返回 `false` 导致 `Temp` 为空、循环自然终止，但多走了一轮无意义筛选。

**优化**：
```cpp
B.push_back(F[min]);
if(is_number(B.back())) break;  // 或 return {B.back()};
```

#### 优化点 2：合并两遍扫描

```cpp
// 一遍扫描：同时找最小秩 + 分类
std::size_t min_idx = 0;
for(std::size_t i = 1; i < F.size(); ++i)
    if(rankcompare(F[i], F[min_idx])) min_idx = i;

B.push_back(std::move(F[min_idx]));
if(is_number(B.back())) return B;

std::vector<polynomial_> next_F;
next_F.reserve(F.size() - 1);
for(std::size_t i = 0; i < F.size(); ++i)
{
    if(i == min_idx) continue;
    if(is_reduced(F[i], B.back()))
        next_F.push_back(std::move(F[i]));
}
F = std::move(next_F);
```

**优先级**：中。常数优化收益小（仅矛盾列场景），扫描合并减少约 2x 常数因子。

---

### Issue 3: `basset` 按 rank 排序的一遍扫描优化

> **发现日期**：2026-04-20
> **相关代码**：`clpoly/charset.hh:76-97`
> **相关理论**：算法 BasSet（王东明 p.138）

#### 问题描述

如果输入 F 按 **rank 序**（rank 相同则 lex 序打破平局）排序，basset 可以**一遍扫描**完成：

```cpp
// 假设 F 已按 rank 升序排列
std::vector<polynomial_> B;
for(const auto& f : F)
{
    bool ok = true;
    for(const auto& b : B)
        if(!is_reduced(f, b)) { ok = false; break; }
    if(ok) B.push_back(f);
}
```

#### 复杂度分析

| 实现 | 复杂度 | 说明 |
|-----|--------|------|
| 当前 | O(\|F\| × 轮数) | 每轮完整扫描 F，轮数 ≤ n（变元数）|
| 排序后一遍扫描 | O(\|F\| × n) | 内层检查 B，\|B\| ≤ n |

由于轮数 ≤ n，两者**同阶**。但排序后实现：
- 无 Temp vector 反复构建
- 顺序访问，缓存友好
- `charset` 层面若保持有序，每轮无需重新找最小秩

#### charset 层面的联动

当前 `charset` 使用 `std::set`（lex 序），不是 rank 序。若改为 rank 有序容器：

```cpp
// 需要自定义比较器
struct rank_less {
    bool operator()(const polynomial_& a, const polynomial_& b) const {
        if(rankcompare(a, b)) return true;
        if(rankcompare(b, a)) return false;
        return a < b;  // tie-breaker
    }
};
std::multiset<polynomial_, rank_less> F;  // multiset：rank 同内容可能不同
```

**代价**：
- 插入 R 需 O(\|R\| log \|F\|) 保持有序
- `multiset` 内存开销大于 `vector`

**收益**：
- `basset` 变为 O(\|F\| × n) 一遍扫描
- 但 `charset` 真正的瓶颈是伪除法 `prem`，不是 basset

#### 结论

理论可行，但实现复杂度高，且 `charset` 的瓶颈在伪除法循环而非 basset。**建议先实施 Issue 2 的扫描合并，Issue 3 作为后续深度优化考虑。**

**优先级**：低-中。需要 benchmark 验证实际收益。

---

### Issue 6: `std::set` 的 `operator<` 与 rank 序的一致性

> **发现日期**：2026-04-20
> **相关代码**：`clpoly/charset.hh:105`，`clpoly/basic_polynomial.hh:365`，`clpoly/basic.hh:312`
> **相关理论**：定义 6.2.1（王东明 p.136）

#### 问题描述

`charset` 使用 `std::set<polynomial_>` 存储多项式组 F。`std::set` 依赖 `polynomial_::operator<` 排序，而该运算符调用 `pair_vec_comp`，按**字典序**（lex order）比较多项式。

`basset` 需要找**rank 最低**的多项式，使用 `rankcompare`：
- 先比较类（`get_first_var`）
- 同类再比较导次数（`get_first_deg`）

#### 验证结果

**关键发现**：在 `lex_<custom_var_order>` 序下，`operator<` 的前缀比较与 `rankcompare` **完全一致**：

| 比较步骤 | `operator<`（字典序） | `rankcompare`（秩） |
|---------|---------------------|-------------------|
| 第一步 | 比较首项的最大变元 | 比较 `get_first_var`（类） |
| 第二步 | 比较该变元的次数 | 比较 `get_first_deg`（导次数） |

因为在 lex 序下，首项恰好包含最大变元 `get_first_var`，且该变元的次数就是 `get_first_deg`。

#### 严格验证

| 条件 | `rankcompare(F, G)` | `operator<(F, G)` | 是否一致 |
|------|---------------------|-------------------|---------|
| `cls(F) < cls(G)` | F rank 低 | F < G（F 在前）| ✅ |
| `cls(F) > cls(G)` | G rank 低 | G < F（G 在前）| ✅ |
| `cls(F) = cls(G)`, `ldeg(F) < ldeg(G)` | F rank 低 | F < G（F 在前）| ✅ |
| `cls(F) = cls(G)`, `ldeg(F) > ldeg(G)` | G rank 低 | G < F（G 在前）| ✅ |
| `cls(F) = cls(G)`, `ldeg(F) = ldeg(G)` | 相等 | 用内容区分 | 用内容区分 |

**结论**：`operator<` 的排序与 rank 序**完全一致**，不是"倾向于"，是**一定**。

#### 对算法的意义

这意味着 `std::set` 的 `begin()` **就是全局 rank 最低的多项式**！`basset` 可以优化：

```cpp
// 当前：遍历整个 set 找最小秩
std::size_t min = 0;
for (std::size_t i = 1; i < F.size(); ++i)
    if (rankcompare(F[i], F[min])) min = i;

// 优化：直接取 begin()
auto min_it = F.begin();  // 这就是 rank 最低的！
```

**注意**：`std::set` 的迭代器是 const 的，且 `basset` 需要修改 F。但至少"找最小秩"可以 O(1)。

#### 需要验证和测试

- [ ] 构造 rank 相同但内容不同的多项式，验证 `operator<` 能否正确区分
- [ ] 构造多组随机多项式，验证 `*set.begin()` 始终等于 `rankcompare` 找出的最小元
- [ ] benchmark：对比当前遍历实现 vs `*begin()` 直接取

**优先级**：中。如果验证通过，可以简化 `basset` 实现。

---

## cad.hh

### Issue 4: Lazard 投影算子尾系数优化

> **发现日期**：2026-04-20
> **相关代码**：`clpoly/cad.hh`（Lazard 投影算子实现）
> **相关理论**：Lazard (1994), McCallum (1998)

#### 背景

Lazard 投影算子（用于 CAD 的投影阶段）通常包含：
- 主系数（leading coefficient）
- 判别式（discriminant）
- 子结式（subresultant / PSC）
- **尾系数（trailing coefficient）**

#### 问题描述

在某些情况下，**尾系数可以从投影算子中省略**，而不会损失正确性。具体来说：

- 当多项式在投影变量上的次数为**奇数**时，尾系数在实根隔离中不影响符号变化
- 当多项式在样本点评估时，尾系数的零点已经被其他投影因子覆盖

#### 理论依据

Lazard (1994) 的原始投影算子包含尾系数，但后续研究表明：
- McCallum (1998) 证明了在某些条件下尾系数是冗余的
- Brown (2001) 的改进投影算子进一步减少了投影因子数量

#### 影响

- **正**：减少投影因子数量 → 减少后续实根隔离和样本点计算的开销
- **负**：需要严格的条件检查，错误省略会导致 CAD 不正确

#### 结论

需要仔细验证省略尾系数的**充分必要条件**，并在实现中加入条件判断。当前 CLPoly 的 CAD 实现应保留尾系数以确保正确性，后续可在理论验证后优化。

**优先级**：中。需要查阅 Lazard/McCallum/Brown 的原始论文确认条件。

---

### Issue 7: McCallum 投影算子系数集优化（结式截断）

> **发现日期**：2026-04-22
> **相关代码**：`clpoly/cad.hh:102-108`（`__project_mccallum_prims_to`）
> **参考实现**：ProjectionCAD `PCAD_McCcoeffset`（ProjectionCAD.mpl:1524-1553）
> **相关理论**：McCallum (1998), Brown (2001)

#### 问题描述

CLPoly 当前的 McCallum 投影算子取**所有非零系数**：

```cpp
auto coF = coeff(poly);
for (auto& c : coF) {
    if (!is_number(c))
        projs.push_back(std::move(c));
}
```

ProjectionCAD 的 `PCAD_McCcoeffset` 实现了**结式截断优化**：从低次到高次遍历系数，用 `resultant(crw, next_coeff, tvar)` 判断"独立性"——若结式为常数（无公共零点），则截断返回，省略后续系数。

#### 优化原理

McCallum 投影需要系数集的动机：确保多项式 $p(x_n, \bar{x})$ 在 $R^{n-1}$ 的每个 cell 上关于 $x_n$ 是 delineable。如果首项系数为零，次数下降，需要低次项信息。

**关键观察**：若 $a_i(\bar{x})$ 和 $a_j(\bar{x})$ 没有公共零点（$\text{res}(a_i, a_j) \neq 0$），则 $a_j$ 的零点集与 $a_i$ 不相交，$a_j$ 引入了新的零点，需要保留。反之，若 $\text{res} = 0$，则 $a_j$ 的零点已被覆盖，可省略。

#### ProjectionCAD 算法

```maple
cflist ← [coeff(poly,var,0), ..., coeff(poly,var,deg)]  // 从低次到高次
crw ← cflist[1]
for i from 2 to nops(cflist):
    tvar ← crw 中的一个变量
    res ← resultant(crw, cflist[i], tvar)
    if res = 0:       continue           // 不独立，跳过
    elif res 是常数:  return [cflist[1..i]]  // 独立，截断返回
    else:             crw ← res          // 更新，继续
```

#### 对 CLPoly 的影响

| | 当前 CLPoly | 优化后 |
|--|------------|--------|
| 系数数量 | 所有非零系数 | 可能显著减少 |
| 投影多项式数量 | 较多 | 较少 |
| 后续实根隔离开销 | 较大 | 较小 |

**注意**：此优化不影响正确性，只影响性能。需要验证：
1. 结式计算的开销是否被减少的投影多项式数量抵消
2. 对典型输入的实际收益

**优先级**：中。投影阶段性能优化，非 correctness 问题。

---

### Issue 5: OpenCAD 可采用 Brown 投影算子

> **发现日期**：2026-04-20
> **相关代码**：`clpoly/cad.hh`
> **相关理论**：Brown (2001) "Simple CAD Construction"

#### 背景

OpenCAD（开放柱形代数分解）只需要计算**开区域**（sectors）的样本点，不需要处理**截面**（sections）的代数样本点。这使得 OpenCAD 的投影阶段可以比完整 CAD 更轻量。

#### 问题描述

当前 CLPoly 的 OpenCAD 实现可能使用了**完整 CAD 的投影算子**（如 McCallum 或 Lazard），而实际上可以采用 **Brown (2001) 的简化投影算子**：

| 投影算子 | 适用场景 | 投影因子数量 |
|---------|---------|------------|
| McCallum | 完整 CAD | 较多 |
| Lazard | 完整 CAD | 较多 |
| **Brown** | **OpenCAD** | **较少** |

#### Brown 投影算子的优势

Brown (2001) 证明了对于 OpenCAD：
- 可以省略某些子结式条件
- 判别式的要求可以放宽
- 整体投影因子数量显著减少

#### 影响

- **正**：OpenCAD 的投影阶段更快，产生的细胞数更少
- **负**：Brown 投影算子**不适用于完整 CAD**（需要 Sections），必须严格区分 OpenCAD 和 Full CAD 的代码路径

#### 结论

CLPoly 当前有 `OpenCAD` 和 `Full CAD` 两条路径（根据上下文）。OpenCAD 路径应评估是否可采用 Brown 投影算子。这需要：
1. 实现 Brown 投影算子
2. 与当前 McCallum/Lazard 投影算子做正确性对比测试
3. Benchmark 验证性能提升

**优先级**：中-高。OpenCAD 是 CLPoly 的重要应用场景，投影阶段是主要瓶颈之一。

---

## 待验证

> 以下条目需要在后续代码审查或测试中验证

| # | 描述 | 相关代码/理论 | 优先级 |
|---|------|-------------|--------|
| 1 | `charset<Tc>` 算法与书中 CharSet/WuCharSet 的完全对应关系 | `charset.hh:99-138` vs 定理 6.2.3 / 定理 2.1 | 高 |
| 2 | `wrsd<Tc>` 的递归结构与书中 WRSD 算法的一致性 | `charset.hh:140-` vs 零点分解算法 | 高 |
| 3 | `is_regular<Tc>` 与正则列定义 6.4.1 的对应 | `charset.hh` vs 定义 6.4.1 | 中 |
| 4 | `ZDtoRC<Tc>` 与定理 6.4.7 的零点分解对应 | `charset.hh` vs 定理 6.4.7 | 中 |

---

> **维护说明**：每次发现新问题或验证完一个条目时更新此文件。已验证的问题可以移到"已解决"小节或删除。
