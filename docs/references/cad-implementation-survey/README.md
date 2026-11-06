# CAD 实现细节调研：技术报告、文档与源码

> 历史阅读辅助：保留原整理日期和观点，尚未逐条校正，不作为当前接口、算法选择或正确性结论。OCR、译述和页码以原文为准；当前复核见 [CAD 文档入口](../../cad/README.md)。

> **用途**：收集完整 CAD（含代数数样本点/sections）的实现细节资料
> **范围**：技术报告、实现文档、开源源码、闭源系统的公开文档
> **排除**：纯理论论文（已充分掌握）
> **整理日期**：2026-04-21

---

## 目录

- [第一优先级：高级语言的完整 CAD 实现](#第一优先级高级语言的完整-cad-实现)
  - [ProjectionCAD (Maple)](#projectioncad-maple)
  - [Maple RegularChains](#maple-regularchains)
  - [Mathematica](#mathematica)
  - [Reduce / Redlog](#reduce--redlog)
- [第二优先级：低级语言的完整 CAD 实现](#第二优先级低级语言的完整-cad-实现)
  - [QEPCAD / QEPCAD B](#qepcad--qepcad-b)
  - [SageMath](#sagemath)
  - [SymPy](#sympy)
- [第三优先级：三角列/正则链实现](#第三优先级三角列正则链实现)
- [第四优先级：实根隔离库](#第四优先级实根隔离库)
- [待获取资源与获取方式](#待获取资源与获取方式)
- [调研计划](#调研计划)

---

## 第一优先级：高级语言的完整 CAD 实现

高级语言实现（Maple/Mathematica）通常**文档最完整**，算法策略最清晰，是理解 CAD 实现细节的最佳入口。

---

### ProjectionCAD (Maple)

| 项目 | 信息 |
|------|------|
| **作者** | Matthew England, David Wilson, Russell Bradford, James H. Davenport (University of Bath, UK) |
| **系统** | Maple 第三方包 |
| **语言** | Maple |
| **源码** | 随论文发布，可下载 |
| **核心** | **经典的投影+提升（Projection & Lifting）框架**，与 Maple 内置的 RegularChains 方法形成对比 |

**关键特性**：
- **唯一实现 order-invariant CAD 的开源系统**（每个多项式在每个 cell 上消失的阶数恒定）
- **唯一实现 delineating polynomials**（修改提升阶段，使 McCallum 投影算子适用更广）
- 支持 equational constraints (ECCAD)、truth-table invariance (TTICAD)
- 支持 layered sub-CADs（只返回指定维度及以上的 cells）
- 支持 variety sub-CADs（只返回给定方程组为零的 cells）
- **使用 RegularChains Library 处理代数数运算和实根隔离**

**公开文档与源码**：

| 来源 | 内容 | 获取方式 |
|------|------|---------|
| *An implementation of CAD in Maple* (arXiv:1306.3062, 2013) | **ProjectionCAD V1**：Collins + McCallum 投影，sign/order-invariant CAD | https://arxiv.org/abs/1306.3062 |
| *An implementation of CAD in Maple utilising EC and TTICAD* (arXiv:1401.0645, 2014) | **ProjectionCAD V2**：equational constraints + TTICAD + 问题形式化启发式 | https://arxiv.org/abs/1401.0645 |
| *Using the RegularChains Library to build CADs by projecting and lifting* (ICMS 2014, arXiv:1405.6090) | **ProjectionCAD 架构**：如何用 RegularChains 的代数数运算支撑 PL-CAD | https://arxiv.org/abs/1405.6090 |
| *An implementation of Sub-CAD in Maple* (arXiv:1503.06599, 2015) | **ProjectionCAD V3**：layered/variety sub-CADs | https://arxiv.org/abs/1503.06599 |
| ICMS 2014 Slides | 实现细节：stack generation 调用 RegularChains 命令 | https://www.csd.uwo.ca/~mmorenom/Publications/England-ICMS-ProjectionCAD.pdf |
| 源码下载 | 随论文发布的 Maple 代码和工作表 | http://opus.bath.ac.uk/35636/ (V2), http://opus.bath.ac.uk/43911/ (V3) |

**与 CLPoly 缺口直接相关的关键信息**：

> "ProjectionCAD uses the RegularChains Library to create stacks of cells in the lifting phase. The main motivation: Uses efficient algorithms for triangular decomposition to compute with algebraic numbers."
> — ICMS 2014 Slides

> "Stack generation (step 29 in Algorithm 1) is achieved using an existing command from the RegularChains package, described fully in Section 5.2 of (Chen et al., 2009b). To use this we must first process the input to satisfy the assumptions of that algorithm: that polynomials are co-prime and square-free when evaluated on the cell (separate above the cell in the language of regular chains). This is achieved using other commands from the RegularChains library."
> — arXiv:1401.0645

**重点关注**：
- [ ] **Stack generation 调用 RegularChains 的具体命令**（MakeSemiAlgebraic 的子算法？）
- [ ] **代数数样本点的表示方式**：RegularChains 的 `rootof` 格式
- [ ] **多项式在代数数处求值**：RegularChains 的 `eval` 或 `Regularize` 命令
- [ ] **实根隔离**：RegularChains 的 `RealRootIsolate` 如何被调用
- [ ] **Delineating polynomials 的实现**：如何修改提升阶段避免 nullification 失败
- [ ] **源码结构**：Maple 代码包的组织方式

**为什么 ProjectionCAD 对 CLPoly 特别重要**：
1. **纯 PL-CAD 框架**：与 CLPoly 当前架构一致（投影+提升），而非 RegularChains 的复空间三角化路线
2. **代数数运算外包给 RegularChains**：CLPoly 可以借鉴这种"投影提升自己做，代数数运算用现有库"的架构
3. **源码可获取**：Maple 代码可读性强，可以直接分析提升阶段的算法细节
4. **order-invariant + delineating polynomials**：这些是 CLPoly 未来可能需要的高级特性

---

### Maple RegularChains

| 项目 | 信息 |
|------|------|
| **作者** | Changbo Chen, Marc Moreno Maza, Bican Xia, Lu Yang 等 |
| **系统** | Maple 内置/第三方库 |
| **核心** | **复空间三角化 → 实空间精化** 的 CAD 路线（与 ProjectionCAD 互补） |
| **理论基础** | Chen-MM-Xia-Yang ISSAC 2009（[本地分析](projectioncad/CAD-via-Triangular-Decomposition-ISSAC2009-中文详解.md)） |

**公开文档**：

| 来源 | 内容 | 获取方式 |
|------|------|---------|
| Maple Help: `CylindricalAlgebraicDecomposition` | 命令文档，算法选项（incremental/recursive） | https://www.maplesoft.com/support/help/ |
| Maple Help: `SemiAlgebraicSetTools` | 半代数系统工具 | 同上 |
| *Computing CAD via Triangular Decomposition* (ISSAC 2009, Chen-MM-Xia-Yang) | **奠基论文**：定义 `TRDgenerate_stack` 与 `TRDisolate_real_zeros` 算法 | PDF（可选本地附件：`projectioncad/CAD-via-Triangular-Decomposition-ISSAC2009.pdf`） / [arXiv:0903.5221](https://arxiv.org/abs/0903.5221) / [本地分析](projectioncad/CAD-via-Triangular-Decomposition-ISSAC2009-中文详解.md) |
| *Triangular Decomposition of Semi-Algebraic Systems* (ISSAC 2010, Chen-Davenport-MM-Xia) | 半代数系统的三角分解理论 | 论文 |

**重点关注**：
- [ ] `CylindricalAlgebraicDecompose` 的 `method=incremental` vs `method=recursive`
- [ ] `RealRootIsolate`：代数数系数多项式的隔离方法
- [ ] `RepresentingChain`：正则链的数据结构

---

### Mathematica

| 项目 | 信息 |
|------|------|
| **作者** | A. Strzebonski, Wolfram Research |
| **系统** | Mathematica (商业 CAS) |
| **核心函数** | `CylindricalDecomposition`, `Reduce`, `Resolve` |

**公开实现细节来源**：

| 来源 | 内容 | 获取方式 |
|------|------|---------|
| *Cylindrical Algebraic Decomposition Using Validated Numerics* (J. Symb. Comput. 2006) | **Strzebonski 核心论文**：数值验证与符号 CAD 结合 | 学术数据库 |
| Mathematica Documentation | `CylindricalDecomposition` 算法描述 | https://reference.wolfram.com/ |

**重点关注**：
- [ ] Strzebonski 2006：数值验证如何与符号 CAD 结合
- [ ] 代数数样本点的混合处理策略

---

### Reduce / Redlog

| 项目 | 信息 |
|------|------|
| **作者** | T. Sturm, A. Dolzmann, V. Weispfenning |
| **系统** | Reduce (开源 CAS) + Redlog (包) |
| **源码** | Reduce 开源，Redlog 随 Reduce 分发 |

**公开文档**：

| 来源 | 内容 | 获取方式 |
|------|------|---------|
| Redlog User Manual | 完整命令参考，算法选项 | https://redlog.dolzmann.de/download/ |
| *Redlog: Computer Logic Meets Computer Algebra* (1999) | 系统介绍 | 论文 |

---

## 第二优先级：低级语言的完整 CAD 实现

低级语言实现（C/C++）更接近工程实际，但文档通常较少，需要阅读源码。

---

### QEPCAD / QEPCAD B

| 项目 | 信息 |
|------|------|
| **作者** | C.W. Brown (US Naval Academy) |
| **语言** | C |
| **官网** | https://www.usna.edu/Users/cs/wcbrown/qepcad/B/QEPCAD.html |
| **源码** | 官网可下载 tarball |
| **核心算法** | 完整 CAD，支持 McCallum/Brown/Hong 投影 |

**实现细节关注点**：
- [ ] 样本点表示：代数数 vs 有理数区间？
- [ ] 提升阶段（lifting）的代码路径
- [ ] 实根隔离的后端调用
- [ ] `ASSIGN` 函数：多项式在样本点处的求值
- [ ] `ROOTS` 函数：单变量多项式实根隔离

**获取方式**：
```bash
wget https://www.usna.edu/Users/cs/wcbrown/qepcad/B/INSTALL/qepcad-B.1.74.tar.gz
```

---

### SageMath

| 项目 | 信息 |
|------|------|
| **作者** | Sage 社区 |
| **语言** | Python (封装 C/C++) |
| **源码** | https://github.com/sagemath/sage |
| **核心** | 封装 QEPCAD，部分原生实现 |

---

### SymPy

| 项目 | 信息 |
|------|------|
| **作者** | SymPy 社区 |
| **语言** | Python |
| **源码** | https://github.com/sympy/sympy |
| **核心** | 纯 Python 实现，含 CAD、Groebner 等 |

**关注点**：`sympy.polys.numberfields`, `sympy.polys.rootisolation`

---

## 第三优先级：三角列/正则链实现

| 项目 | 作者 | 语言 | 源码 |
|------|------|------|------|
| **BPAS** | Moreno Maza 团队 (UWO) | C++ | https://github.com/orcca-uwo/BPAS |
| **Epsilon** | D.K. Wang | Java | http://www-polsys.lip6.fr/~wang/epsilon/ |

---

## 第四优先级：实根隔离库

| 项目 | 作者 | 语言 | 核心 |
|------|------|------|------|
| **RS** | F. Rouillier | C | 实根隔离，基于 Descartes 法则 |
| **FLINT / Arb** | W. Hart, F. Johansson | C | 任意精度算术，多项式根隔离 |

---

## 待获取资源与获取方式

### 立即获取（今天）

| 资源 | 获取命令 | 大小 |
|------|---------|------|
| ProjectionCAD 论文 (arXiv) | `wget https://arxiv.org/pdf/1405.6090.pdf` | ~200KB |
| ProjectionCAD V2 源码 | `wget http://opus.bath.ac.uk/35636/` | 需确认 |
| QEPCAD B 源码 | `wget https://www.usna.edu/Users/cs/wcbrown/qepcad/B/INSTALL/qepcad-B.1.74.tar.gz` | ~2MB |
| BPAS 源码 | `git clone https://github.com/orcca-uwo/BPAS.git` | ~50MB |
| SymPy 源码 | `git clone https://github.com/sympy/sympy.git` | ~100MB |

### 1-2 周内获取

| 资源 | 获取方式 |
|------|---------|
| Strzebonski 2006 论文 | 学术数据库（J. Symb. Comput.） |
| Maple RegularChains 文档 | https://www.maplesoft.com/support/help/ |
| Redlog User Manual | https://redlog.dolzmann.de/download/ |

---

## 调研计划

### 阶段 1：ProjectionCAD 深度分析（1-2 周）

**目标**：理解 PL-CAD 框架下如何用 RegularChains 处理代数数，这是 CLPoly 最直接的参考。

**任务**：
1. 下载 ProjectionCAD V2/V3 源码（Maple 代码）
2. 阅读 arXiv:1405.6090（架构论文）和 arXiv:1401.0645（V2 功能）
3. 定位并分析 lifting 阶段的 Maple 代码：
   - 如何调用 RegularChains 生成 stack
   - 样本点表示（`rootof` 格式）
   - 多项式在样本点处的求值流程
4. 记录关键数据结构和算法流程

**产出**：ProjectionCAD 提升阶段分析报告。

### 阶段 2：QEPCAD 源码分析（2-3 周）

**目标**：理解 C 语言层面的工程实现细节。

**任务**：
1. 下载并编译 QEPCAD
2. 追踪源码关键路径：
   - `source/lift/`：提升阶段
   - `source/sample/`：样本点管理
   - `source/root/`：实根隔离
3. 对比 ProjectionCAD 和 QEPCAD 的样本点表示差异

### 阶段 3：补充实现（1-2 周）

**任务**：
1. 分析 BPAS 的正则链和实根隔离模块
2. 了解 RS / FLINT 的实根隔离接口
3. 整理 CLPoly 的实现方案

---

## 当前 CLPoly 的缺口与调研目标

| 缺口 | 调研目标 | 优先级 | 来源 |
|------|---------|--------|------|
| 代数数样本点表示 | ProjectionCAD 如何用 RegularChains 的 `rootof` 表示样本点 | **最高** | ProjectionCAD |
| 多项式在代数数处求值 | ProjectionCAD 调用 RegularChains 的哪个命令求值 | **最高** | ProjectionCAD |
| 代数数系数多项式的实根隔离 | RegularChains `RealRootIsolate` 的调用方式 | **最高** | ProjectionCAD + RegularChains |
| 三角列商环运算 | BPAS / Maple RegularChains | 中 | 第三优先级 |
| 完整 CAD 的 Sections | QEPCAD 的 lifting 代码 | 高 | QEPCAD |
| Delineating polynomials | ProjectionCAD 如何避免 nullification | 中 | ProjectionCAD |
