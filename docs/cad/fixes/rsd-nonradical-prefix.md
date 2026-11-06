# RSD 使用边界：非约化前缀、文献条件与 WRSD 替代范围

## 2026-11-06 底层待办与上层审核边界

本轮上层审核以 `HEAD=5402483` 为边界，假设已提交底层库满足其文献与接口契约。本节记录的底层实现问题留待后续修复，不计为未提交上层代码的错误；上层仍须满足合法调用前提。条件正确性结论见[本轮审核记录](../reports/uncommitted-review.md)。

| 底层待办 | 缺陷位置 | 提交来源 | 原有接口反例 |
|---|---|---|---|
| WRSD 非法迭代器 | `charset.hh` 的 `g.erase(g.end())` | `968c20d9`，2021-04-14 | `wrsd([x²-2,y²-2],y-x)` |
| 平方自由分解不终止 | `charset.hh` 中 WRSD 返回未变前缀后再次递归 RCtoSqrfree 的分支 | `bbcacc48`，2021-07-17 | `RCtoSqrfree([x²,y³,z²+xy])`，机制见下文 |

两处代码均已存在于 `HEAD`，`charset.hh` 当前无未提交修改。仅包含已提交头文件、直接调用上述旧接口也能复现，无需 `regular_chain.hh`、`real_algebraic.hh` 或 `full_cad.hh`。

WRSD 第一例使用比较器 `{y,x}`、方程提升序 `{x,y}`；平方自由分解例使用比较器 `{z,y,x}`、提升序 `{x,y,z}`，全部多项式绑定同一个有效比较器。将库与探针统一以 `-D_GLIBCXX_DEBUG` 重编译后，第一例以 SIGABRT 退出并报告 `attempt to erase from container with a past-the-end iterator`。普通构建可以表面返回正确标签，不能据此排除未定义行为；本记录不宣称通过 ASan。第二例直接调用旧 RCtoSqrfree 会重复输入并最终以 SIGSEGV 退出，应只在隔离进程中运行。

已确认 `full_cad({y-x,y+x},{y,x})` 的合法来源判零路径会触发第一项。第二项在一般正则链接口上成立，但该特定非约化前缀能否由完整 CAD 自动生成，尚未确认；两种影响范围不得混写。本轮没有修改底层实现。

`charset.hh` 原有的 `rsd` 实现和 `test_charset.cc` 原有 RSD 测试完整保留，与提交 `5402483` 一致。新增三角列、实代数点、Lazard 和 CAD 代码不直接调用 RSD；新增的 `split_zero` / `__split_zero` 强契约包装器不再提供，只保留 WRSD 包装器。本说明和 WRSD 接口、测试、理论 PDF 一并审核，没有把 RSD 的强契约改成 WRSD 契约。

## 两种契约与替代范围

设 T 为无参数零维正则链，V 表示复零点集。WRSD 返回两组正则链 H、G，满足：

\[
\bigcup_{C\in H}V(C)=V(T)\cap V(f),\qquad
\bigcup_{C\in G}V(C)=V(T)\setminus V(f).
\]

RSD 的旧接口还承诺零分支上 `prem(f;C)=0`。含重根时，在全部根上为零不等价于伪余式为零，因此 WRSD 不无条件替代这一强契约。

指定点查询与系数扫描只需要上述零集划分。原有递归分解还必须分别证明问题规模严格下降，不能由标签正确直接推出终止性：

| 调用者 | 实际需要的性质 | 当前算法 |
|---|---|---|
| `point_zero`，以及经它调用的符号判断、同点比较、重编码、Lazard 和 CAD | 分支有限、覆盖完整、零与非零标签正确 | `weak_split_zero` → `wrsd` |
| `roots_over` 的系数扫描 | 在所选前缀上确定系数为零或非零 | `weak_split_zero` → `wrsd` |
| `ZDtoRC` | 选出下一层初式不消失的前缀分支 | `wrsd` |
| `RCtoSqrfree` | 零／非零分支之外还需要递归进展；非约化前缀上已有不终止反例，见下文 | 原有 `wrsd` 调用，递归策略待修复 |
| `GRDforZDSAS` | 对等式链上的约束多项式作零集划分 | `wrsd` |

