# 完整 CAD 与实代数点 API

设计入口：[独立模块](../modular-cad/README.md)。

## 区间与三角列

```cpp
#include <clpoly/rational_interval.hh>
rational_interval a(QQ(1), QQ(2));
auto b = reciprocal(a);                 // [1/2,1]
auto both = intersection(a,b);          // optional，端点相交也是非空

#include <clpoly/regular_chain.hh>
const regular_chain::order_type order(custom_var_order({z,y,x}));
regular_chain T({x,y}, std::vector<polynomial_ZZ>{x*x-2, y*y-x}, &order);
// 这里 {x,y} 是提升顺序；目标序末尾是 {y,x}，预留 z 用于后续提升。
// order 必须比 T、各分支及生成的实点活得更久，且使用期间不改变。
auto prefix = T.prefix(1);
auto parts = weak_split_zero(T, polynomial_ZZ(y-x));  // WRSD：zero / nonzero

// 内部入口：q 已采用 T 的 QQ 字典序，变量覆盖和规范化由调用者保证。
auto q = T.convert(polynomial_ZZ(y-x));
auto direct = __weak_split_zero(T, q);
```

一般入口 `weak_split_zero` 负责检查和序转换，非法变量抛 `std::invalid_argument`；内部入口 `__weak_split_zero` 只用断言。`squarefree_decomposition` 直接接收有效正则三角列对象，完成平方自由分解和输出装配，采用单一入口。`T.prefix(n)` 检查长度，`T.__prefix(n)` 要求调用者保证长度合法。

## 有理开区间采样

```cpp
#include <clpoly/rational_sample.hh>
auto q = sample_rational(QQ(1,3), QQ(1,2));    // 2/5，严格排除端点
auto left = sample_rational(std::nullopt, QQ(-7,3)); // -3
auto right = sample_rational(QQ(7,3), std::nullopt); // 3
auto whole = sample_rational(std::nullopt, std::nullopt); // 0
```

按既约分母最小、其次分子绝对值最小选点；有限端点不满足 left<right 时抛 `std::invalid_argument`。这是开区间接口，不接受 `rational_interval` 闭单点。算法仅依赖现有 QQ；CAD 通过根盒提供 QQ 内间隙，其最简性范围和证明见[采样规格](../rational-sampling.md)。

## 实代数点

```cpp
#include <clpoly/real_algebraic.hh>
auto points = isolate_real_points(T);    // 要求 T 平方自由正则
for (auto& a : points) {
    a.refine(QQ(1,1000));                // 每维宽度 < 1/1000，保留同一点
    bool zero = point_zero(polynomial_ZZ(y*y-x), a);
    int sign = point_sign(polynomial_ZZ(y), a);
    const auto& box = a.box();          // 只读 QQ 区间
}
auto finer_chains = squarefree_decomposition(T);
auto roots = roots_over(polynomial_ZZ(z*z-y), points.back(), z, &order);
auto sector = append_rational(points.back(), z, QQ(2,3), &order);
```

`real_algebraic_point` 持有链、隔离盒及每层平移证书；链只借用调用者提供的比较器。普通构造 `(chain,box[,shifts])` 检查正则平方自由链及严格上下界/导数证书；任意宽隔离盒可能被拒绝，请优先使用隔离器。点查询可能精化传入的非 const 点；复制后查询可保留原盒。

`same_real_point(a,b)` 比较同一有序变量表上的两个点；`reencode_point(a,T)` 在另一条含该点的正则链上重新生成平方自由证书。

实代数模块提供 CAD 所需的代数点和 QQ 多项式值查询；任意两个独立代数数之间的通用域合并/四则数值类不在接口范围内。

## Lazard 与 CAD

```cpp
#include <clpoly/full_cad.hh>
auto value = lazard_evaluate(f, lifting_vars, point);
// value.polynomial：QQ 代表，仍保留冻结变量；value.valuation：逐变量阶数
// point 必须由调用方保留；value 不保存点，也不能换用另一个共轭点解释

auto cad = full_cad(std::vector<polynomial_ZZ>{x*x+y*y-1}, {y,x});
auto a = cad.leaf_sample(0);            // real_algebraic_point，可复制后精化
int s = cad.signs(0)[0];
size_t dim = cad.dimension(1,0);
```

外部 `vars` 是投影顺序；`sample.chain().variables()` 和盒均按反向的提升顺序。例如 `full_cad(...,{x1,x2,x3})` 先投影消去 x1、x2，再按 x3、x2、x1 提升；其三角列接收 `{x3,x2,x1}`，内部比较器恢复为 `{x1,x2,x3}`，主变量仍取比较器下最小变量。`sample(level,index)`、`leaf_sample`、`node`、`dimension`、`signs`、`level_polys`、`stack`、`root` 和 `structure` 可查询。单独复制出的 CAD 样本继续借用结果容器的比较器，使用期间须保留该结果或其副本。

每个 stack 的 `roots` 是包含前缀和新增坐标的实代数点列表，`sources` 记录本层来源，`evaluations` 保存 QQ Lazard 结果。树边界 `cad_root(p,r)` 是第 p 个层投影多项式在该父样本上 Lazard 求值后的第 r 个不同实根；共享根为每个来源独立计数，存储时选首个来源。使用 `root(level,parent,reference)` 解析证书，不能把 r 直接用作合并根表下标。

零多项式可作为 CAD 输入（符号恒零），但不能作为 Lazard 求值输入；`roots_over` 的恒零纤维抛异常，非零常量返回空根列表。重复变量、未声明变量及非法证书明确拒绝。

## 数据结构与算法入口

包含 `clpoly/lazard_qq_result.hh` 可单独使用 Lazard 结果值类型，不引入求值算法；包含 `clpoly/full_cad_result.hh` 可使用 `full_cad_result`、`cad_stack` 数据类型；包含 `clpoly/full_cad.hh` 使用 `full_cad(...)` 和自由函数 `__lift_stack(result, level, parent, sample, evaluations)`。一般调用方只需使用 `full_cad(...)`。

结果类的 `add_sector`、`add_section`、`add_stack`、`set_signs` 是底层构建操作，要求调用者维持层序/父节点顺序、完整子块、样本和维数正确、栈表与父节点一一对应；它们不代替算法验证。`structure()` 保留原 cad_tree 的结构和来源根身份；完整 CAD 的边界证书通过 `root` 结合指定父栈解析。

实点精化支持 `clpoly::shrink(a)`、`clpoly::refine(a, epsilon)`；`a.shrink()`、`a.refine(epsilon)` 仅转发到这两个自由函数。

## 完整示例

```cpp
#include <clpoly/full_cad.hh>
#include <iostream>

int main()
{
    using namespace clpoly;
    variable x("x"), y("y");
    const regular_chain::order_type order(custom_var_order({y,x}));
    regular_chain chain({x}, std::vector<polynomial_ZZ>{x * x - 2}, &order);
    auto points = isolate_real_points(chain);
    for (auto& point : points)
    {
        refine(point, QQ(1, 1000));
        std::cout << point_sign(polynomial_ZZ(x), point) << '\n';
    }

    // 投影序 y,x；提升序 x,y。
    auto result = full_cad(std::vector<polynomial_ZZ>{x * x + y * y - 1}, {y, x});
    std::cout << result.leaf_count() << '\n'; // 13
}
```

使用项目编译器、头文件路径及 GMP 链接设置，链接 `libclpoly`。上文片段中的 x、y、z 是调用方创建的 `variable`；示例之外的 f、point、lifting_vars 分别是待求值多项式、已认证前缀实点及其完整提升变量表。
