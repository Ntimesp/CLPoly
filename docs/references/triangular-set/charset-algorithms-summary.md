# CLPoly charset.hh 算法功能对照表

本文是历史对照笔记，以下旧函数名和页码不作为当前 API 规格。新增三角列包装器使用 WRSD，见[三角列设计](../../cad/design/modular-cad/regular-chain.md)；原有 RSD 实现和测试保留，相关反例、调用边界和文献条件单列于[RSD 独立说明](../../cad/fixes/rsd-nonradical-prefix.md)。

> **来源**：`clpoly/charset.hh` + 理论文献对照
> **整理日期**：2026-04-21

---

## 辅助函数

| 函数名 | 输入 | 输出 | 作用 | 理论来源 |
|--------|------|------|------|---------|
| `rankcompare` | 两个多项式 F, G | bool | 判断 F 的秩是否低于 G（先比类，再比次数） | 定义 6.2.1（王东明 p.136） |
| `is_reduced` | 两个多项式 F, G | bool | 判断 F 关于 G 是否约化（F 中 G 的导元次数 < G 的导次数） | 定义 6.1.1（王东明 p.134） |
| `sqrfree` | 多项式(组) | 去平方后的多项式(组) | 去掉系数公因子，去除平方因子 | 标准代数预处理 |

---

## 核心算法

| 函数名 | 输入 | 输出 | 作用 | 理论来源 | 与 CLPoly 其他模块的关系 |
|--------|------|------|------|---------|------------------------|
| **basset** | 多项式组 P | 基列 B（三角列） | 从 P 中选取秩最低的基列，逐步筛选约化多项式 | 算法 BasSet（王东明 p.138） | 被 `charset` 调用 |
| **charset** | 多项式组 P | 特征列 C | 吴-Ritt 特征列算法：反复计算基列和伪余式，直到所有余式为 0 | 算法 CharSet（王东明 p.138-139）/ 定理 6.2.3 | 调用 `basset` 和 `prem` |
| **wrsd** | 三角列 T，多项式 f | (H, G) 两个正则链集 | **弱正则子结式分解**（WRSD）：将 f 分解为对 T 整式依赖部分 H 和互素部分 G | wr 算法（Yang et al. 1995）/ 定义 2.13（Xia & Yang 2016 p.33） | 被 `ZDtoRC` 和 `rsd` 调用 |
| **rsd** | 三角列 T，多项式 f | (H, G) 两个正则链集 | **正则子结式分解**（RSD）：比 WRSD 更强的分解，使用子结式链 | 算法 RSD（Xia & Yang 2016 p.35） | 调用 `wrsd` 和 `subreschain` |
| **is_regular** | 三角列 T | bool | 检查 T 是否为正则列（所有初式的结式非零） | 定义 6.4.1（王东明 p.147）/ 定义 2.3（Xia & Yang 2016 p.27） | 被 `ZDtoRC` 隐式使用 |
| **ZDtoRC** | 三角列 T | 正则链集 G | **零维三角列到正则链**：将非正则三角列分解为多个正则链 | 算法 2.3（Xia & Yang 2016 p.38）/ 定理 2.7 | 调用 `wrsd`，被 `GRDforZD` 调用 |
| **Wucharset** | 多项式组 P | 特征序列 {C₁, C₂, ...} | **吴特征序列**：计算特征列后，对每个初式递归分解，得到完整的零点分解 | 零点分解算法（王东明 p.140-145）/ 定义 6.3.1 | 调用 `charset`，被 `GRDforZD` 调用 |
| **GRDforZD** | 多项式组 P | 正则链集 T | **零维通用正则分解**：结合吴方法和 ZDtoRC，将多项式组分解为正则链 | 算法 2.4（Xia & Yang 2016 p.39） | 调用 `Wucharset` 和 `ZDtoRC` |
| **RCtoSqrfree** | 正则链 T | 无平方正则链集 TT | 将正则链转化为无平方正则链（去掉平方因子） | 简单列/无平方系统（王东明 p.149） | 被 `GRDforZDSAS` 调用 |
| **GRDforZDSAS** | 半代数系统 S = [P, G₁, G₂, H] | 半代数系统的正则链分解 | **零维半代数系统的通用正则分解**：处理带不等式的系统 | 扩展的 GRD 算法 | 调用 `GRDforZD` 和 `RCtoSqrfree` |