新增代码没有 RSD 调用；原有 `rsd` 的自递归和 `test_charset.cc::rsd_basic` 的调用保留。外部若需要商环理想成员关系或伪余式证书，不能将 WRSD 的零标签作为这种证书。

## 新增代码经旧模块的间接调用核对

新增代码不直接调用 RSD；如果经原有 `charset.hh` 代码间接进入 RSD，必须在这里列明调用链并汇报，不能为了消除该路径而擅自修改或删除基线算法。当前核对结果如下：

```text
point_zero / roots_over 的系数扫描
  → weak_split_zero → __weak_split_zero → wrsd → wrsd 自递归

squarefree_decomposition
  → RCtoSqrfree → wrsd / RCtoSqrfree 自递归
```

当前这两条路径均不进入 RSD。原有 `ZDtoRC`、`GRDforZDSAS` 同样调用 WRSD，没有 RSD 调用。全库源码中的 RSD 调用仅有 `rsd` 自身的递归，以及保留的基线测试 `rsd_basic`；测试中的直接调用不属于新增功能的间接调用。

总头文件包含 `charset.hh` 会使原有 RSD 定义可见，但包含定义本身不是执行调用。后续若修改上述旧算法而新增间接 RSD 路径，必须重新核对并汇报。

`point_zero` 改用 WRSD 的数学论证成立，但这不等于全库每个递归算法都已正确。尤其 `roots_over` 最终依赖平方自由分解；其系数扫描不需要 RSD 强契约，而完整纤维接口仍依赖下述未完成的平方自由分解修复。

## 文献及版本边界

依据仓库所给 [Xia–Yang 2016](<../../references/triangular-set/Xia 和 Yang - 2016 - Automated Inequality Proving and Discovering.pdf>)：

- 定义 1.4、1.5，书页 9–10：子结式及扩展 PSC 链。
- 定义 2.6、2.7，书页 30：正则链与平方自由性是不同条件。
- 定理 2.4、2.5 和 Algorithm 2.1，书页 33–35：公共因子计算及 RSD 终止性。
- 定义 2.11、Algorithm 2.2、定理 2.6，书页 38–44：WRSD 的零集契约与算法。

原 PDF 页面已直接核对，下面的条件与公式不是仅据 OCR 得出。关于条件缺口的判断针对这份版本的书面陈述；尚未取得作者勘误或原始 RSD 实现来确认其原意，不扩大为“任何 RSD 算法都不终止”。

## 最小反例

提升顺序为 `(x,y)`，输入为：

\[
T=[x^2,y^3],\qquad f=xy.
\]

两个初式均为 1，因此符合书中零维正则链条件；后一方程关于前一方程已约化。唯一不同零点是 `(0,0)`，但链不平方自由。Algorithm 2.1 的输入没有另列平方自由条件。

第一步的两个提前返回条件均不满足：

\[
\operatorname{prem}(xy;T)=xy\ne0,\qquad
\operatorname{res}(xy;T)=0.
\]

按第 1 章先按次数排序的约定，以 F=y³、G=xy 计算关于 y 的链。m=3、l=1、μ=2。定义 1.4 给出 S₁=lc(G)^(m-l-1)G=x²y；定义 1.5 给出 S₂=G、S₃=F、R₃=1。

| 下标 j | 子结式 Sⱼ | PSC Rⱼ | 对前缀 `[x²]` 的伪余式 |
|---|---|---|---|
| 0 | 0 | 0 | 0 |
| 1 | x²y | x² | 0 |
| 2 | xy | 0 | 0 |
| 3 | y³ | 1 | 1 |

