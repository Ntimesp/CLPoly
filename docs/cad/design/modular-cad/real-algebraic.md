# 实代数点层：实现细化

依赖独立三角列与区间模块；实现依据理论 PDF的 XZ-LIFT、NSHR-QQ、ISOLATE-REGULAR、POINT-ZERO/SIGN。

## 数据

`real_algebraic_point` 保存 `regular_chain`、QQ 严格根隔离盒和每层固定有理平移；平移后的根盒位于正半轴。方程始终 QQ 系数；每个坐标由对应前缀上的单根定义。只有能够认证上下界多项式在隔离区间两端严格异号、且两界导数同号的构造才有效。普通区间对象不冒充根证书。

根盒按 `chain.variables()` 的提升顺序绑定坐标。例如 CAD 投影输入为 `{x0,x1,x2}` 时，内部盒的三项分别对应 x2、x1、x0；读取盒必须同时读取变量表。记投影序 X、提升序 L，则 `L=reverse(X)`，`box[i]` 绑定 `L[i]`。原三角列分解算法只处理方程和比较器，不接收根盒。

自由函数 `refine(a,epsilon)` 调用 NSHR-QQ 直到每维宽度小于 epsilon；不修改方程或选择不同根；没有固定迭代上限。空点是合法根上下文。前缀截取提升坐标表、继续借用同一个外部比较器，并继承已验证的链/盒证书，不重算旧前缀的结式和上下界。

## 实际数据接口与所有权

| 接口／成员 | 用途、前提和返回契约 |
|---|---|
| `real_algebraic_point()` | 零坐标的唯一空点，作为提升起点 |
| `real_algebraic_point(chain,box,shifts={})` | 公开构造；要求链平方自由正则、盒为严格区间、尺寸一致，并逐层验证正半轴及上下界证书；非法输入抛 `invalid_argument` |
| `_chain`、`_box`、`_shifts` | 私有数据：定义链、原坐标根盒及固定平移；下标均按提升方向对应 |
| `chain()`、`box()`、`shifts()` | 只读借用；调用者保留原点对象，避免在修改对象后继续使用已失效的容器引用 |
| `size()`、`max_width()` | 坐标数及各维最大区间宽度；空点分别为 0 和 QQ 零 |
| `prefix(n)` | 返回前 n 维的独立证书；复用累计属性和已有平移，越界抛 `out_of_range` |
| `shrink()`、`refine(epsilon)` | 便捷成员，分别转发到同名自由函数；算法定义在数据类型之外 |

定义链只借用外部比较器，所有点及其副本、前缀和转换结果均须在该比较器存活且变量序不变时使用。`append_rational` 与 `roots_over` 显式接收扩展坐标所用的比较器地址，禁止返回借用局部临时比较器的点。CAD 调用统一传入结果容器持有的比较器。

## 实际算法入口

| 函数 | 输入及效果 |
|---|---|
| `shrink(a)` | 原地进行一次分层保根收缩，保持链、平移和所选实点 |
| `refine(a,epsilon)` | epsilon 必须为正；原地收缩至各维宽度严格小于 epsilon |
| `__lift_real_points(chain,prefix)` | 内部入口；调用者保证平方自由正则链比已认证前缀多一维，且定义方程前缀相同；返回该前缀上全部末层实根 |
| `isolate_real_points(t)` | 平方自由正则链的全部实点；非平方自由输入抛异常，无实根返回空表，空链返回一个空点 |
| `point_zero(f,a)` | 精确判定 f 在所选点是否为零；可精化 a，接受一般序 ZZ/QQ 输入 |
| `point_sign(f,a)` | 返回 -1、0 或 1，可精化 a；区间包络含零时不直接判为零 |
| `point_on_chain(t,a)` | 要求坐标表相同，逐方程判断 a 是否属于 t；可精化 a |
| `same_real_point(a,b)` | a 按值传入，只精化局部副本；成员关系和严格盒包含／分离共同决定是否同点 |
| `reencode_point(a,chain)` | 保持指定点，更换定义链和证书；新链须包含该点，可以含其他根或重根 |
| `append_rational(a,v,value,order)` | v 必须未出现；追加方程 `v-value`、严格根区间和固定平移，返回新的完整实点 |
| `roots_over(input,a,v,order)` | v 为新变量；返回指定前缀上全部互异实根，按末坐标排序；恒零特化抛 `domain_error`，非零常数返回空表 |

