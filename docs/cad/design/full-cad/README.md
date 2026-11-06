# 完整 CAD 阅读入口

完整 CAD 采用 QQ 代表多项式、XY16 三角列和 XZ06 实点隔离/精化。结果存储在 `full_cad_result`，算法入口为 `full_cad`。

- [模块总设计与阅读顺序](../modular-cad/README.md)
- [公共 API](api.md)
- [理论 PDF](../../theory/lazard-evaluation.pdf)
- [逐算法实现对应与证明要点](detailed-design.md)
- [验证报告](../../reports/modular-cad.md)

[架构](architecture.md)定义数据和算法边界；[详细设计](detailed-design.md)给出投影、提升与胞腔语义；[形式规格](formal-specification.md)说明正确性目标及机器证明义务。
