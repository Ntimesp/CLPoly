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

## CAD

公开入口为 `full_cad(input,vars)`，其中 vars 是投影顺序。样本使用 `real_algebraic_point`，通过 `chain/box/point_zero/point_sign` 查询。结果由 `full_cad_result` 存储；提升由自由函数 `__lift_stack` 执行。

### `cad_stack` 与 `full_cad_result`：纯数据接口

`cad_sample` 是 `real_algebraic_point` 的别名，`lazard_evaluation_result` 是 `lazard_qq_result` 的别名。`cad_stack.roots` 按新增坐标递增排列，`evaluations[j]` 对应本层第 j 个投影来源，`sources[i]` 列出第 i 个合并根的全部来源下标。

| `full_cad_result` 接口 | 用途及调用契约 |
|---|---|
| `full_cad_result(order,vars,projection)` | 调用者提供非空比较器所有权及已绑定到它的投影；vars、projection 按投影序输入，原有 `cad_tree` 反转为提升序 |
| `num_levels()`、`level_size(k)`、`leaf_count()` | 层数、每层胞腔数、叶数；零变量空间有一个叶 |
| `node(level,index)`、`structure()` | 只读借用原有树节点或整树，节点边界保留 `cad_root` 来源身份 |
| `sample(level,index)`、`leaf_sample(leaf)` | 只读借用完整实点证书；零变量叶返回空点；对点做可变查询时先复制 |
| `stack(level,parent=SIZE_MAX)` | 只读借用指定父样本根栈；第零层使用唯一虚拟父节点 |
| `root(level,parent,reference)` | 将有限来源身份解析为合并根表中的实际实点；按该来源计数，无穷边界和无效身份报错 |
| `dimension(level,index)`、`signs(leaf)` | 胞腔维数及按原输入顺序保存的符号行 |
| `level_polys(level)`、`level_var(level)`、`order()` | 借用本层投影多项式、新增变量和比较器 |
| `set_signs(signs)` | 转移调用者已计算好的完整符号表，不求值 |
| `add_sector(...)`、`add_section(...)` | 同步追加结构节点、样本、维数；调用者保证样本正确且同父子节点连续，树沿用原有断言契约 |
| `add_stack(level,stack)` | 按父节点顺序追加已完成根栈；第零层只追加一个栈 |

私有 `_order` 共享拥有投影比较器，`_tree` 复用原有树；`_samples`、`_dimensions` 与节点下标一致，`_stacks` 按父节点组织，`_leaf_signs` 按叶组织。复制容器时共享不可变投影比较器，内部实点及其定义链按值复制，但只借用容器持有的同一个比较器。单独复制的样本不能超过该比较器的生命周期；复制整个结果容器会保留比较器的共享所有权。

### `__lift_stack` 与 `full_cad`：实际控制流

`__lift_stack(result,level,parent,alpha,evaluations)` 的调用者保证：父节点按存储顺序处理，alpha 是父胞腔的完整样本，evaluations 与本层投影来源逐项对应并相对于同一个 alpha 解释。

```text
__lift_stack(result,level,parent,alpha,evaluations):
  product := 采用 result.order() 的 QQ 常数 1
  对每个 evaluation，转换其 QQ 代表到同一比较器并乘入 product
  st.evaluations := evaluations
  st.roots := roots_over(product,alpha,result.level_var(level),result.order())
  root_counts := 每个来源已处理不同根数，初值全 0
  对 st.roots 中每个 root:
    sources := 所有满足 point_zero(evaluations[j].polynomial,root) 的来源 j
    若 sources 为空，抛 logic_error
    source := sources.front()
    boundaries 追加 cad_root(source,root_counts[source])
    对 sources 中每个 j，root_counts[j] 加 1
    st.sources 追加 sources
  对 i=0..|st.roots|:
    无根时 q:=0；否则按相邻根盒给出的严格 QQ 开间隙调用 sample_rational
    如有相邻根盒接触，先 shrink 两侧根直到严格分离
    result.add_sector(...,q,append_rational(alpha,本层变量,q,result.order()),父维数+1)
    若右边仍有一个根，result.add_section(...,该根的来源身份,该根样本,父维数)
  result.add_stack(level,st)

full_cad(input,vars):
  检查变量表唯一、覆盖全部输入变量，指数非负
  建立按 vars 排列的投影比较器
  分别保存清正分母的 ZZ 输入与保持原值的 QQ 输入
  projection := __project_full(polys,vars,projection_method::LAZARD,basis_computation_method::FACTOR)
  将投影多项式绑定到结果持有的比较器
  result := full_cad_result(order,vars,projection)
  lifting := reverse(vars)
  对每个 level，按节点存储顺序遍历全部父胞腔:
    a := 父胞腔样本，第零层使用空点
    对本层每个投影多项式 f，计算 lazard_evaluate(f,提升变量前缀,a)
    __lift_stack(result,level,parent,a,全部求值结果)
  对每个叶样本，使用 point_sign 计算原 QQ 输入的完整符号行
  result.set_signs(signs)
  返回 result
```

