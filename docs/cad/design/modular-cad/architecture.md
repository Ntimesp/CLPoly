# 模块架构与算法接口

实代数点由有理三角列、QQ 根隔离盒和固定平移证书表示。QQ 保持现有分子分母表示。库中存储的多项式系数为 QQ 或 ZZ；无理坐标只出现在数学解释中。

## 分层及依赖

| 层 | 文件 | 数据 | 算法及依赖 |
|---|---|---|---|
| 精确数 | `number/ZZ.hh`、`number/QQ.hh` | 整数与有理数 | 可信整数、有理数算术 |
| 有理采样 | `rational_sample.hh` | 调用方提供的 QQ 开端点，可缺失 | `sample_rational`，独立连分数算法，仅依赖 QQ 和标准库 |
| 区间 | `rational_interval.hh` | 两个 QQ 端点 | 基础区间算术与集合操作，仅依赖 QQ 和标准库 |
| 多项式估值 | `polynomial_interval.hh` | 调用方提供的有理盒 | `assign_interval`，连接多项式和区间 |
| 零维正则链 | `regular_chain.hh` | 外部序指针、QQ 方程、平方自由性质 | `weak_split_zero`、`squarefree_decomposition`，复用 `charset.hh` |
| 实代数点 | `real_algebraic.hh` | 三角列、QQ 根盒、固定平移 | 隔离、精化、判零、符号、同点判定和纤维求根 |
| Lazard | `lazard_qq_result.hh`、`lazard_qq.hh` | QQ 代表及 valuation | `prefix_zero`、`lazard_evaluate_qq`，依赖实点查询 |
| CAD 存储 | `full_cad_result.hh` | 树、样本、根栈、维数及符号 | 查询和追加存储操作 |
| CAD 算法 | `full_cad.hh` | 使用 CAD 存储接口 | `__lift_stack`、`full_cad`，复用可信投影 |

多项式区间求值单独设计、测试和审核，见[设计](../polynomial-interval/design.md)。区间与三角列是并列基础模块；只有实代数点层组合三角列和区间证书。CAD 调用实点公共接口，不修改实点内部根盒。

有理采样与闭区间算术也是独立模块，见[采样设计](../rational-sampling.md)。CAD 在已分离根盒提供的 QQ 开间隙中选样本；采样器不接收三角列、根证书或无理数，不负责根精化。

## 算法与数据结构分离

数据类型负责表示、所有权、只读查询及存储不变量。构造器校验给定数据的有效性；区间值类型提供基础算术。三角列分解、实根隔离、精化、判零及 CAD 提升由独立函数实现。

三角列分解对外部多项式采用公开入口与 `__` 内部入口：`weak_split_zero` / `__weak_split_zero`。公开入口检查外部多项式并转换到链的 QQ 字典序；内部入口要求调用者保证比较器和变量覆盖，仅使用断言，不重复执行公开构造器的完整检查。`squarefree_decomposition` 直接接收有效正则链，完成系数适配与平方自由分解，采用单一入口。输出装配按值保存方程并借用外部比较器；未知的平方自由属性仍需计算，文献已保证的属性直接保存。详见[三角列规格](regular-chain.md)。

`real_algebraic_point` 保存证书。自由函数 `shrink(a)` 与 `refine(a,epsilon)` 实现 NSHR-QQ / REFINE；同名成员函数仅转发。`shrink` 负责修改既有点的私有根盒，保持定义链、固定平移及所选实点不变。内部 `__lift_real_points` 仅组合已经认证的前缀和新层 XZ-LIFT 证据来构造新点；它不改变输入点的语义，不向公开构造提供绕过检查的入口。构造检查不允许任意含根宽盒冒充有效精化证书。

`full_cad_result` 组合可信 `cad_tree`，其 `add_sector` / `add_section` 同步追加节点、样本和维数，`add_stack` 追加父栈，`set_signs` 保存叶符号。这些操作不执行求根、精化或样本选择。`__lift_stack` 和 `full_cad` 通过这些接口构建结果，遵循 `open_cad` / `cad_tree` 的算法与容器分离原则。

职责分离不要求每个类型独占一个头文件。`full_cad_result.hh` 不包含 `cad.hh` 或 `lazard_qq.hh`；Lazard 结果的值类型单独放在 `lazard_qq_result.hh`。实点证书依赖仍通过实点头提供。

## 所有权与表示不变量

- 三角列只保存外部比较器的地址；方程和单项式共用该地址，复制与移动使用默认成员操作。调用者保证比较器比全部链、转换结果和实代数点活得更久，且借用期间不改变变量序。`convert`、`integer_equations` 的输出不依赖原链继续存活。完整细化见[三角列规格](regular-chain.md)。
- 点包含自己的三角列、严格隔离盒及固定平移。前缀构造建立准确的变量表，并保持所选坐标语义。
- Lazard 结果拥有代表多项式的比较器，不拥有输入点。代表与 valuation 必须相对于调用方保留的同一实点解释。
- CAD 结果共享拥有投影比较器；投影多项式重绑定到该比较器。样本证书、根栈三角列与投影共用结果持有的比较器；单独复制出的样本仍要求该比较器保持存活。Lazard 结果另行持有代表多项式的比较器。
- 公共 CAD 输入采用投影顺序；树层、三角列坐标和根盒采用反向的提升顺序。例如 CAD 输入 `{x1,x2,x3}`，提升和三角列坐标为 `{x3,x2,x1}`，三角列比较器的优先级恢复为 `{x1,x2,x3}`。主变量始终取当前比较器下最小的变量。

## CAD 存储契约

按层及父节点存储顺序构建，每个父节点的子节点必须组成连续完整块。节点、样本和维数数组下标一致；每层根栈表按父节点下标索引。第零层只有一个虚拟父节点，以 `SIZE_MAX` 表示。

根边界 `cad_root(p,r)` 保留来源多项式 p 与该来源的不同根序号 r，通过 `root(level,parent,reference)` 解析到父栈证书。每个来源独立计数，合并根表的下标与 r 不可混用。其证书描述样本纤维上的点；整个 Section 的语义由投影定理给出的有序连续根函数确定。底层追加接口要求算法保证这些条件，不自行证明样本属于胞腔。

## 规格与证明依赖

三角列分解依据 XY16；实点隔离和精化依据 XZ06；XY02 为补充；完整 CAD 的投影与 delineability 使用 MPP 的明确契约。编号伪代码与证明见[理论 PDF](../../theory/lazard-evaluation.pdf)，区间算术见[区间规格](../interval-arithmetic/design.md)。

实现审查按“前提 → 算法步骤 → 循环不变量 → 终止性 → 测试证据”进行。控制流显式展开；注释解释数学意义、所有权和算法编号。测试证据集中于[验证报告](../../reports/modular-cad.md)，机器精化义务集中于[形式化接口](formalization.md)。