## `xz_detail` 中的一元与区间辅助函数

`roots_over` 的系数扫描只需要 WRSD 零集契约；其后续平方自由化仍依赖 `squarefree_decomposition` 完成并满足规格。原有分解在 `[x²,y³,z²+xy]` 上不终止，故任意输入纤维接口的实现终止性不能仅由点查询替换成功推出。详见[非约化前缀问题](../../fixes/rsd-nonradical-prefix.md)。

该命名空间隔离 `eval`、`integer` 等局部辅助名称。`P` 是 QQ 定义多项式，`U=std::vector<QQ>` 是次数递增的一元系数表，`I` 是 `rational_interval`；U 的零表和常数表遵守各入口的明确约定。

| 实际函数 | 输入前提与效果 |
|---|---|
| `eval(f,QQ)` / `eval(f,I)` | 稠密 Horner 点值或区间包络；空表表示零 |
| `derivative_u(f)` | 稠密 QQ 一元形式求导；常数导数为空表 |
| `coefficients(f,v)` | 按 v 的全部次数收集系数，多项式规范化、指数非负；返回系数借用原比较器 |
| `translate(f,v,offset)` | 展开 `f(...,v+offset)`；仍为 QQ 多项式，返回值借用 f 的比较器 |
| `bounds(f,v,vars,box)` | 前缀按提升方向传入；返回 QQ 一元上下界，仅在新坐标为正时具有夹逼及导数夹逼性质 |
| `integer(f)` | 清正分母，转为原有 ZZ 一元多项式，保持根集、重数和符号 |
| `squarefree(f)` | 以二参数 GCD 判断重根；零为 false，非零常数为 true |
| `strict_sign(s)` | 只接受 -1 和 1，排除区间 unknown 与零 |
| `bracket(f,b,epsilon)` | 调用者保证 b 含唯一单根；检查端点严格异号和正精度，返回宽度至多 epsilon 的严格子括区间 |
| `univariate(f,epsilon)` | 平方自由非零 QQ 一元式的全部实根；补入零根，认证严格括区间、导数及互异根分离 |
| `certified(bounds,interval)` | 调用者保证合法上下界及正半轴区间；检查两界导数同号、端点严格异号，不执行精化 |
| `lift(f,v,a,used_shift)` | 调用者保证指定前缀处首项非零、特化平方自由；可精化 a，输出全部有序原坐标根区间和成功采用的固定平移 |

## 按代码名称给出的主要控制流

```text
shrink(a):
  对 i=0..a.size()-1:
    q := xz_detail::translate(a.chain().equations()[i],a.chain().variables()[i],-a.shifts()[i])
    b := xz_detail::bounds(q,本层变量,当前已缩小的前缀变量,当前已缩小的前缀盒)
    old := a.box()[i] 平移到正半轴
    epsilon := old.width() / (i=0 时为 2，否则为 8)
    l := xz_detail::bracket(b.first,old,epsilon)
    u := xz_detail::bracket(b.second,old,epsilon)
    a._box[i] := hull(l,u) 平移回原坐标

refine(a,epsilon):
  若 epsilon<=0，抛 invalid_argument
  当 a.max_width()>=epsilon:
    shrink(a)

__lift_real_points(chain,prefix):
  assert chain 平方自由、比 prefix 多一维且定义前缀相同
  intervals := xz_detail::lift(chain.equations().back(),chain.variables().back(),prefix,shift)
  对每个 interval:
    point._chain := chain
    point._box := prefix.box() 后追加 interval
    point._shifts := prefix.shifts() 后追加 shift
    追加 point
  返回全部新点

isolate_real_points(t):
  若 !t.is_squarefree()，抛 invalid_argument
  points := [空点]
  对 i=0..t.size()-1:
    chain := t.prefix(i+1)
    对 points 中每个 prefix，调用 __lift_real_points(chain,prefix)
    points := 合并本层得到的全部新点
  返回 points
```

