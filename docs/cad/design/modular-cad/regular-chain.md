# 零维正则链：细化设计、伪代码与实现对应

本层定义零维无参数 QQ 正则链的数据对象 `regular_chain`，并适配已有 `charset.hh` 分解算法。入口为 `clpoly/regular_chain.hh`，不依赖区间、实代数点或 CAD。区间求值是并列模块，见[多项式区间求值](../polynomial-interval/design.md)。

## 范围与可信依赖

本层只支持无参数、零维的正则三角列。方程保存为 QQ 多项式；原有整数多项式运算、结式、判别式以及 WRSD、RCtoSqrfree 按可信接口使用。本层不重复实现这些算法，也不以构造器检查代替其数学证明。

[XY16](<../../../references/triangular-set/Xia 和 Yang - 2016 - Automated Inequality Proving and Discovering.pdf>) Algorithm 2.2、3.1 分别位于书页 40、62，对应 WRSD、平方自由分解。此处给出数据管理、调用前提和结果检查的完整伪代码；上述文献算法本身直接调用已审核的 `charset.hh`。

## 公开入口与内部入口

以下清单覆盖 `regular_chain.hh` 的类型、全部公开成员、公开／内部算法入口及实现辅助函数。沿用 `project` / `__project` 的分层约定：公开算法入口检查外部参数并完成必要的序转换；`__` 入口要求调用者保证前提，只使用断言。数据查询、特殊成员函数和单纯的表示转换按各自职责列出，不为每个成员机械增加一个 `__` 版本。

数据成员沿用 `cad_tree` 的前置下划线命名，内部自由函数直接放在 `clpoly` 命名空间。当前仅提供 `weak_split_zero` / `__weak_split_zero`，使用 WRSD 的零集契约。新增代码不提供 RSD 强契约包装器；`charset.hh` 原有 RSD 和测试保留。直接／间接调用边界和文献条件核查见[RSD 独立说明](../../fixes/rsd-nonradical-prefix.md)。

### 类型与参数约定

| 类型或记号 | 含义 |
|---|---|
| `regular_chain` | 借用外部比较器、按值保存 QQ 方程和累计平方自由标记的零维正则三角列 |
| `regular_chain::order_type` | `lex_<custom_var_order>` |
| `regular_chain::polynomial_type` | 采用上述字典序的 QQ 多项式 |
| `regular_chain::integer_polynomial` | 采用上述字典序的 ZZ 多项式 |
| `regular_chain_split` | 结果对象；`zero`、`nonzero` 均为 `std::vector<regular_chain>` |
| `vars`、`equations` | 构造器中的提升坐标表和按同一提升方向排列的方程表；具体变量序见下一节 |
| `f`、`q` | f 为外部 ZZ/QQ 多项式，可采用其他多项式序；q 为已转换到 T 的 QQ 字典序的同一多项式 |

输入多项式沿用库的规范化表示前提。所有算法处理方程及复零集；三角列层不接收根盒，也不选择某个实根。

### 公开数据接口

| 接口 | 返回值或效果 | 检查、转换及所有权 |
|---|---|---|
| `regular_chain()` | 空链，平方自由标记为 true | 借用静态默认比较器 |
| `regular_chain(order)` | 借用指定序的空链 | order 必须非空，调用者保证其生命周期 |
| `regular_chain(vars,equations,order)` | 构造正则链 | 接受一般序的 ZZ/QQ 方程；检查非空序指针、连续坐标编号、三角结构和正则性，转换到 order 指定的 QQ 字典序，并计算平方自由属性；非法输入抛 `invalid_argument` |
| 默认复制构造／赋值 | 复制数据及比较器地址 | 副本与源对象借用同一个外部序；不重算结式 |
| 默认移动构造／赋值 | 转移容器并复制比较器地址 | 不遍历方程或单项式；源对象可销毁或重新赋值 |
| `size()` | `size_t`，方程数／坐标数 | 只读查询 |
| `is_squarefree()` | `bool`，整链是否平方自由 | 读取累计标记，不重新计算判别式 |
| `variables()` | `const std::vector<variable>&` | 借用本链的提升坐标表 |
| `equations()` | `const std::vector<polynomial_type>&` | 借用本链的 QQ 方程表 |
| `order()` | `const order_type*` | 返回借用的外部比较器地址，不转移所有权 |
| `convert(f)` | `polynomial_type` | 检查变量覆盖和非负指数，否则抛 `invalid_argument`；调用 `poly_convert` 转换到本链的 QQ 字典序；结果借用同一外部比较器；当前无独立 `__convert` 入口 |
| `integer_equations()` | `std::vector<integer_polynomial>` | 调用已有 QQ→ZZ 转换清正分母，保持方程零集及符号；结果借用同一外部比较器；输入已由对象不变量保证，不增加检查层 |
| `prefix(n)` | 借用同一比较器的 `regular_chain` | 检查 n 不超过链长，否则抛 `out_of_range`；合法时调用 `__prefix(n)`，复用已有前缀标记 |

