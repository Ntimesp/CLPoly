/**
 * @file full_cad_result.hh
 * @brief 完整 CAD 的存储与查询接口
 *
 * 树结构、样本、根栈、维数和符号按相同节点下标组织。
 * 规格见 docs/cad/design/modular-cad/lazard-and-cad.md。
 */
#ifndef CLPOLY_FULL_CAD_RESULT_HH
#define CLPOLY_FULL_CAD_RESULT_HH

#include <clpoly/cad_tree.hh>
#include <clpoly/real_algebraic.hh>
#include <clpoly/lazard_qq_result.hh>
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace clpoly{

    using cad_sample = real_algebraic_point;
    using lazard_evaluation_result = lazard_qq_result;

    // 一个父样本上的完整根栈，根按新增坐标递增排列。
    // evaluations[j] 对应本层第 j 个投影来源，sources[i] 列出第 i 个合并根的全部来源。
    struct cad_stack
    {
        std::vector<real_algebraic_point> roots; // 完整前缀及新增根坐标。
        std::vector<lazard_evaluation_result> evaluations; // 在同一父样本上的 Lazard 代表。
        std::vector<std::vector<size_t>> sources; // 与 roots 一一对应，每项非空。
    };

    // 纯数据容器：树给出胞腔结构，其余表按同一层、同一节点下标组织。
    // 投影、根隔离、分支选择和符号计算由 full_cad.hh 中的自由函数完成。
    class full_cad_result
    {
    public:
        using order_type = lex_<custom_var_order>;
        using poly_type = polynomial_<ZZ, order_type>;

    private:
        std::shared_ptr<const order_type> _order; // 投影、样本链和根栈共用的不可变比较器所有权。
        cad_tree<custom_var_order> _tree; // 复用原有按层存储的树结构。
        std::vector<std::vector<cad_sample>> _samples; // 每个胞腔的完整提升前缀样本。
        std::vector<std::vector<cad_stack>> _stacks; // 每层按父节点顺序保存根栈。
        std::vector<std::vector<size_t>> _dimensions; // 与每层样本和节点一一对应。
        std::vector<std::vector<int>> _leaf_signs; // 每行是原输入顺序的叶胞腔符号。

    public:
        // vars、projection 按投影方向输入，cad_tree 内部反转为提升方向。
        // 检查形状、变量优先级和比较器绑定；持有共享所有权使结果可复制。
        full_cad_result(std::shared_ptr<const order_type> order, std::vector<variable> vars,
                        std::vector<std::vector<poly_type>> projection)
            : _order(std::move(order))
        {
            if (!_order || vars.size() != projection.size() ||
                _order->comp.v_map.size() != vars.size())
                throw std::invalid_argument("full_cad_result: inconsistent projection shape");
            for (size_t i = 0; i < vars.size(); ++i)
            {
                auto position = _order->comp.v_map.find(vars[i]);
                if (position == _order->comp.v_map.end() ||
                    position->second != static_cast<int64_t>(i+1))
                    throw std::invalid_argument("full_cad_result: incompatible variable order");
                for (const auto &p : projection[i])
                {
                    if (p.comp_ptr() != _order.get() || is_number(p) || get_first_var(p) != vars[i])
                        throw std::invalid_argument("full_cad_result: invalid projection layer");
                    for (const auto &term : p)
                        for (const auto &ve : term.first)
                        {
                            auto position = _order->comp.v_map.find(ve.first);
                            if (ve.second < 0 || position == _order->comp.v_map.end() ||
                                position->second < static_cast<int64_t>(i+1))
                                throw std::invalid_argument("full_cad_result: invalid projection variable");
                        }
                }
            }
            _tree = cad_tree<custom_var_order>(std::move(vars), std::move(projection));
            _samples.resize(_tree.num_levels());
            _stacks.resize(_tree.num_levels());
            _dimensions.resize(_tree.num_levels());
        }

        // 提升层数等于变量数；无变量时为零。
        size_t num_levels() const
        {
            return _tree.num_levels();
        }

        // 第 k 个提升层上的胞腔数，下标要求沿用 cad_tree 的断言契约。
        size_t level_size(size_t k) const
        {
            return _tree.level_size(k);
        }

        // 零变量空间也有一个叶胞腔，即唯一空元组。
        size_t leaf_count() const
        {
            return num_levels() ? level_size(num_levels() - 1) : 1;
        }

        // 借用结构节点；边界 cad_root 保存投影来源身份，不是合并根表下标。
        const cad_node &node(size_t level, size_t index) const
        {
            return _tree.node(level, index);
        }

        // 借用该胞腔的完整实点证书；点可复制后精化，副本仍借用结果持有的比较器。
        const cad_sample &sample(size_t level, size_t index) const
        {
            return _samples.at(level).at(index);
        }

        // 统一查询末层叶样本；零变量情形返回静态空点，非法叶下标抛 out_of_range。
        const cad_sample &leaf_sample(size_t leaf) const
        {
            if (leaf >= leaf_count())
            {
                throw std::out_of_range("CAD leaf sample index");
            }
            if (num_levels())
            {
                return sample(num_levels() - 1, leaf);
            }
            static const cad_sample empty;
            return empty;
        }

        // 借用指定父胞腔的根栈；第零层仅有一个虚拟父节点，约定 parent=SIZE_MAX。
        const cad_stack &stack(size_t level, size_t parent = SIZE_MAX) const
        {
            return _stacks.at(level).at(level ? parent : 0);
        }

        // 在指定父栈解析来源身份；共享根的任何有效来源身份都指向同一个实点。
        // reference.root_idx() 只统计该来源的不同根，无穷边界没有对应样本。
        const cad_sample &root(size_t level, size_t parent, const cad_root &reference) const
        {
            if (!reference.isfinite())
            {
                throw std::invalid_argument("CAD: infinite boundary has no root sample");
            }
            const auto &current = stack(level, parent);
            size_t ordinal = 0;
            for (size_t i = 0; i < current.roots.size(); ++i)
            {
                const auto &sources = current.sources[i];
                if (std::find(sources.begin(), sources.end(), reference.poly_idx()) != sources.end())
                {
                    // 仅遇到包含该来源的根才推进编号，其他来源的根不参与计数。
                    if (ordinal == reference.root_idx())
                    {
                        return current.roots[i];
                    }
                    ++ordinal;
                }
            }
            throw std::out_of_range("CAD: root reference is not in this parent stack");
        }

        // 返回胞腔维数；Section 沿用父维数，Sector 在父维数上加一。
        size_t dimension(size_t level, size_t index) const
        {
            return _dimensions.at(level).at(index);
        }

        // 借用原输入多项式在指定叶上的符号行，各项为 -1、0 或 1。
        const std::vector<int> &signs(size_t leaf) const
        {
            return _leaf_signs.at(leaf);
        }

        // 借用该提升层的投影多项式，顺序与根栈的 evaluations 来源编号一致。
        const auto &level_polys(size_t level) const
        {
            return _tree.level_polys(level);
        }

        // 借用该层新增变量，按投影顺序的反方向提升。
        const variable &level_var(size_t level) const
        {
            return _tree.level_var(level);
        }

        // 只读借用底层树，避免绕过本容器修改节点后破坏并列表的对应关系。
        const cad_tree<custom_var_order> &structure() const
        {
            return _tree;
        }

        // 调用者已按叶顺序算好完整符号表；这里只转移存储，不重新求值或验证符号。
        void set_signs(std::vector<std::vector<int>> signs)
        {
            assert(signs.size() == leaf_count());
            _leaf_signs = std::move(signs);
        }

        // 借用投影比较器；借用者须保留拥有它的结果对象或对应共享所有权。
        const order_type *order() const
        {
            return _order.get();
        }

        // 同步追加 Sector 节点、样本和维数；调用者保证样本末坐标为 q，属于左右边界之间。
        // 同一父节点的全部子节点必须连续追加，结构约束由 cad_tree 的断言检查。
        void add_sector(size_t level, size_t parent, cad_root left, cad_root right, const QQ &q,
                        cad_sample sample, size_t dimension)
        {
            assert(level < num_levels() && sample.size() == level+1 && dimension <= level+1);
            _tree.add_sector(level, parent, left, right, q);
            _samples[level].push_back(std::move(sample));
            _dimensions[level].push_back(dimension);
        }

        // 同步追加 Section 节点、精确根样本和维数；root 是该根的规范来源身份。
        void add_section(size_t level, size_t parent, cad_root root, cad_sample sample,
                         size_t dimension)
        {
            assert(level < num_levels() && sample.size() == level+1 && dimension <= level);
            _tree.add_section(level, parent, root);
            _samples[level].push_back(std::move(sample));
            _dimensions[level].push_back(dimension);
        }

        // 按父节点存储顺序追加，第零层只有一个虚拟父节点。
        void add_stack(size_t level, cad_stack stack)
        {
            assert(stack.roots.size() == stack.sources.size());
            assert(stack.evaluations.size() == level_polys(level).size());
            _stacks.at(level).push_back(std::move(stack));
        }
    };
}
#endif