```text
point_zero(f,a):
  branches := weak_split_zero(a.chain(),f)
  循环:
    按字典序优先级同步排列 a 的变量与根盒
    对两个分支列表，删除存在某方程 g 满足 __assign_interval(g,vars,box) 排除零的分支
    若两类同时为空，抛 logic_error
    若 nonzero 为空，返回 true
    若 zero 为空，返回 false
    a.refine(a.max_width()/2)

point_sign(f,a):
  q := a.chain().convert(f)
  若 __assign_interval(q,有序变量,有序根盒) 给出严格符号，返回该符号
  若 point_zero(q,a)，返回 0
  反复精化 a，直到同一 q 的区间值给出严格符号，返回该符号

reencode_point(a,chain):
  相同坐标且相同定义方程时返回 a
  若 !point_on_chain(chain,a)，抛 invalid_argument
  若 chain 为空，返回 a
  对 squarefree_decomposition(chain) 中包含 a 的 component:
    prefix := reencode_point(a.prefix(a.size()-1),component.prefix(a.size()-1))
    对 __lift_real_points(component,prefix) 中每个 candidate:
      若 same_real_point(candidate,a)，返回 candidate
  抛 logic_error

roots_over(input,a,v,order):
  检查 order 非空、v 未出现、input 变量覆盖与非负指数，并转换到 order 指定的字典序
  order 必须支持扩展后的连续提升坐标，返回点借用该外部序
  cs := xz_detail::coefficients(f,v)；selected := a.chain()
  从最高次系数向下:
    branches := weak_split_zero(selected,cs[j])
    以 point_on_chain 选取包含 a 的分支
    若选到非零分支，记录实际次数 j 并停止；否则继续下一系数
  恒零特化抛 domain_error；非零常数返回空表
  删除已在 selected 上为零的高次项，将截断后的方程追加到 selected，以 regular_chain(vars,equations,order) 建立扩展链
  对 squarefree_decomposition(extended) 中每条 t:
    若 t 的旧坐标前缀不含 a，跳过
    prefix := reencode_point(a,t.prefix(a.size()))
    对 __lift_real_points(t,prefix) 中的候选根，使用 same_real_point 精确去重
    对互异但根盒相交的候选持续精化，并保留缩小后的盒
  按末坐标区间左端点排序，返回全部互异根
```

上述伪代码只简写容器装配和局部 lambda 的循环，所有被调用的函数均使用代码中的名称。数值证书的数学证明及 `xz_detail::lift`、`univariate` 的界条件见理论 PDF 与下文；此处不增加另一组自定义算法名。

## 可执行子程序

- `xz_detail::univariate`（PDF 的 UNI-QQ）接收非零平方自由 QQ 一元多项式和正精度；常量返回空列表，重根输入与非正精度明确拒绝。清正分母后调用可信标量 `uspensky` 取得非零根，若常数项为零则显式补入零根，再统一转换为严格括区间、认证导数并排序分离。这不是代数系数多项式。
- 平方自由检查清正分母得到整系数一元多项式 z，复用库中二参数 `polynomial_GCD(z,derivative(z))`，仅检查结果是否为非零常数，不计算未使用的 Bézout 系数。清分母和 GCD 的非零常数因子均不改变重根判据；零多项式返回 false，非零常量返回 true。
- `xz_detail::bracket`：唯一单根严格括区间二分；恰好命中有理根时保留根两侧的严格有理端点。初始 Uspensky 单点或端点根转换成这样的括区间，并认证导数非零。
- 有理平移先对 QQ 多项式展开，再估值系数；不能先生成负半轴上的界多项式再声称其夹逼成立。
- `xz_detail::lift` 使用完整实根列表检查上下界次数、平方自由、正根数、逐序配对、区间分离和共同导数符号；不满足时同时提高一元精度并精化前缀。
- `shrink` 对每个已精化前缀重算上下界，在旧区间内各取宽度至多 1/8 的根括区间，再取 hull；第一维至多减半。
- `point_zero` 通过 `weak_split_zero` 直接取得两类分支标签；只用“某方程区间排除零”删除分支，若尚存两类则精化原点。零集覆盖保证包含原点的分支不会丢失，有限个错误分支最终被区间收敛排除；正确性及终止性不依赖零分支上的伪余式为零，也不创建扩域系数。
- `point_sign` 先做区间估值，得到严格符号便直接返回；不确定时精确判零，再对非零值精化。区间包含零不能直接返回零。