方程表和坐标表的引用依赖所属三角列。比较器指针及转换结果则只依赖外部比较器：销毁、移动或覆盖原链不会使该比较器失效；调用者须保证比较器比所有借用者活得更久，且使用期间不改变变量序。

### 公开算法与对应内部入口

| 公开入口 | 内部入口 | 返回值及数学效果 | 边界职责 |
|---|---|---|---|
| `weak_split_zero(T,f)` | `__weak_split_zero(T,q)` | `regular_chain_split`；WRSD 分解，zero 覆盖 `V(T)∩V(f)`，nonzero 覆盖 `V(T)\V(f)`；零分支不额外要求逐次伪余式为零 | 公开入口通过 `T.convert(f)` 检查并转换；内部要求 q 已规范化、仅含链变量且比较器与 T 一致 |
| `squarefree_decomposition(T)` | 无独立内部入口 | `std::vector<regular_chain>`；各链正则且平方自由，其零集之并等于 `V(T)` | T 已是有效正则链，无需再验证输入正则性或转换序；此函数直接完成系数适配、调用底层算法及输出对象装配 |
| `T.prefix(n)` | `T.__prefix(n)` | `regular_chain`；取前 n 个提升坐标、方程和累计标记 | 公开入口检查长度；内部断言 `n<=T.size()`，不重算正则性或平方自由性 |

公开入口和对应内部入口的数学效果一致。WRSD 返回的是定义零点分支的链列表；同一列表内不要求不同链的零集互不相交。空链仅允许常数 f：f=0 时返回 `zero=[T]`，非零时返回 `nonzero=[T]`；另一列表为空。空链或已平方自由链的平方自由分解返回 `[T]`。`prefix(0)` 返回空链，`prefix(size())` 返回方程的完整副本并继续借用同一外部序。

### 内部装配与实现辅助函数

| 实际名称 | 输入／输出 | 调用前提与职责 |
|---|---|---|
| `__make_regular_chain(model,equations,flags)` | 同坐标、同字典序的 ZZ 方程及累计平方自由标记 → `regular_chain` | 调用者保证方程是正则链且 flags 正确；断言长度、比较器、主变量对应及正则性；复制 model 的坐标和比较器地址，转为按值保存的 QQ 方程；不调用公开构造器重新验证输出 |
| `__squarefree_prefixes(T,equations)` | 同坐标的正则 ZZ 链 → `std::vector<bool>` | 断言链长一致；计算各层判别式的逐次结式，建立累计平方自由标记；已出现 false 后后续标记均为 false |

`__make_regular_chain` 只装配可信算法的同坐标输出，不接受任意一般序输入；flags 的正确性由调用算法保证，装配函数不另算一遍验证。内部调用仍需清除 QQ 正分母或将 ZZ 系数转换为 QQ，这属于代数表示转换和对象所有权管理，不重新选择变量序。