S₂ 关于 y 的次数是 1，小于下标 2，因此是缺陷项，其 PSC 按定义取零。原实现返回和筛选得到的正是这张表，不能把本例解释为简单的数组末项越界或多遍历一项。

## 公共因子条件的缺口

取 i=3，定理 2.4 写出的条件满足：所有较小 PSC 对前缀的伪余式为零，并且：

\[
\operatorname{res}(R_3;x^2)=\operatorname{res}(1,x^2;x)=1\ne0.
\]

其书面结论会将 S₃=y³ 作为商环 `QQ[x,y]/〈x²〉` 中的公共因子。但在系数环 `QQ[x]/〈x²〉` 中，x 仍非零，只有 x² 为零。因此 xy 是非零的一次多项式，首一三次多项式 y³ 不可能整除它。

这说明所给版本定理 2.4 的陈述不足以支持在该非约化前缀上直接选 Sⱼ 作为公共因子。可能需要限制下标、加强前缀条件或补充主系数退化处理；本次不将未经证明的任一种补充作为作者原意。

Mathematica 14.2.1 的独立核算确认：

```wl
ClearAll["Global`*"];
Subresultants[y^3, x*y, y]
(* {0, x^2}：这是通常 PSC 列表，不是子结式多项式列表。 *)

PolynomialReduce[x*y, {y^3, x^2}, {y,x}]
(* {{0,0}, x*y}：候选因子和前缀不能消去查询。 *)

PolynomialRemainder[x^2, x^2, x]
(* 0 *)
Resultant[1, x^2, x]
(* 1 *)
```

扩展项 S₂、S₃、R₂、R₃ 依据定义 1.5 补充，不将其冒称为 Mathematica 的直接输出。

## 原实现为什么无限递归

保留的 `charset.hh` 原实现中，相关处理是：

```cpp
auto ft1 = subreschain[i];
for (auto c = T.rbegin()+1; c < T.rend(); ++c)
    ft1 = prem(ft1, *c, get_first_var(*c));