## 纤维根接口

`roots_over` 依赖点精化、精确成员关系、重编码和 XZ06 单前缀提升，负责任意非零 QQ 代表在指定点上的全部纤维根：先由 WRSD 限制次数分支，再将扩展链平方自由分解；筛去不含所选前缀的分支，在保留分支上重编码同一点，只提升最后坐标。相同候选精确去重，互异候选持久化精化至末坐标区间不相交，再按端点排序。

`reencode_point` 按维数递归：相同规范化方程直接复用证书；否则选择含该点的平方自由分支，先重编码其低一维前缀，再提升最后坐标，并以方程成员关系及严格盒包含/分离选回原点。不调用全链实点枚举，也不假设新链的全部零点都在旧链上。

`__lift_real_points(chain,prefix)` 只供已满足前置条件的内部算法使用：链平方自由正则，比前缀多一维，前缀定义方程与链的低一维前缀相同。它组合旧前缀和新层 XZ-LIFT 证书，不提供绕过验证的公开实点构造。全链 `isolate_real_points` 则在每层对所有已隔离前缀调用此函数。

`xz_detail::bounds` 对提升顺序的变量和区间同步反转一次，得到比较器的投影优先级，再调用字典序估值核心。例如点的坐标及盒为 `(x2,x1)` 和 `(I2,I1)` 时，核心绑定必须为 `(x1,x2)` 和 `(I1,I2)`；不能只反转变量表。系数按已有字典序分组后直接使用。公开实点构造继续验证外部提供的完整证书。

## 验收反例

正/负 sqrt2、嵌套 sqrt(sqrt2)、有理根和零根、可约链上所选点为零但商环非零、共轭符号相反、精化不漂移、无实根、非法宽盒/重复根证书。与 PDF 逐算法核对，禁止以根的界多项式替换实际定义三角列。

原有 171 项证书和纤维测试保留；本轮另增加一般序转换、直接字典序内核和非法坐标测试。测试不替代各算法的数学证明。

## PDF 算法与函数对应

| PDF 编号 / 算法 | 实现 | 保持的性质 |
|---|---|---|
| 1 XZ-LIFT | `xz_detail::lift` | 先认证首项非零；严格根界平移；QQ 下/上界；次数、符号、平方自由、完整根数、配对、导数和分离检查；不满足则同时提高一元精度、缩前缀 |
| 2 NSHR-QQ | `shrink(real_algebraic_point&)` | 按前缀从低到高；保持成功隔离时的平移；上层两界根括区间各缩至旧宽的 1/8，再取 hull |
| 3 REFINE | `refine(real_algebraic_point&, epsilon)` | 只接受正精度；无固定次数上限；每维宽度达到要求 |
| 4 ISOLATE-REGULAR | `isolate_real_points` | 所有前缀逐层全部提升；空链单个空点；第一级走空前缀的 XZ-LIFT，此时 L=U，为经过有理平移的同一个 UNI 调用，结果平移回原坐标，等价于 PDF 首层一元隔离 |
| 5 POINT-ZERO | `point_zero` | WRSD 标签；仅用某方程的区间排除零删除分支；始终精化原点，绝不从相交推断包含 |
| 6 POINT-SIGN | `point_sign` | 严格区间符号直接返回；不确定时精确判零，非零后细化至严格同号 |
| 9 REENCODE | `reencode_point` / `same_real_point` | 相同方程复用证书；否则先验证成员关系、平方自由化，再按维数递归重编码前缀并提升最后坐标；成员关系和严格盒匹配选回同点 |
| 10 ROOTS-QQ | `roots_over` | 逐系数 WRSD 选分支；处理点上降次；首项在整个分支非零后追加方程；平方自由化；筛选含指定前缀的分支并重编码；仅提升最后坐标；精确去重并持久化分离，再以只读端点排序 |
| 11 UNI-QQ | `xz_detail::univariate` | 检查正精度及平方自由；明确补入零根；严格括区间、导数认证、排序和分离 |

