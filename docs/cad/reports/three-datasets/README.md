# CAD 题集验证工具

本目录保存精确题集适配、线性等式预处理原型、独立见证检查和临时插桩工具。库的公共入口仍是 `clpoly.hh` 中的 `full_cad`；公式处理原型尚未成为公共 C++ API。预处理规格见[设计](../../design/formula-preprocessing.md)。

## 当前工具与历史证据

[三数据集报告](../three-datasets.md)及本目录已有 JSON、CSV、三个归档均为 2026-10-22 的历史实验，不代表当前库的完成数或耗时。`build-snapshot.tar.gz` 内保存原始生产代码、所链接库及五个工具的原文；`input-snapshot.tar.gz` 保存输入，`profile-evidence.tar.gz` 保存诊断。`artifact-sha256.json` 保持原值：五个工具的哈希应核对构建归档内同路径文件，其他条目核对本目录文件。`test_tools.py` 自动执行此检查，并逐项核对原始清单中 145 个输入、生产源码及静态库条目与归档内容的哈希。

目录中的同名工具维护为当前版本。2026-11-06 复核修正：

- `driver.cc` 在库调用前检查读取成功、非负计数、整数系数、非零分母、指数范围、原子引用与关系、尾随内容和命令行参数。原版本的无符号读取不能可靠拒绝负数，越界原子索引也未经检查。
- `linear_preprocess.py` 检查公式结构并合并重复项。主元选择、精确代换、布尔化简和恢复映射保持原设计；参考 `z3_status` 仅为可选元数据，不参与决策或算法断言。原版本断言将确定结果与参考标签绑定，不能作为算法正确性的理由。
- `run.py` 的文本输入检查使用异常，`python -O` 下也生效。Z3 只在本工具中用于独立对照与等价检查。
- `profile.py` 对接当前 `__` 字典序内核，插桩替换要求唯一完整行匹配，允许整体缩进变化。只写临时头文件；退出时停止并 `join` 监控线程，避免原分离线程访问已析构状态。该工具面向当前串行 CAD，不支持同时执行多个求解线程。
- `check_witnesses.py` 保持独立有理数／多二次根式运算，不依赖 CLPoly 或 Z3；解析及见证检查不因 Python 优化而关闭。

## 构建和验证

需要项目正常构建环境；Z3 对照另需 Python 的 `z3-solver`。不安装 Z3 时，标准库测试仍可运行，对照项明确跳过。C++ 输入测试由环境变量指定驱动，不指定时明确跳过。

```sh
make
mkdir -p /private/tmp/clpoly-cad-tools
g++-13 -std=c++17 -O2 -DNDEBUG -I. -I/opt/homebrew/opt/gmp/include -I/opt/homebrew/opt/boost/include docs/cad/reports/three-datasets/driver.cc lib/debug/libclpoly.a -L/opt/homebrew/opt/gmp/lib -lgmpxx -lgmp -o /private/tmp/clpoly-cad-tools/driver
CLPOLY_CAD_DRIVER=/private/tmp/clpoly-cad-tools/driver python3 -O docs/cad/reports/three-datasets/test_tools.py
```

路径和编译器按本机配置调整。测试覆盖空公式／空子句、析取保护、变量主元禁止、负系数、矛盾、恢复映射、重复项、非法输入、根式运算和历史哈希。72 个归档题例各核对 `F(R(u)) ⇔ G(u)` 和 `F(x) ⇒ x=R(x|U)`，共 144 个 Z3 不可满足检查；另独立验算 25 个 SAT 见证。这些有限检查补充设计中的恒等式与主元归纳证明。

复现实验时请使用新的输出目录，勿覆盖本目录历史数据：

```sh
python3 docs/cad/reports/three-datasets/run.py --stage prepare --data /path/to/datasets --out /private/tmp/clpoly-cad-new-run
# 将上面构建的 driver 放入输出目录，再执行：
python3 docs/cad/reports/three-datasets/run.py --stage run --out /private/tmp/clpoly-cad-new-run --timeout 10
python3 docs/cad/reports/three-datasets/check_witnesses.py --out /private/tmp/clpoly-cad-new-run
python3 docs/cad/reports/three-datasets/profile.py --out /private/tmp/clpoly-cad-new-run --timeout 10
```

`prepare` 独立核对 TXT／模型并生成 `.cad` 输入、预处理结果及清单；`run` 记录原式、残余式、两种变量序和外部超时；`summarize` 只汇总已有结果。诊断应在正式计时结束后运行，插桩耗时不作为生产基准。超时不代表错误答案，也不代表已完成求解。当前版本验证记录见[上层复核](../uncommitted-review.md)，历史完成率仅见历史报告。
