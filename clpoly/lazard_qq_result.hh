/**
 * @file lazard_qq_result.hh
 * @brief Lazard 求值结果的数据表示
 *
 * 保存 QQ 代表和逐坐标阶数，点上下文由调用方保留。
 * 规格见 docs/cad/design/modular-cad/lazard-and-cad.md。
 */
#ifndef CLPOLY_LAZARD_QQ_RESULT_HH
#define CLPOLY_LAZARD_QQ_RESULT_HH

#include <clpoly/polynomial.hh>
#include <memory>
#include <stdexcept>
#include <vector>


namespace clpoly{

    // 结果只保存有理代表和阶数；解释代表时必须使用生成它的同一个前缀实点。
    struct lazard_qq_result
    {
        using order_type = lex_<custom_var_order>;

        // 代表多项式借用此比较器。复制结果共享其不可变存储，避免比较器悬空。
        // 不能在保留 polynomial 的同时重置 order 或改绑到另一种变量序。
        std::shared_ptr<const order_type> order;
        polynomial_<QQ, order_type> polynomial; // 保留冻结变量的 QQ Taylor 系数代表。
        std::vector<size_t> valuation; // 按前缀提升顺序记录每个坐标的首次非零阶数。

        // o 必须非空；先持有比较器，再建立借用它的零代表，供求值算法填充。
        explicit lazard_qq_result(std::shared_ptr<const order_type> o)
            : order(std::move(o)),
              polynomial(order ? order.get() :
                  throw std::invalid_argument("lazard_qq_result: null order"))
        {
        }
    };
}
#endif