XZ-LIFT 的上下界数组只表示 QQ 一元界多项式，根证书始终保存实际定义三角列。有理平移构造的多项式须持有有效比较器，成功时采用的平移随证书保存。点查询可能精化证书，但不改变所选点。

## 指定纤维的完整性与终止性

WRSD 的次数分支与 RCtoSqrfree 分解保持所选纤维根集的并。不含指定前缀的分支不贡献目标根；含该前缀的分支经 REENCODE 后仍表示同一点。单前缀 XZ-LIFT 对每个保留分支返回全部最后坐标根，因此只提升所选前缀不会丢失目标根。精确去重仅移除相同点，精化保持点值，输出仍是全部互异根。

REENCODE 每次递归降低维数；各层分解有限，隔离与同点判定按其规格终止。有限个互异末坐标根间有正距离，保根区间收敛最终将其严格分离；排序比较器随后只读端点，并对同一点返回 false。

公共构造验证完整外部证书。内部单层提升仅组合已验证的前缀证书和新层 XZ-LIFT 证书；prefix 只截取已验证证书。这些内部路径须保持与公共构造相同的 PointRep，不将内部复用接口暴露成无检查的公开构造。

## 本轮接口分层与效率审核

公开 `point_zero`、`point_sign` 先检查变量覆盖、指数并转换为链的 QQ 字典序，再分别调用 `__point_zero`、`__point_sign`；内核仅用断言，不重复转换。`__point_sign` 的判零路径直接调用字典序内核，`point_on_chain` 也复用同序方程。

公开 `roots_over` 先检查新坐标、完整提升坐标与目标比较器的反向连续后缀关系，以及原输入的变量和指数，再转换并调用 `__roots_over`。内部根表始终相对于同一个前缀点解释。零纤维的 domain_error 是精确求值后的查询结果，不是重复的一般序输入校验。

`__isolate_real_points`、`__refine` 分别接收调用者保证平方自由的链和正精度；公开入口保留运行时检查。内部系数收集要求新主变量具有最高优先级，按已有字典序分组后移除共同主变量幂，每组仍规范化，无需再排序。XZ-LIFT 确认上下界平方自由后调用 `__univariate`，避免 Release 构建中重复 GCD；有检查的 `xz_detail::univariate` 适配入口保留给独立验证。

类负责证书存储、只读查询、前缀截取及外部构造有效性校验；隔离、精化和所有点查询由自由函数执行，同名成员仅转发。内部单层装配断言前缀方程及坐标与目标链一致。

`test_real_algebraic` 另覆盖一般 grlex QQ 输入、直接 QQ 字典序判零／符号／纤维内核、缺失变量、空序指针和不兼容的目标序。`test_clpoly_api` 仅通过库总头文件验证隔离与查询。数学正确性以 XY16 分解契约、XZ06 §3 的正则平方自由和证书条件为前提；不将已提交底层缺陷计入本轮上层问题。

## 历史单纤维性能诊断（2026-10-22）

用 `T=[product(x-i,i=1..n)]` 的最后一个实点 x=n 求 y-x 的根。相同输入和构建配置，各版本交替执行五次，取函数内计时的中位数。前缀初始隔离不计入计时。对照为全链隔离后筛选前缀的实现快照；当前路径为指定前缀提升及证书复用。测量包含这组实现差异，不把收益仅归因于一个函数。