auto ft2 = pquo(T[n-1], ft1, x);
auto T1 = T, T2 = T;
T1[n-1] = ft1;
T2[n-1] = ft2;
auto W1 = rsd(T1, f), W2 = rsd(T2, f);
```

反例中选中下标 3，得到 `ft1=y³`、`ft2=1`、`T1=T`。f 也没有变化，于是从第一次调用开始重复相同的 `(T,f)`，最终耗尽调用栈。实际跟踪与 AddressSanitizer 的栈溢出结果一致；不是普通的有限深递归。

原实现同时可用 `git show 5402483:clpoly/charset.hh` 复查。使用该原有接口的最小输入为：

```cpp
variable x("x"), y("y");
lex_<custom_var_order> order(custom_var_order({y,x}));
using P = polynomial_<ZZ,lex_<custom_var_order>>;
P a(&order), b(&order), f(&order);
a = polynomial_ZZ(x*x);
b = polynomial_ZZ(y*y*y);
f = polynomial_ZZ(x*y);
auto result = rsd(std::vector<P>{a,b}, f);
```

上述片段使用仍保留的原有接口，但会触发已知的不终止问题。只能在有超时或深度诊断的独立进程中复现，不得把这个调用放进正常测试进程。

## WRSD 为什么足够支持指定点判零

对同一反例，WRSD 可以直接返回 `H=[T]`、`G=[]`：xy 在唯一不同零点 `(0,0)` 上为零，符合零集契约，虽然伪余式仍为 xy。

`point_zero` 直接接受 WRSD 的 H/G 标签，然后仅在原点的根盒上排除不相容分支。包含所选点的分支不会被区间求值错误删除；不包含该点的分支必有一个方程在该点非零，随根盒收缩最终可以排除。分支有限，因此最终只剩正确标签并终止。该证明不使用 `prem(f;C)=0`，也不要求分解结果自身平方自由；被精化的是原来已经认证的实点。

当前 `point_zero`、理论 PDF 的 POINT-ZERO 伪代码及证明均采用这个契约。公开入口是 `weak_split_zero`，内部入口是 `__weak_split_zero`；没有名为 `split_zero` 的兼容别名，也没有 RSD/WRSD 选择开关。

## 平方自由分解的额外失败：零集标签不保证递归进展

对 WRSD 的全部调用者继续核查时，发现原有 `RCtoSqrfree` 还存在独立的不终止输入：

\[
T=[x^2,y^3,z^2+xy].
\]

它是初式均为 1 的零维正则链，唯一不同零点为 `(0,0,0)`。当前 `squarefree_decomposition(T)` 会反复进入同一个 `RCtoSqrfree(T)`，最终栈溢出。

具体过程如下：

1. 最大待平方自由层是最后一层，前缀 A=[x²,y³] 仍不平方自由。
2. 末层判别式为 -4xy，WRSD(A,-4xy) 可以并且实际返回零分支 A 本身、非零列表为空。这符合零集契约。
3. 子结式的第零 PSC 为 4xy，其对 A 的伪余式非零，而逐次结式为零。
4. 再调用 WRSD(A,4xy)，仍得到未变化的零分支 A。代码补回 z²+xy 后，对同一个 T 递归。

跟踪结果为：

```text
SF call 1 T=[x²] [y³] [z²+xy], PSC=4xy
SF call 2 T=[x²] [y³] [z²+xy], PSC=4xy
SF call 3 T=[x²] [y³] [z²+xy], PSC=4xy
...
```

这里没有恢复或调用 RSD；WRSD 的零标签也是正确的。失败说明平方自由分解调用者没有建立递归严格下降条件，不能把 WRSD 的弱零标签视为会自动简化前缀。

该问题不是本次将 `point_zero` 切换为 WRSD 引入的：原有 `RCtoSqrfree` 已经使用 WRSD。但它限制了目前能够给出的全库正确性结论。算法 3.1 的书面递归进展及其 C++ 落实需要继续审核和修复，不能以已有回归通过替代证明，也不能用增加栈空间解决。

当前仅确认 WRSD 路线足以支持点查询所需的零集契约；尚不能据此保证任意正则链的平方自由分解及全部下游功能均正确终止。本批整体正确性审核尚未完成。

## 实验记录及本批回归

对原有实现的固定种子实验包含 60 组二维／三维正则链，其中 48 组平方自由、12 组含重根；所有输入复根都可精确枚举。WRSD 的 60 组均通过标签、覆盖及额外零点检查，RSD 有两组栈溢出，随后缩减为上面的反例。

48 组平方自由输入中，各例重复测量的单次中位耗时求和，原始 RSD/WRSD 分别为 2.153/1.002 ms，对象内部入口分别为 3.944/2.612 ms。完整 CAD 小规模测试总耗时约 62 ms，改用 WRSD 后差异很小。这些是临时构造实验的观察，不作为“所有输入更快”的保证或正式数据集基准。

新增功能回归使用 WRSD。`test_regular_chain.cc` 使用 `[x²,y³]`、`xy` 检查零分支保留完整根集；其余新增测试检查共轭分支、全部为零／非零、一般序 QQ 转换、前缀和对象所有权。`test_charset.cc` 的原有 WRSD 与 RSD 测试保持不变，原有 RSD 测试继续执行。下游联测覆盖实代数点、Lazard 与完整 CAD。验证结果见[构建与测试记录](../reports/modular-cad.md)。

新增代码如需直接调用 RSD，须先补齐含重根前缀的算法依据、输出强契约及严格下降证明；如经基线代码间接调用，则先列明路径和影响范围并汇报。保留原有实现不表示相关问题已经修复；增加栈空间、截断递归或直接返回原链都不能替代这些要求。