正则性、分支标签和已保证的平方自由性等昂贵输出诊断只在断言启用时执行。分支的平方自由标记若未知，必须实际计算判别式以建立数据；这项计算不能用断言代替。RCtoSqrfree 已保证所有前缀平方自由，因此其输出直接采用全 true 标记。定义 `NDEBUG` 时，诊断断言被移除，调用者仍须保证所有前提。

### 与原有算法接口的对应

`charset.hh` 中的基列算法使用内部名称 `__basset`，只由 `charset` 调用，不提供公开的 `basset` 入口。仍启用的原有算法保留原名和向量接口；三角列对象的检查包装器及对应 `__` 入口位于 `regular_chain.hh`。底层向量接口要求调用者保证既定字典序和算法前提。

| 底层接口 | 数学任务／当前前提 | 本层调用关系 |
|---|---|---|
| `rankcompare(F,G)` | 比较吴秩；输入采用一致的自定义字典序 | `__basset` 选最低秩时使用 |
| `is_reduced(F,G)` | 判断关于 G 的主变量是否约化；断言 G 非常数 | `__basset` 筛选及 WRSD 的约化步骤使用 |
| `__basset(first,last)` | 从非零多项式组中选取基列；非零常数允许输入 | 由 `charset` 调用；本层分解入口不调用它 |
| `charset(P)` | 计算吴特征列；初始 P 不含零，比较器一致 | 属于既有特征列算法路径 |
| `wrsd(T,z)` | XY16 Algorithm 2.2；零维正则链、变量编号连续且比较器一致 | `__weak_split_zero` 直接调用；`RCtoSqrfree` 也会调用它 |
| `is_regular(T)` | 对已合法排列的三角列检查初式的逐次结式 | 内部装配时仅在断言中诊断输出 |
| `RCtoSqrfree(T)` | XY16 Algorithm 3.1；将零维正则链平方自由分解 | `squarefree_decomposition` 调用 |

当前主要调用路径为：

```text
weak_split_zero(T,f)
  → T.convert(f) → __weak_split_zero(T,q) → wrsd

squarefree_decomposition(T)
  → RCtoSqrfree

T.prefix(n) → T.__prefix(n)
```

原有特征列路径为 `charset → __basset → rankcompare / is_reduced`。外部数据的异常检查、内部函数的调用前提和可信原始算法的数学契约分别由上述各层承担，不能仅凭函数名把原有 `wrsd` 当作新的公开检查入口。

## 投影顺序、提升顺序和主变量

本文的数组下标及示例变量下标均从 0 开始。记 k 个投影变量组成的有序表为 `X=(x0,x1,...,x(k-1))`。统一使用以下两个术语：

- **投影顺序**：CAD 对外传入的变量表，按此顺序消去变量，同时作为字典序比较器的变量优先级。例如 `{x0,x1,x2}` 先消去 x0，再消去 x1，留下关于 x2 的投影多项式。
- **提升顺序**：投影顺序的反序。例如 `{x2,x1,x0}` 先隔离 x2，再在其样本上处理 x1，最后处理 x0。三角列坐标表、方程列表和根盒均采用提升顺序。

`get_first_var` 始终返回当前字典序比较器下最小的变量，即排在最前的变量。以比较器优先级 `{x0,x1,x2}` 为例，一条合法三角列可以是：

```text
提升下标 i    提升坐标与投影变量的对应    方程                 get_first_var
0             v0 = x2                   x2² - 2             x2
1             v1 = x1                   x1² - x2            x1
2             v2 = x0                   x0² - x1            x0
```

构造方式为：

```cpp
const regular_chain::order_type order(custom_var_order({x0, x1, x2}));
regular_chain T({x2, x1, x0},
    std::vector<polynomial_ZZ>{x2*x2-2, x1*x1-x2, x0*x0-x1}, &order);
```

构造器参数 `vars` 是提升顺序，外部 `order` 的优先级为 `{x0,x1,x2}`。构造器检查二者的连续反序关系，不创建比较器。它没有改变最小变量作主变量的规则。`full_cad(...,{x0,x1,x2})` 与该构造器的变量表用途不同，不能直接混用。

