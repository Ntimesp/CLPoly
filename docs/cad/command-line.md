# 用命令行求解题集公式

在仓库根目录执行：

```sh
make cad
_build/release/bin/clpoly-cad examples/cad/sat.txt
# SAT
_build/release/bin/clpoly-cad examples/cad/unsat.txt
# UNSAT
```

程序是原生 C++ 可执行文件，运行只使用 CLPoly 的既有依赖，不需要 Python 或 Z3。当前构建产物位于 `_build/release/bin/clpoly-cad`；重新构建时沿用项目的 `config.mk` 配置。

## 文件格式

沿用题集约定，空行分成三段。例如 [sat.txt](../../examples/cad/sat.txt)：

```text
x

x^2 - 2
x

[0,=]
[1,>]
```

第一段是变量表，也就是投影顺序；第二段每行一个多项式，从 0 编号；第三段每行一个子句。`[i,=]`、`[i,>]`、`[i,<]` 分别表示第 i 个多项式等于、大于、小于零。同一行用 `|` 连接表示“或”，不同行表示“且”。上例要求 `x²=2` 且 `x>0`，答案为 SAT。

多项式支持任意精度整数、精确有理系数、`+ - * /`、括号及 `^`／`**` 非负整数幂。乘号必须显式书写；除数必须为非零有理常量。小数请写成整数比值，例如 `1/10`；变量必须预先声明。接受 UTF-8 文本、LF／CRLF 和开头 BOM。每段均需有内容，常量真／假可写 `0` 多项式加 `[0,=]`／`[0,>]`。

## 输出和选项

```sh
# 输出一个精确解的定义链与隔离盒
_build/release/bin/clpoly-cad --sample examples/cad/sat.txt

# 仅检查语法、变量和索引，成功输出 VALID
_build/release/bin/clpoly-cad --check question.txt

# 反转声明的投影变量序
_build/release/bin/clpoly-cad --reverse question.txt

# 从标准输入读取
_build/release/bin/clpoly-cad - < question.txt

# 查看帮助
_build/release/bin/clpoly-cad --help
```

默认标准输出只有一行 `SAT`（有实数解）或 `UNSAT`（无实数解）。题目中的候选答案、界或反例已经编码在公式中，程序回答这个公式是否成立于某个实数赋值，不从题目文本自动计算另一个未指定的目标值。

`--sample` 的输出示例：

```text
SAT
sample_variables: x
equations:
x^3-2*x = 0
isolating_intervals:
x in (5/4, 3/2)
shifts: 4
```

定义链可能是多个投影因子的乘积，并不要求与原输入方程相同。链与隔离盒共同指定唯一实点；这里是正根 √2。多变量证书按输出的 `sample_variables` 顺序解释，该顺序是实际提升顺序。隔离区间不是把解截断成小数的答案；`shifts` 保存复核证书时所用的精确平移。

正常答案或检查成功退出码为 0；命令行、文件或格式错误为 2；求解异常为 1。诊断写入标准错误，错误不会被当成 UNSAT。`--check` 与 `--sample` 不能合用。

程序直接调用完整 CAD，输入规模和变量序可能显著影响时间。当前版本不启用题集的实验线性预处理，也不设置内部超时。历史完成率和耗时见[历史实验](reports/three-datasets.md)，不能当成本程序当前版本的性能数据。

## 测试与实现依据

```sh
make test-cad-cli
```

该命令运行 C++ 解析／布尔单元测试与 Python 标准库驱动的可执行文件测试，覆盖原题集 72 个 TXT 的读取、8 个原查询的求解、SAT／UNSAT、精确样本、标准输入、反序和非法输入。解析器也注册在整库 C++ 回归中。

输入结构与算法分离，工具通过库公开入口完成检查和换序；不修改投影或提升算法。[实现设计](design/command-line.md)给出语法、契约及基于完整 CAD 覆盖和符号不变性的判定论证。

本次构建、测试及证书预期修正见[验证记录](reports/command-line.md)。