`full_cad` 接受一般序 ZZ/QQ 多项式，零和常数合法；重复变量、未声明变量及负指数抛 `invalid_argument`。原输入值与清分母后的方程分别保存，避免因子归一化丢失叶符号。投影定理负责胞腔整体上的 delineability 和符号不变，样本上的精确求值不能单独替代这一全局数学前提。

投影保持既有 Lazard + FACTOR 路线；树仍使用 `cad_tree`。每个父样本：

1. 对本层投影多项式调用 QQ Lazard。
2. 将结果 QQ 代表相乘，通过 `roots_over` 隔离所有不同实根；常数因子可保留，零结果属于错误。
3. 根上的 Section 使用 `roots_over` 返回的实点；Sector 在根盒严格分离后，通过独立 `sample_rational` 在 QQ 内间隙选择最简有理数，再由 `append_rational` 添加线性方程。无界间隙取绝对值最小的可用整数；见[采样规格](../rational-sampling.md)。
4. 记录来源多项式及 valuation；对每个来源分别给不同根编号。边界使用 `cad_root(来源索引,该来源根序号)`，共享根对所有来源计数，并选首个来源作为规范身份。
5. 叶节点原输入符号用普通 POINT-SIGN，不能使用 Lazard 值代替。

空变量表的 R^0 返回一个空样本叶；空输入在每层形成单个 Sector；任意有理/代数坐标交替均表示为完整三角列。

## 验证与性能边界

几何测试覆盖 sqrt2、坐标轴、圆、三维消零、代数/有理坐标交替、共享根、QQ 输入和四维坐标分解。通过 QQ 多项式点查询检查样本，通过完整前缀实点比较检查父子关系。实测结果见[验证报告](../../reports/modular-cad.md)。

`test_lazard_qq` 的 19 项与 `test_full_cad` 的 3067 项 Debug 检查通过。完整 CAD 在 `-O2 -DNDEBUG -fno-elide-constructors` 及 AddressSanitizer 下也通过同一组 3067 项检查，覆盖复制后的比较器所有权、混合样本及根来源编号。地址检查使用 Clang 与现有 GNU C++ ABI，另加 `-fdelayed-template-parsing` 兼容原有 `realroot.hh` 的 `uroot` 前向声明顺序；没有为诊断修改可信头文件。插桩覆盖新增头文件和测试，底层既有静态库未插桩，不等价于全库内存验证。

XY16 正规化和平方自由分解保持原契约；随后仅重编码并提升所选前缀，不枚举无关前缀的最后坐标。末坐标根去重时持久化精化区间，再以只读比较器排序。代表乘积仍按 MPP Algorithm 2.12 构造，其次数和项数膨胀属于正式实验需要测量的成本。

## 完整 CAD 的一般入口与字典序内核

`full_cad` 是有检查的一般序入口：检查变量表唯一、变量覆盖与非负指数，并分别转换为同一比较器下的 ZZ 投影输入和原值 QQ 输入，再调用 `__full_cad`。内部调用方保证二者逐项对应、ZZ 输入为原 QQ 输入的正倍数、比较器与变量表一致；内核仅用 assert 诊断。

`__lift_stack` 是内部栈提升算法，不是稳定的公开输入入口。驱动在统一共享比较器下准备 QQ Lazard 代表，调用 `__lazard_evaluate_qq`，避免每父每因子重复校验和建立变量序。每个来源转换一次后用于根来源判零，乘积使用 `__roots_over`，原输入叶符号使用 `__point_sign`。这些内核只接收已满足契约的 QQ 字典序多项式。

`full_cad_result` 只保存结构和数值证书。公开构造器在构建底层树之前检查层数、变量优先级、投影变量范围及多项式与所持比较器的绑定；非法输入抛 invalid_argument。低层追加接口由构建算法保证语义，用断言检查样本维数、根来源表形状及符号行数，不执行求根或采样。