下文统一以 `L=(v0,...,v(k-1))` 表示提升顺序。它与投影变量表 X 的对应关系为：

\[
L=\operatorname{reverse}(X),\qquad
v_i=x_{k-1-i},\qquad x_j=v_{k-1-j},\qquad 0\le i,j<k.
\]

因此 k=3 时，`X=(x0,x1,x2)`，`L=(v0,v1,v2)=(x2,x1,x0)`，即 `v0=x2`、`v1=x1`、`v2=x0`。

下标 i 的方程 `T[i]` 只含 `L[0..i]`，即 `v0,...,vi` 或等价的 `x(k-1),...,x(k-1-i)`，且关于 `L[i]=vi=x(k-1-i)` 的次数为正；比较器在链坐标上的相对优先级为 `reverse(L)=X`，故 `get_first_var(T[i])=L[i]`。

取 `prefix(n)` 时保留提升表、方程和证书的前 n 项。例如上例 `prefix(2)` 保存 `{x2,x1}` 及前两条方程，继续借用完整比较器 `{x0,x1,x2}`；它在前缀变量上的相对优先级为 `{x1,x2}`。实点层调用字典序区间内核时，将提升变量表和根盒同步反转，不能只反转其中一个。

## 根盒与变量信息的来源

三角列本身不保存根盒。实点层的根盒满足 `box[i]` 对应 `chain.variables()[i]`，即 `vi=x(k-1-i)`。如果 CAD 对外变量表为 `{x0,x1,x2}`，则当前实点内部坐标及盒为 `{x2,x1,x0}`、`{I_x2,I_x1,I_x0}`。因此按 X 顺序提供给区间求值内核的盒应为 `{I_x0,I_x1,I_x2}`，须与坐标表同步反转。必须以明确的变量—区间绑定解释根盒。

原有 `charset.hh` 不另传变量表，是因为输入已是 `lex_<custom_var_order>` 多项式：单项式保存变量，比较器保存变量优先级。论文记号 `cls(f)` 保留文献的一基编号：文献的第 i+1 个提升变量对应本文的 `vi`。非常数 f 的 `cls` 是所涉及提升变量的最大一基编号；非零常数的 `cls` 为 0。因此 `mvar(f)=vi` 时 `cls(f)=i+1`；以完整投影表 X 表示，则 `mvar(f)=xj` 时 `cls(f)=k-j`。本节将变量名和数组下标改为零基，不改变文献 `cls` 的定义。

代码用 `get_first_var` 读取主变量，并用 `cls(f)=N+1-v_map[主变量]` 计算，其中 N 是该比较器声明的变量数，`v_map` 中的位置也从 1 开始。例如 `{x0,x1,x2}` 对应 `cls(x2)=1`、`cls(x1)=2`、`cls(x0)=3`。

WRSD 还要求 `cls(T[i])=i+1`，且 f 仅使用当前链覆盖的变量。递归前缀可以继承完整比较器，但各方程的 `cls` 必须从 1 开始连续，不能使用未声明变量，或把不同顺序的多项式混在一起。故“无需额外 vars 参数”不等于“无需预先给定变量顺序”。

这一限制关系到下标安全：取比较器 `{x0,y,x1,x2}`、链 `{x2²−2,x1²−3,x0²−1}` 和 f=x0−1，按比较器计算得到 `cls(f)=4`，即代码中的 n=4，但链长只有 3。当前 WRSD 会越界访问方程，已用启用标准库下标断言的独立例子复现异常终止。去掉未使用的 y 后，正常给出一个零分支和一个非零分支。`rankcompare`、`__basset` 仅比较相对顺序，没有这处编号到方程下标的转换，不能把这一问题泛化到全部三角列算法。公开构造器要求目标比较器中的链变量位于末尾连续位置，并与提升坐标反序，拒绝这种带编号缺口的目标序；输入方程的原比较器可以不同，由 `poly_convert` 转到目标序。若要让原 WRSD 接受这种比较器，须从链的实际主变量定位方程下标，而非直接使用完整比较器计算的 `cls`；本次仅明确前提，尚未修改原算法主体。