---

## 算法调用关系图

```
用户输入 P
│
├─► charset(P) ──► 特征列 C
│   └─► basset(F) ──► 基列 B
│   └─► prem(f, C) ──► 伪余式
│
├─► Wucharset(P) ──► 特征序列 {C₁, C₂, ...}
│   └─► charset(P)
│   └─► 对每个初式 Iᵢ：Wucharset(P ∪ {Iᵢ})
│
├─► GRDforZD(P) ──► 正则链集
│   └─► Wucharset(P)
│   └─► 对每个 Cᵢ：ZDtoRC(Cᵢ)
│       └─► 找最大正则前缀 Tₖ
│       └─► wrsd(Tₖ, lc(Tₖ₊₁)) ──► (H, G)
│           └─► 子结式链计算
│       └─► 递归 ZDtoRC(R ∪ Tₖ₊₁...)
│
└─► GRDforZDSAS(S) ──► 半代数系统分解
    └─► GRDforZD(P)
    └─► RCtoSqrfree(T)
    └─► 检验不等式
```

---

## 输入输出格式速查

| 函数 | 输入类型 | 输出类型 | 说明 |
|------|---------|---------|------|
| `basset` | `vector<polynomial>` | `vector<polynomial>`（三角列） | 单个基列 |
| `charset` | `vector<polynomial>` | `vector<polynomial>`（特征列） | 单个特征列，矛盾列为 `[c]` |
| `wrsd` | `vector<polynomial>`（三角列）, `polynomial` | `pair<vector<vector<polynomial>>, vector<vector<polynomial>>>` | (H, G)，H=整式依赖，G=互素 |
| `rsd` | `vector<polynomial>`（三角列）, `polynomial` | 同上 | 比 wrsd 更强的分解 |
| `ZDtoRC` | `vector<polynomial>`（三角列） | `vector<vector<polynomial>>`（正则链集） | 分解非正则三角列 |
| `Wucharset` | `vector<polynomial>` | `vector<vector<polynomial>>`（特征序列） | 完整的零点分解 |
| `GRDforZD` | `vector<polynomial>` | `vector<vector<polynomial>>`（正则链集） | 零维系统的正则分解 |
| `GRDforZDSAS` | `vector<vector<polynomial>>`（4个部分） | `vector<vector<vector<polynomial>>>` | 半代数系统分解 |

---

## 与理论文献的完整对应

| CLPoly 函数 | 王东明《计算机代数》 | Xia & Yang 2016 | 其他文献 |
|------------|---------------------|-----------------|---------|
| `basset` | 算法 BasSet (p.138) | — | — |
| `charset` | 算法 CharSet (p.138-139) | WuCharSet | 定理 2.1 (p.29) |
| `wrsd` | — | 定义 2.13 (p.33) | wr 算法 (Yang et al. 1995) |
| `rsd` | — | 算法 RSD (p.35) | — |
| `is_regular` | 定义 6.4.1 (p.147) | 定义 2.3 (p.27) | Kalkbrener 1993 |
| `ZDtoRC` | — | 算法 2.3 (p.38) | 定理 2.7 |
| `Wucharset` | 零点分解 (p.140-145) | — | — |
| `GRDforZD` | — | 算法 2.4 (p.39) | — |
| `RCtoSqrfree` | 定义 6.4.2 简单列 (p.149) | — | Wang 2000 |
| `GRDforZDSAS` | — | — | Xia & Yang 2002 |
