# 开区间的最简有理数采样

本模块用经典连分数方法在非空有理开区间中选择最简有理数，可供几何算法选择有理样本。本文集中给出调研、架构、函数细化、伪代码、证明和验证要求；算法及数据类型分离，不为采样引入新的类。数学论证和人工审核不等于机器形式化验证。

## 算法依据与选择

- [SageMath `simplest_rational`](https://doc.sagemath.org/html/en/reference/rings_numerical/sage/rings/real_mpfi.html#sage.rings.real_mpfi.RealIntervalFieldElement.simplest_rational) 定义最简性为：既约分母最小，其次分子绝对值最小，并支持开闭端点。[官方源码](https://github.com/sagemath/sage/blob/develop/src/sage/rings/real_mpfi.pyx) 提供精确后备。
- [Haskell `Data.Ratio.approxRational` 源码](https://hackage-content.haskell.org/package/base-4.7.0.1/docs/src/Data-Ratio.html) 解释了取整数部分、取倒数和反向恢复的经典算法。其源码按闭区间实现，不能将端点判断直接用于 CAD 开间隙。
- 本实现独立细化上述连分数思路，严格处理开区间；不引入 Sage、MPFI 或其他数值依赖。它不是 XY16/XZ06 论文中的采样子程序，也不改变这些论文对应的三角列和实点算法。

连分数通过整除商跳过候选，不逐分母枚举；零和整数就是基本分支。原候选的二进制网格搜索不保证最小分母，不作为正式算法。Stern–Brocot 方法也可求解；这里不采用逐条树边移动的朴素版本，以免大部分商造成长路径。

## 模块和接口规约

文件 `clpoly/rational_sample.hh`，仅依赖现有 `number/QQ.hh` 和标准库；不新增数据类型，不依赖多项式、三角列、实点或 CAD。QQ 保持既有表示，`rational_interval` 仍表示非空有界闭区间。

```cpp
QQ sample_rational(const QQ& left, const QQ& right);
QQ sample_rational(const std::optional<QQ>& left,
                   const std::optional<QQ>& right);
```

前置/异常：有限端点必须满足 `left < right`，否则抛 `std::invalid_argument`，包括相等端点。缺失左端点表示负无穷，缺失右端点表示正无穷；所有存在的端点均排除。不接受闭区间单点，也不把 `rational_interval` 隐式解释成开区间。

后置：返回既约 QQ，严格属于指定区间；在全部候选中分母最小，同分母时分子绝对值最小。输入保持不变，没有共享可变状态。

有限重载避免为普通有界间隙构造两个 optional；无界重载只负责边界分派，两个端点存在时转发到有限重载。缺失端点不是 QQ 值，不能参与 QQ 运算。

公开入口检查有限端点的严格顺序并抛异常，Release 构建也保留检查。内部 `rational_sample_detail::positive` 仅处理调用方保证合法的非负区间，以 `assert` 检查 `0<=left<right`；取整辅助函数断言 QQ 分母为正。本模块不涉及变量序，无需字典序转换。`QQ` 负责数值表示，独立函数负责采样算法。

## 完整伪代码与函数细化

以下使用代码中的实际函数名。`rational_sample_detail` 隔离局部的向下取整和非负区间算法；数学证明中的 a、b、K 分别对应实现的 left、right、parts。所有整数部分和中间分子、分母均为 ZZ，不转换成固定宽度整数。

### `rational_sample_detail::floor(q)`：向下取整

```text
rational_sample_detail::floor(q):
    result := ZZ 零
    ZZ::fdiv_q(result,q.num(),q.den())
    返回 result
```

QQ 分母为正，向下取整和向零截断在负数上不同，必须复用这里的整数下整除。

### `sample_rational(left,right)`：有限入口

```text
sample_rational(left,right):
    若 !(left<right): 抛 invalid_argument
    若 left<0<right: 返回 QQ 零
    若 right<=0: 返回 -rational_sample_detail::positive(-right,-left)
    返回 rational_sample_detail::positive(left,right)
```

### `rational_sample_detail::positive(left,right)`：连分数循环

前提 `0<=left<right`，两个端点都有限且不包含。按值保存变换中的端点，parts 保存尚未恢复的整数部分。

```text
rational_sample_detail::positive(left,right):
    parts := 空的 ZZ 栈
    循环:
        k := rational_sample_detail::floor(left)
        result := QQ(k+1)
        若 result<right: 跳出循环

        将 k 压入 parts
        fraction := left-k
        left := 1/(right-k)
        若 fraction=0:
            result := QQ(rational_sample_detail::floor(left)+1)
            跳出循环
        right := 1/fraction

    按 parts 的逆序依次取 k:
        result := k+1/result
    返回 result
```

没有整数内点时，倒数变换交换端点。新下界使用修改前的右端点，新上界使用旧下界的小数部分；fraction=0 时，新区间为右无界，立即选择整数，不实际计算 1/0。恢复阶段严格逆序，避免长连分数耗尽递归调用栈。

### `sample_rational(optional left,optional right)`：无界入口

```text
sample_rational(optional left,optional right):
    若 left、right 都存在: 返回 sample_rational(*left,*right)
    若 left、right 都缺失: 返回 QQ 零
    若 left 缺失:
        若 *right>0: 返回 QQ 零
        返回 QQ(-rational_sample_detail::floor(-*right)-1)
    若 *left<0: 返回 QQ 零
    返回 QQ(rational_sample_detail::floor(*left)+1)
```

无界结果为区间内绝对值最小的整数。例 `(1/3,1/2)` 依次变为 `(2,3)`、`(1,+∞)`；整数 3 是开端点，必须排除。末层返回 2，逆序恢复得 `2+1/2=5/2`、`1/(5/2)=2/5`。

## 正确性、最简性和终止性

### 区间变换与不变量

进入每次有限循环时 `0<=a<b`。设 k=floor(a)，若 k+1<b，则 k+1 严格在区间中。否则 `k<=a<b<=k+1`，每个 `x∈(a,b)` 都满足 `0<x-k<1`。变换 `y=1/(x-k)`、逆变换 `x=k+1/y` 给出从原开区间到 `(1/(b-k),1/(a-k))` 的双射。取倒数交换端点次序。若 a=k，新上界为正无穷，由专门分支立即选点，实际不计算 1/0；其余情况新的有限端点仍严格有序。

设栈 K 表示变换 `T_k(t)=k+1/t` 的依次复合 Phi，空栈时为恒等。循环保持：当前区间经 Phi 双射到原正区间。恢复阶段按逆序应用 T_k，返回值属于每一层开区间。零分支和负区间反射同样保持严格包含性。

### 最简性：证明分子、分母同时最小

在非负开区间中候选均为正数。证明返回的既约 `p0/q0` 对任意候选既约 `p/q` 同时满足 `p0<=p` 和 `q0<=q`。

整数基本分支返回 n=floor(a)+1。任意候选 p/q>a>=k，故 `p>kq`，从而 `p>=kq+1>=k+1=n`；分母 q>=1。上界无穷的基本分支同理。

无整数时，既约 x=p/q 经变换成为既约 `y=q/(p-kq)`，其分子、分母均正，且 `gcd(q,p-kq)=1`。若变换后结果为既约 u/v，归纳假设给出 `u<=q`、`v<=p-kq`。恢复为 `(ku+v)/u`，仍既约，并满足

`u<=q`，`ku+v<=kq+(p-kq)=p`，因为 k>=0。

所以逆序恢复保持同时最小性，特别推出分母优先、同分母时分子最小。负区间反射保持分子绝对值；跨零时 0/1 最简。无界入口返回绝对值最小的可用整数。

### 终止性

有限端点写为既约 a=p/q、b=s/t，q,t>0。无整数且 a 非整数时，新端点的分母分别是 `s-kt`、`p-kq`，满足

`0<s-kt<=t`，`0<p-kq<q`。

分子分母交换不产生公因子，故新分母之和严格小于 q+t。若 a 是整数，下一步转为右无界并直接结束。正整数 q+t 是下降度量，所有变换有限终止，栈也有限。恢复过程中的除数始终为正。

该论证证明终止性，不把大整数运算视为常数时间；不承诺每次循环固定耗时。

## 使用与验证

包含 `<clpoly/clpoly.hh>` 或独立的 `<clpoly/rational_sample.hh>`。

```cpp
using namespace clpoly;
QQ q = sample_rational(QQ(1,3), QQ(1,2)); // 2/5
QQ negative = sample_rational(std::nullopt, QQ(0)); // -1
```

独立头文件测试检查手算结果、严格开端点、无界区间、空区间拒绝、任意精度与长连分数。固定种子 20261106 的 1000 个小有理区间使用逐分母枚举独立求最简答案，同时检查包含性。`test_clpoly_api` 仅包含库总头文件，验证公共集成。

```sh
make test/test_rational_sample test/test_clpoly_api
_build/debug/bin/test_rational_sample
_build/debug/bin/test_clpoly_api
```

本模块不负责根隔离、精化或代数数表示；后续算法使用它时应先提供认证的 QQ 开间隙。

## CAD 接口协议与理论对应

`full_cad.hh::__lift_stack` 调用采样器，`full_cad_result` 保持纯存储职责。

| Sector | 传入采样器的严格 QQ 开区间 |
|---|---|
| 无根 | 两侧无界，得到 0 |
| 第一个根左侧 | `(-∞, first_root.box().back().left())` |
| 相邻根之间 | `(previous.box().back().right(), next.box().back().left())` |
| 最后一个根右侧 | `(last_root.box().back().right(), +∞)` |

内部 Sector 仍先通过 XZ06 精化使根盒严格分离。采样器既不精化根，也不接收无理数。真实边界为 α<β，已认证端点满足 `α<a<b<β`，新样本 q 满足 `α<a<q<b<β`；混合有理/代数边界沿用同一证书适配。

最简性仅针对传入的 QQ 内间隙，不保证在整个真实代数根间隙中全局最简。Section 仍使用原实点。新 Sector 样本属于同一胞腔，后续纤维根和隔离盒可以变化；投影、完整栈、来源身份、维数和符号不变性证明不变。

PDF 的无编号辅助过程 SAMPLE-RATIONAL/POSITIVE 给出以上伪代码和证明，算法 12 LIFT-STACK 统一调用，算法 13 FULL-CAD 不改变控制流。原 13 个编号算法保持编号。

## 验证设计与性能边界

独立测试 `test/test_rational_sample.cc` 只包含采样头，不依赖多项式。可手算用例直接检查答案：跨零、正负整数、整数端点、零端点、非二进制分母 2/5、反射、无界、相等/反序拒绝、超出 64 位的端点、200 位窄区间和长连分数恢复。

独立实验用固定种子的随机 QQ 区间检查包含性，并逐分母枚举小规模输入，检查最简性；枚举不进入库算法。完整 CAD 回归检查完整栈、Section/根身份、Sector 严格归属、前缀一致、维数和原输入符号；补充根 0、1 之间返回 1/2 的集成用例。

结果及复现命令集中在[构建与测试验证](../reports/modular-cad.md)。算法保证简单样本，不保证每个输入更快；选点改变后续纤维几何，完整 CAD 的收益必须实测。

### 054 的历史独立阶段实验（2026-10-22）

此前对 054 首个 Sector `(-∞,-27^(1/4))` 的实验，从相同根盒边界选 -3，代替分母 `2^89`、约 -3.27950705695 的旧样本。固定九次 y 层乘积，前缀盒为样本加减 1/8，重建上下界并调用原两次平方自由检查。GCC 13.4.0、arm64，翻译单元 `-O2 -DNDEBUG` 链接既有 Debug 静态库，三次交替运行中位数：

| 样本 | 平移 | 上下界系数最大位数 | 两次平方自由检查 ms |
|---|---|---:|---:|
| 原分式 | 原分式 | 5073 bit | 3941.48 |
| -3 | 原分式 | 278 bit | 98.53 |
| 原分式 | 向上取整 | 706 bit | 364.92 |
| -3 | 向上取整 | 99 bit | 11.56 |

这是首纤维首轮检查，不是完整 CAD 加速倍数。四种配置均通过原平方自由检查；样本也改变纤维几何，不把全部差异归因于位数。源码和结果位于 `/private/tmp/clpoly-054-simple-sample-20261022/`，临时目录可能被清理。

本次正式采样函数的独立枚举结果、保存于仓库的阶段探针和复测数据见[构建与测试验证](../reports/modular-cad.md)。复测只更换采样，不开启整数平移。

整数平移 `S=ceil(2+M/m)` 仍为独立候选优化，本次不接入。其依据为 `S>=2+M/m`，保证原根 r 平移后 `r+S>=1>0`；实际平移必须随根证书保存，后续 NSHR 使用同一值。正式 XZ-LIFT 仍采用 PDF 的精确 `2+M/m`，不能把上表两个优化同时开启的结果记为本次实现收益。