当前新增构造器还接受默认 grlex 等其他序的 ZZ/QQ 输入，需要调用者给出目标坐标顺序和外部比较器地址，再统一转换。因此本批保留其提升坐标参数。只有当接口明确要求输入已具有共同的目标字典序时，才能从该比较器和方程主变量推导坐标表；仍需验证三角形、坐标完备性及后续索引契约，不能直接删除参数。

## 约化判定与基列的常数终止

按 XY16 书页 28 的定义，`is_reduced(F,G)` 的前提是 `cls(G)>0`，即 G 非常数；判定式为 `degree(F,mvar(G)) < degree(G,mvar(G))`。入口断言 `!is_number(G)`，不为 G=0 或非零常数扩展定义。F 可以是零或非零常数，均关于非常数 G 约化；本库 `degree(0,v)=0`，因此可直接使用同一个次数比较式。

`__basset` 仍要求输入不含零，非零常数可以输入。按《计算机代数》书页 138 的 BasSet B2.3，选中非零常数时直接返回仅含该常数的基列，不再调用 `is_reduced`。选中的多项式若为零则断言失败。`charset` 初始输入须满足不含零的前提；后续只加入非零伪余式，因此维持该前提。WRSD 的约化对象来自输入正则链，其方程按定义均非常数。

## 数据结构与所有权

| 成员 | 类型及含义 |
|---|---|
| `_order` | `const order_type*`，借用外部 `lex_<custom_var_order>`；默认指向静态 `order_type::init` |
| `_vars` | 提升顺序 L，与方程主变量逐项对应 |
| `_equations` | 按值保存 QQ 方程，各方程和单项式共用 `_order` 指向的比较器 |
| `_squarefree` | 整条正则链是否平方自由 |
| `_squarefree_prefix` | 第 i 项表示前 i+1 个方程组成的前缀是否平方自由 |

三角列不拥有比较器，也不使用共享指针。外部调用者保证比较器比三角列、分解分支、前缀、转换结果以及由它们生成的实代数点活得更久，并且在借用期间不改变变量序。CAD 由结果容器持有一个稳定的比较器，内部三角列和实点统一借用它。

`equations()` 和 `variables()` 返回对象内部容器的引用，依赖该对象的生命周期。`order()`、`convert(f)`、`integer_equations()` 借用的是外部比较器，不要求原链继续存活。移动或销毁原链不会改变外部比较器的地址。

## `regular_chain::regular_chain(vars,equations,order)`：公开构造器

以下伪代码使用实际函数名、参数名和成员名，省略最外层 `clpoly::`；中文语句描述操作，不另设算法编号。构造器参数 `vars` 对应提升表 L，`equations` 是同方向排列的 ZZ/QQ 输入方程，order 为外部比较器地址。非法输入抛异常，非平方自由本身不是构造失败。

```text
regular_chain::regular_chain(vars,equations,order):
  _order := order
  _vars := move(vars)
  若 order 为空、|_vars|!=|equations| 或 _vars 有重复，抛 invalid_argument
  _equations := 空列表
  对 i=0..|_vars|-1:
    positions := _order->comp.v_map
    若 _vars[i] 不在 positions 中、i>=|positions|，或其位置不等于 |positions|-i，抛 invalid_argument
    对 equations[i] 中每个变量—指数对 (v,e):
      若 e<0 或 v 不属于 _vars[0..i]，抛 invalid_argument
    q := polynomial_type(_order)
    poly_convert(equations[i],q)
    若 q=0 或 degree(q,_vars[i])<=0，抛 invalid_argument
    _equations 追加 move(q)
  z := integer_equations()
  _squarefree := true；_squarefree_prefix := 空列表
  对 i=0..|_vars|-1:
    initial := leadcoeff(z[i])
    discriminant_i := discriminant(z[i],_vars[i])
    对 j=i-1,...,0:
      initial := resultant(initial,z[j],_vars[j])
      若 discriminant_i!=0:
        discriminant_i := resultant(discriminant_i,z[j],_vars[j])
    若 initial=0，抛 invalid_argument
    若 discriminant_i=0: _squarefree := false
    _squarefree_prefix 追加 _squarefree
```

