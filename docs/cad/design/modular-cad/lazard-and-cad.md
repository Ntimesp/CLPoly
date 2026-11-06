# QQ Lazard 求值的接口与证明

本功能依赖[实代数点](real-algebraic.md)，以 MPP Definition 2.6、Remark 2.9 的 Lazard 求值和最小字典序 Taylor 系数为数学定义。原文见[MPP 论文](../../../references/lazard/McCallum-Parusinski-Paunescu-Lazard-validity-1607.00264v2.pdf)。

## 数据与算法分离

`lazard_qq_result.hh` 只定义结果值类型：共享拥有不可变字典序比较器，保存借用它的 QQ 多项式以及按提升顺序排列的 valuation。构造器拒绝空比较器。复制结果保留比较器所有权；使用代表期间不能单独重置 order 或改绑 polynomial。

结果不持有实点，不能脱离生成它的前缀点解释，也不能换用另一个共轭点。实点及其外部比较器由调用方保持有效。全部求值算法位于 `lazard_qq.hh` 的自由函数中。

## 公开入口与字典序内核

| 入口 | 检查与效果 |
|---|---|
| `prefix_zero(p,prefix,active)` | 接受一般序 ZZ/QQ；检查 active 无重复、与冻结坐标不交，覆盖 p 的变量且指数非负；转换到活动变量优先、冻结变量保持链中相对序的 QQ 字典序 |
| `__prefix_zero(p,prefix,active)` | 输入已规范化 QQ 字典序多项式；活动变量优先于全部冻结变量，冻结变量相对序与链一致；只用 assert，不重新校验或转换 |
| `lazard_evaluate_qq(f,lifting_order,sample)` | 检查 f 非零、提升坐标唯一且恰为样本前缀加一个变量，输入只含声明变量且指数非负；创建拥有比较器的 QQ 代表并转入内核 |
| `__lazard_evaluate_qq(result,lifting_order,sample)` | result 初始含非零 QQ 代表和空 valuation，比较器与代表一致并按提升坐标的反序排列；就地提取 QQ 代表和 valuation；合法性由调用者保证，只使用断言 |

一般入口保留 Release 检查：非法变量／坐标抛 invalid_argument，零求值输入抛 domain_error。内部断言用于诊断调用契约，不是用户输入校验。输出只含 QQ 系数；不在运行时建立代数系数域。

## 字典序系数分组

活动变量都排在冻结变量之前，所以规范化字典序中的同一完整活动单项式必定连续。扫描时保存活动指数向量，把剩余冻结单项式追加到当前 QQ 系数；组变更时对完整系数调用 `__point_zero`。

固定活动幂后，删除它不会改变冻结单项式的相对序，也不会引入重复项，因此系数无需重新排序或 map 合并。完整活动指数向量必须包含全部活动变量，不能逐项判零。例如在 x²=2 的所选前缀上，`x²*y-2*y` 应把 y 的系数合并为 `x²-2` 后才判零。

```text
__prefix_zero(p,prefix,active):
  coefficient := 链比较器下的 QQ 零多项式
  扫描 p 的规范化项:
    拆出完整活动指数向量与冻结变量单项式
    若开始新的活动组且已有 coefficient:
      若 __point_zero(coefficient,prefix) 为假，返回 false
      清空 coefficient
    将当前冻结项追加到 coefficient
  返回 coefficient 为空，或 __point_zero(coefficient,prefix)
```

有限个活动单项式线性无关，因而每个合并系数在前缀上为零，当且仅当整式特化为零。点判零使用实点层的精确零集标签和保根精化契约，不依赖伪余式为零。

## Taylor 提取与终止性

```text
__lazard_evaluate_qq(result,lifting_order,sample):
  对每个已固定坐标 i:
    prefix := sample 的前 i+1 维证书副本
    h := result.polynomial；r := 0；factorial := 1
    active := 提升表中 i 之后的变量
    当 __prefix_zero(h,prefix,active):
      h := 对第 i 个坐标求导
      r := r+1；factorial := factorial*r
      assert h 非零
    将 h 的 QQ 系数全部除以 factorial
    result.polynomial := h；valuation 追加 r
```

设前 i-1 个坐标已提取，当前冻结解释的多项式非零。把它关于第 i 个坐标在样本值处作有限 Taylor 展开，首个非零系数的次数为 r；在特征零下，首个非零特化导数恰为该系数乘 r!。除以精确正整数阶乘便提取了该系数。

代表保留先前冻结变量，且不模三角列约简。对不同后续变量求导与先前坐标特化交换，所以这个 QQ 代表在下一步仍有正确解释。逐坐标归纳得到最小字典序非零 Taylor 系数，valuation 正是其指数元组，最终专门化非零。这与 MPP 的逐坐标除因子定义一致。

每次导数使当前变量次数下降；非零特化不变量保证在零多项式之前找到非零系数。坐标数有限，系数判零均终止，所以整个求值终止。样本仅按 const 引用输入，算法精化局部前缀副本，不改变原点。

## 公共集成与验证

通过 `<clpoly/clpoly.hh>` 或独立 `<clpoly/lazard_qq.hh>` 使用，纯结果类型可独立包含。测试包括无理前缀的多阶消失与阶乘、完整活动系数组合、共轭上下文、一般 grlex 输入、直接字典序内核、非法活动变量和空结果比较器。

[独立 Taylor 探针](../../reports/upper-contract-probes.cc) 对固定种子 20261106 的 72 个双前缀 QQ 用例直接按二项式展开原输入，合并并选取最小字典序非零系数；不调用待审算法的求导、平移或系数收集构造答案。交替提升坐标序，逐例比较 valuation 和最后一元多项式。

```sh
make test/test_lazard_qq test/test_clpoly_api
_build/debug/bin/test_lazard_qq
_build/debug/bin/test_clpoly_api
```

本轮以已提交精确算术、分解和实点查询的契约为依赖，不修改底层算法。