| n | 全链路径最终候选数 | 指定前缀最终候选数 | 返回根数 | 全链路径 ms | 指定前缀 ms |
|---:|---:|---:|---:|---:|---:|
| 1 | 1 | 1 | 1 | 0.106 | 0.072 |
| 2 | 2 | 1 | 1 | 0.224 | 0.069 |
| 4 | 4 | 1 | 1 | 1.778 | 0.088 |
| 8 | 8 | 1 | 1 | 42.335 | 0.229 |
| 12 | 12 | 1 | 1 | 416.765 | 0.447 |

候选数通过临时诊断副本的返回列表长度计数确认，计数未进入生产头；时间来自未加计数的版本。GCC 13.4.0、arm64、探针翻译单元 `-O2 -DNDEBUG`，链接既有 Debug 静态库。此例说明无关前缀与重复认证开销已去除，不构成全库或大规模 CAD 的普遍加速承诺。

### 054：导数证书的多轮精化与终止检查

最简有理采样接入后，054 首个 y 纤维的前缀为 `z=-3`，初始盒 `[-4,-2]`，固定平移为 4259。乘积关于 y 的次数为 9。精确代入 z=-3 后，用可信普通 GCD 检查得到 `deg(gcd(f,f'))=0`，因此目标根均为单根。

临时跟踪中，第 50 轮仍有 `derivative-signs=2,2`。2 是区间符号 unknown，表示包络含零，并不表示两个导数真正异号。每轮都改变前缀盒、上下界多项式和根括区间，不能将一次不通过视为永久不通过。

对同一纤维单独运行原扩展 GCD 路径，实际在第 113 轮全部通过，返回 9 个根，带跟踪耗时约 114.94 秒。另一个诊断副本只将平方自由检查换成库中普通二参数 GCD，保持根隔离、精化和接受条件相同，同样在第 113 轮返回相同的 9 个 QQ 括区间，带跟踪耗时约 7.45 秒。两次运行的精化轮次、界系数位数、配对根数和全部证书判断逐条相同。这是改用普通 GCD 之前的成本诊断，不将带跟踪耗时作为正式基准加速比。正式实现已采用普通 GCD，复测见[验证记录](../../reports/modular-cad.md)。

区间过估计可独立复现。该纤维含原坐标 y=0 这个根，导数恰为 72；平移后该根为 t=4259。对固定极限多项式的导数，用展开系数的区间 Horner 求值：在 `4259±2^-50` 上得到约 `[-2.34446e14,2.34446e14]`，符号未知；在 `4259±2^-100` 上约为 `[71.79177,72.20823]`，严格为正。计算使用精确有理数，近似小数只用于显示。这说明很细的输入区间仍可能因平移展开和变量相关性损失而产生很宽的包络；该示例说明一个已知根的区间过估计，不冒充第 50 轮实际失败配对区间。

一般终止证明仍须同时使用：指定前缀上的目标多项式首项非零且平方自由；前缀保根精化收敛；一元括区间精度趋零；固定表达式的区间估值在点处收敛。由简单根处导数非零可得最终严格同号。若这些前提被破坏，确实可能无法终止；不能单用“继续精化”代替该证明，也不能靠设置固定轮数上限伪造完整性。

原实现的主要成本包括每轮最多四次平方自由检查及其中未使用的 Bézout 更新：XZ-LIFT 先检查两界，UNI 又对相同两界各检查一次。原路径的跟踪在结束前记录平方自由检查累计约 106.9 秒，其中扩展 GCD 内的 Bézout 更新约 89.9 秒；这些是包含关系，不能相加。当时已改用普通 GCD，同轮重复检查仍保留。2026-11-06 的逐功能复核另已将 XZ-LIFT 转接到 `__univariate`，Release 不再重复同轮检查；Debug 断言仍核对平方自由前提。上述耗时对应历史快照，尚未按新版本复测。后续研究减少区间过估计时仍须保持证书条件和 XZ06 契约。

诊断源文件、原始日志、两条路径一致性检查和精确区间示例保存在 `/private/tmp/clpoly-054-termination-20261022/`；临时文件可能被清理。这里只验证这条纤维实际终止，不能据此代替全部实现的终止性证明。