三角形和主变量条件由变量覆盖、正次数以及反向比较器共同保证，不再额外扫描一次 `get_first_var`。在零维无参数情形，正则前缀上的逐次结式非零认证初式在所有复前缀根上不消失；判别式采用同一检查判断该层是否在全部前缀上平方自由。首层没有前缀，直接检查其初式和判别式。

`regular_chain()` 默认构造空链，借用静态默认序，`is_squarefree()` 为 true。`regular_chain(order)` 构造借用指定序的空链并拒绝空指针。常量方程、零方程、重复坐标、使用后面坐标或未声明坐标，以及不正则的初式均被拒绝。上述公开构造检查在 Release 中仍执行。

## 默认复制与移动

不声明自定义复制、移动、赋值或析构函数。编译器生成的成员操作复制比较器地址，复制或转移方程、坐标和属性容器。多项式自身的复制和移动已经保留比较器地址，无需重新绑定或规范化。

```text
复制三角列:
  复制 _order 指针、_vars、_equations、_squarefree、_squarefree_prefix
移动三角列:
  复制 _order 指针和 _squarefree，转移三个容器
```

复制的成本包括方程系数和坐标数据，移动不遍历方程或单项式。不提供额外的 `rebind_equations` 或私有 `swap`。移动后的源对象用于销毁或重新赋值，不将其作为已经认证的链继续求解。

## `convert(f)` 与 `integer_equations()`：表示转换

`convert(f)` 接收一个 ZZ/QQ 多项式，返回本链比较器下的 QQ 多项式：

```text
regular_chain::convert(f):
  若 f 含负指数或 _vars 之外的变量，抛 invalid_argument
  q := polynomial_type(_order)
  poly_convert(f,q)
  返回 q
```

`integer_equations()` 批量调用既有 QQ→ZZ 转换，不新增另一套清分母算法：

```text
regular_chain::integer_equations():
  out := 空列表
  对 _equations 中每个 f:
    z := integer_polynomial(_order)
    poly_convert(f,z)
    out 追加 move(z)
  返回 out
```

其中 `poly_convert` 取 f 各 QQ 系数正分母的最小公倍数 D，返回 z=D*f。例如 `x/2+1/3` 变为 `3x+2`。乘以非零正常数保持零集和符号，但不保持原多项式值；本接口只用于方程与判零分解，不代替 QQ 表达式的普通值查询。零多项式对应空项数组，返回零。

## `prefix(n)` 与 `__prefix(n)`：前缀

```text
regular_chain::prefix(n):
  若 n>size()，抛 out_of_range
  返回 __prefix(n)

regular_chain::__prefix(n):
  assert n<=size()
  result := regular_chain()
  result._vars := _vars 的前 n 项
  result._order := _order
  result._squarefree_prefix := _squarefree_prefix 的前 n 项
  result._squarefree := (n=0) 或 result._squarefree_prefix 的最后一项
  result._equations := _equations 的前 n 项副本
  返回 result
```

下标 i 的方程的正则性和判别式条件只依赖 `T[0..i]`，即前 i+1 个方程；删除后续方程不改变这些事实。故前缀复用已认证标记，不重算结式。完整链非平方自由时，前缀仍可能平方自由，不能直接复制整链的布尔值。坐标不重排，QQ 方程不改变，比较器地址也保持不变。

## `weak_split_zero` 与 `__weak_split_zero`：WRSD 适配

`weak_split_zero(t,f)` 返回 `regular_chain_split{zero,nonzero}`。零分支覆盖 `V(t)∩V(f)`，非零分支覆盖 `V(t)\V(f)`；分支为复代数零集层面的分解，不选择某个实共轭根。零标签直接取自 WRSD，不通过伪余式重新分类。

