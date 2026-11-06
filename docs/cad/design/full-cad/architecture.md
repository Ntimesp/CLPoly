# 完整 CAD 架构

数据结构：`full_cad_result.hh`；独立算法：`full_cad.hh`，依赖 `lazard_qq.hh` 和独立实点模块。

流程：验证输入 → Lazard/FACTOR 投影 → 每父样本计算 QQ Lazard 代表 → 纤维根隔离 → 完整 Section/Sector 栈 → 原输入叶符号。

`full_cad_result` 拥有投影比较器、既有 `cad_tree`、每节点 `real_algebraic_point`、每父栈的根/来源/valuation、维数及符号表。坐标按提升序；外部参数沿用投影序。

样本表示、层间接口和错误语义详见 [模块总架构](../modular-cad/architecture.md)、[Lazard/CAD 细化](../modular-cad/lazard-and-cad.md)、[公共 API](api.md)。

`__lift_stack` 完成 QQ 乘积、根隔离、样本选择和来源判定；`full_cad_result` 只提供查询及节点/附属表追加操作。构建算法负责维持父节点顺序和完整子块不变量。

Sector 采样委托给独立的 `rational_sample.hh::sample_rational`，依赖仅为 QQ 和标准库。根盒精化仍由提升算法负责，采样器只读取认证后的 QQ 开端点；详细契约和连分数证明见[采样设计](../rational-sampling.md)。Section 直接保留原根证书。