```text
weak_split_zero(t,f):
  返回 __weak_split_zero(t,t.convert(f))

__weak_split_zero(t,q):
  assert q 的比较器与 *t.order() 一致
  assert q 只含 t.variables() 中的变量，且指数非负
  若 t.size()=0:
    若 q=0，返回 zero=[t]、nonzero=[]
    否则返回 zero=[]、nonzero=[t]
  z := integer_polynomial(t.order())
  poly_convert(q,z)
  branches := wrsd(t.integer_equations(),z)
  result := regular_chain_split，zero 和 nonzero 均为空
  对 (branches.first,zero=true)、(branches.second,zero=false):
    对其中每条返回链 chain:
      assert |chain|=t.size()
      仅在断言启用且 zero=false 时:
        witness := z
        对 i=|chain|-1,...,0:
          witness := resultant(witness,chain[i],t.variables()[i])
        assert witness!=0
      flags := __squarefree_prefixes(t,chain)
      对应的 result.zero 或 result.nonzero 追加
        __make_regular_chain(t,chain,flags)
  assert result.zero 与 result.nonzero 不同时为空
  返回 result
```

代码中的局部 lambda `append` 执行上述两个分支列表的装配循环。非零分支用逐次结式非零诊断，零分支只使用 WRSD 的零集契约，不要求伪余式为零。诊断在定义 `NDEBUG` 时不执行。每条输出都重新绑定到新对象的比较器。

完整性与递归终止性依赖 XY16 Algorithm 2.2 的 WRSD 契约。调试断言不独立证明分支覆盖，也不作为 Release 正确性的前提来源。各列表内部的链不要求互不相交；零标签与非零标签之间的零点集不相交。

## `__squarefree_prefixes` 与 `__make_regular_chain`：输出属性与装配

WRSD 输出的正则性由文献契约保证，平方自由性则未必已知。内部算法必须建立完整的数据属性：

```text
__squarefree_prefixes(t,equations):
  assert |equations|=t.size()
  squarefree := true；flags := 空列表
  对 i=0..|equations|-1:
    若 squarefree:
      d := discriminant(equations[i],t.variables()[i])
      对 j=i-1,...,0，且 d!=0:
        d := resultant(d,equations[j],t.variables()[j])
      squarefree := (d!=0)
    flags 追加 squarefree
  返回 flags

__make_regular_chain(model,equations,flags):
  前提：equations 是与 model 同坐标、同字典序的正则链；flags 正确描述各前缀
  assert |equations|=|flags|=model.size()
  assert is_regular(equations)
  result := regular_chain()
  result._order := model.order()
  result._vars := model.variables() 的副本
  result._squarefree_prefix := move(flags)
  result._squarefree := 标记列表为空，或其最后一项为 true
  对 i=0..|equations|-1:
    assert equations[i] 的比较器与 *model.order() 一致
    assert equations[i]!=0 且 get_first_var(equations[i])=model.variables()[i]
    q := polynomial_type(result.order())
    poly_convert(equations[i],q)
    result._equations 追加 move(q)
  返回 result
```

一旦某前缀不平方自由，所有更长前缀的累计标记均为 false，因此不再计算后续判别式。此处的属性计算保留在 Release；与之不同，`is_regular(equations)` 只在断言启用时诊断算法输出。对象装配不重新执行公开构造器 `regular_chain(vars,equations,order)` 的外部检查。复制比较器地址与 ZZ→QQ 系数转换保持字典序优先级、三角结构和零集。输出方程独立保存，仍借用同一个外部比较器。

## `squarefree_decomposition`：平方自由分解

输入对象已保证零维正则性，函数无需检查一般序多项式或重建变量表，因此使用单一入口。它负责 QQ→ZZ 方程转换、调用 `RCtoSqrfree`、保存输出的平方自由属性；输出方程按值保存，继续借用外部序。

XZ06 第 855 页对实根隔离要求：每个前缀根上，下一层方程的首项系数和判别式均不为零。对本层的纯方程系统，这对应零维正则且平方自由的三角列。正则性由公开构造器保证，本函数补足平方自由性；它不将任意非正则三角列正则化。一般零维多项式组应先做三角分解和正则化，再逐链平方自由分解，最后调用 `isolate_real_points`。

平方自由是相对于前缀根的性质。例如提升顺序为 `(v0,v1)` 的链 `[v0²−2, v1²−v0²+2]`，第二个多项式在 QQ 多项式环中没有重复因子，但在每个前缀根上变为 `v1²`，仍有重根。因此不能用逐个多项式的普通去重因子替代这里的正则链平方自由分解。

```text
squarefree_decomposition(t):
  若 t.size()=0 或 t.is_squarefree()，返回 [t]
  out := 空列表
  对 RCtoSqrfree(t.integer_equations()) 的每条输出 chain:
    flags := 长度 t.size()、全为 true 的列表
    assert __squarefree_prefixes(t,chain)=flags
    out 追加 __make_regular_chain(t,chain,flags)
  assert out 不为空
  返回 out
```

输出各链正则且平方自由，其复零集之并等于输入零集；次数和方程数量的接口条件由既有分解保证，并由内部断言诊断。单条重复根方程也必须处理，首层修复已单独提交，不在本层重写。Release 直接保存文献保证的全 true 属性，不重算全部判别式来验证同一结论。

已知实现限制：原有 `RCtoSqrfree` 在 `[x²,y³,z²+xy]` 上会重复相同参数而不终止。WRSD 正确返回了未变化的零分支，调用者却没有使递归进展。因此，上述输出条件是本接口需要满足的规格，不能视为当前实现在任意正则链输入上的全称正确性结论。反例与待修范围见[RSD/非约化前缀说明](../../fixes/rsd-nonradical-prefix.md)。

## 实现检查重点

| 代码中的函数或成员 | 需要检查的性质 |
|---|---|
| `regular_chain(vars,equations,order)` | 提升顺序、三角支持、正次数、初式和判别式 |
| 默认复制／移动 | 数据与属性保持，所有方程继续借用外部比较器 |
| `convert(f)` | 变量覆盖与 QQ 值保持 |
| `integer_equations()` → `poly_convert` | 清正分母，仅在方程／判零语义使用 |
| `prefix(n)` / `__prefix(n)` | 外部长度报错、内部断言、前缀属性复用 |
| `weak_split_zero` / `__weak_split_zero` | 外部转换、内部序前提、WRSD 的零集覆盖与标签 |
| `__squarefree_prefixes` / `__make_regular_chain` | 属性计算、契约装配、外部序借用 |
| `squarefree_decomposition` | 空链、首层重根、文献保证的平方自由属性及输出借用关系 |

本层有限的检查、复制和转换循环直接终止；WRSD 的递归终止性依赖 XY16 及其实现前提。`RCtoSqrfree` 在非约化前缀上的进展问题尚未修复，不能直接导入其对所有正则输入终止的实现结论。

## 验证与审核边界

`test/test_regular_chain.cc` 覆盖正则／平方自由属性、重复首层、累计前缀、复制移动及比较器借用。一般入口接收默认 grlex 和不同优先级的 QQ 字典序输入；内部入口直接使用链的 QQ 字典序。构造器在转换前检查原输入的变量和指数，内部断言检查的是链的实际坐标，而非比较器中预留的未来坐标。

新增独立有理点代入验证 `[x²-1,y²-1]` 上 `-2(y-x)/3` 的两类标签，并测试重复坐标、方程数量错误、常数方程、后续变量及未声明变量的拒绝。总头文件测试单独验证公共集成。

```sh
make test/test_regular_chain test/test_clpoly_api test/test_charset
_build/debug/bin/test_regular_chain
_build/debug/bin/test_clpoly_api
_build/debug/bin/test_charset
```

本轮假设已提交底层库满足文献契约，只审核封装、转换和表示。已知底层问题见[RSD／非约化前缀说明](../../fixes/rsd-nonradical-prefix.md)，不作为本层缺陷，也不以普通构建通过宣称底层无条件正确。
