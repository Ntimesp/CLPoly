/**
 * @file cad_input.hh
 * @brief CAD 题集 TXT 格式的精确读取与 CNF 符号判断
 */
#ifndef CLPOLY_CAD_INPUT_HH
#define CLPOLY_CAD_INPUT_HH

#include <clpoly/polynomial.hh>
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <istream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace clpoly{
namespace cad_cli{

    struct formula_atom
    {
        size_t polynomial;
        char relation;
    };

    struct cad_formula
    {
        std::vector<variable> variables;
        std::vector<polynomial_QQ> polynomials;
        std::vector<std::vector<formula_atom>> clauses;
    };

    struct input_line
    {
        size_t number;
        std::string text;
    };

    // 扫描状态只保存数据；语法与算术由下面的自由函数处理。
    struct input_reader
    {
        const input_line &line;
        const std::map<std::string,variable> &variables;
        size_t position = 0;
        size_t depth = 0;
    };

    [[noreturn]] inline void input_error(const input_reader &r, const std::string &message)
    {
        throw std::invalid_argument("line " + std::to_string(r.line.number) + ", column " +
                                    std::to_string(r.position + 1) + ": " + message);
    }

    inline void skip_space(input_reader &r)
    {
        while (r.position < r.line.text.size() &&
               std::isspace(static_cast<unsigned char>(r.line.text[r.position])))
        {
            ++r.position;
        }
    }

    inline bool accept(input_reader &r, char c)
    {
        skip_space(r);
        if (r.position < r.line.text.size() && r.line.text[r.position] == c)
        {
            ++r.position;
            return true;
        }
        return false;
    }

    inline void expect(input_reader &r, char c)
    {
        if (!accept(r, c))
        {
            input_error(r, std::string("expected '") + c + "'");
        }
    }

    inline bool digit(char c)
    {
        return c >= '0' && c <= '9';
    }

    inline bool identifier_start(unsigned char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c >= 128;
    }

    inline bool identifier_part(unsigned char c)
    {
        return identifier_start(c) || digit(static_cast<char>(c));
    }

    inline uint64_t read_unsigned(input_reader &r, uint64_t maximum)
    {
        skip_space(r);
        const size_t start = r.position;
        uint64_t result = 0;
        while (r.position < r.line.text.size() && digit(r.line.text[r.position]))
        {
            const auto d = static_cast<unsigned int>(r.line.text[r.position] - '0');
            if (result > (maximum - d) / 10)
            {
                input_error(r, "integer out of range");
            }
            result = result * 10 + d;
            ++r.position;
        }
        if (start == r.position)
        {
            input_error(r, "expected nonnegative integer");
        }
        return result;
    }

    inline int64_t total_degree(const polynomial_QQ &p)
    {
        int64_t result = 0;
        for (const auto &term : p)
        {
            result = std::max(result, term.first.deg());
        }
        return result;
    }

    inline polynomial_QQ read_sum(input_reader &r);

    inline polynomial_QQ read_primary(input_reader &r)
    {
        skip_space(r);
        if (accept(r, '('))
        {
            if (r.depth == 256)
            {
                input_error(r, "parenthesis nesting exceeds 256");
            }
            ++r.depth;
            auto result = read_sum(r);
            expect(r, ')');
            --r.depth;
            return result;
        }
        const size_t start = r.position;
        if (start < r.line.text.size() && digit(r.line.text[start]))
        {
            while (r.position < r.line.text.size() && digit(r.line.text[r.position]))
            {
                ++r.position;
            }
            return polynomial_QQ(QQ(ZZ(r.line.text.substr(start, r.position - start))));
        }
        if (start < r.line.text.size() && identifier_start(r.line.text[start]))
        {
            while (r.position < r.line.text.size() && identifier_part(r.line.text[r.position]))
            {
                ++r.position;
            }
            const auto name = r.line.text.substr(start, r.position - start);
            const auto found = r.variables.find(name);
            if (found == r.variables.end())
            {
                input_error(r, "undeclared variable: " + name);
            }
            return polynomial_QQ(found->second);
        }
        input_error(r, "expected integer, variable or parenthesis");
    }

    inline polynomial_QQ read_power(input_reader &r)
    {
        auto result = read_primary(r);
        skip_space(r);
        bool power = accept(r, '^');
        if (!power && r.line.text.compare(r.position, 2, "**") == 0)
        {
            r.position += 2;
            power = true;
        }
        if (power)
        {
            const uint64_t maximum = std::numeric_limits<int64_t>::max();
            const uint64_t exponent = read_unsigned(r, maximum);
            const auto degree = static_cast<uint64_t>(total_degree(result));
            if (degree && exponent > maximum / degree)
            {
                input_error(r, "polynomial degree out of range");
            }
            result = pow(result, static_cast<int64_t>(exponent));
        }
        return result;
    }

    inline polynomial_QQ read_unary(input_reader &r)
    {
        bool negative = false;
        while (true)
        {
            if (accept(r, '+'))
            {
                continue;
            }
            if (accept(r, '-'))
            {
                negative = !negative;
                continue;
            }
            break;
        }
        auto result = read_power(r);
        return negative ? -result : result;
    }

    inline polynomial_QQ read_product(input_reader &r)
    {
        auto result = read_unary(r);
        while (true)
        {
            if (accept(r, '*'))
            {
                auto q = read_unary(r);
                if (total_degree(result) > std::numeric_limits<int64_t>::max() - total_degree(q))
                {
                    input_error(r, "polynomial degree out of range");
                }
                result = result * q;
            }
            else if (accept(r, '/'))
            {
                auto q = read_unary(r);
                if (q.empty() || !is_number(q))
                {
                    input_error(r, "division requires a nonzero rational constant");
                }
                result = result * (QQ(1) / q.front().second);
            }
            else
            {
                return result;
            }
        }
    }

    inline polynomial_QQ read_sum(input_reader &r)
    {
        auto result = read_product(r);
        while (true)
        {
            if (accept(r, '+'))
            {
                result = result + read_product(r);
            }
            else if (accept(r, '-'))
            {
                result = result - read_product(r);
            }
            else
            {
                return result;
            }
        }
    }

    inline void expect_end(input_reader &r)
    {
        skip_space(r);
        if (r.position != r.line.text.size())
        {
            input_error(r, "unexpected trailing input");
        }
    }

    inline cad_formula read_formula(std::istream &input)
    {
        std::vector<std::vector<input_line>> blocks;
        bool separator = true;
        size_t line_number = 0;
        std::string text;
        while (std::getline(input, text))
        {
            ++line_number;
            if (line_number == 1 && text.compare(0, 3, "\xef\xbb\xbf") == 0)
            {
                text.erase(0, 3);
            }
            if (text.find_first_not_of(" \t\r\n\v\f") == std::string::npos)
            {
                separator = true;
                continue;
            }
            if (separator)
            {
                blocks.emplace_back();
                separator = false;
            }
            if (blocks.size() > 3)
            {
                throw std::invalid_argument("line " + std::to_string(line_number) + ": extra input block");
            }
            blocks.back().push_back({line_number, text});
        }
        if (input.bad() || !input.eof())
        {
            throw std::invalid_argument("failed to read input");
        }
        if (blocks.size() != 3 || blocks[0].size() != 1)
        {
            throw std::invalid_argument("expected variable line, polynomial block and clause block");
        }

        cad_formula result;
        std::map<std::string,variable> variables;
        std::istringstream names(blocks[0][0].text);
        std::string name;
        while (names >> name)
        {
            if (!identifier_start(name.front()) ||
                !std::all_of(name.begin(), name.end(), [](unsigned char c) { return identifier_part(c); }))
            {
                throw std::invalid_argument("invalid variable name: " + name);
            }
            if (variables.count(name))
            {
                throw std::invalid_argument("duplicate variable: " + name);
            }
            variable v(name);
            variables.emplace(name, v);
            result.variables.push_back(v);
        }
        for (const auto &line : blocks[1])
        {
            input_reader r{line, variables};
            result.polynomials.push_back(read_sum(r));
            expect_end(r);
        }
        for (const auto &line : blocks[2])
        {
            input_reader r{line, variables};
            std::vector<formula_atom> clause;
            do
            {
                expect(r, '[');
                const size_t index = static_cast<size_t>(read_unsigned(r, std::numeric_limits<size_t>::max()));
                if (index >= result.polynomials.size())
                {
                    input_error(r, "polynomial index out of range");
                }
                expect(r, ',');
                skip_space(r);
                if (r.position == line.text.size() ||
                    (line.text[r.position] != '=' && line.text[r.position] != '>' && line.text[r.position] != '<'))
                {
                    input_error(r, "expected relation =, > or <");
                }
                const char relation = line.text[r.position++];
                expect(r, ']');
                clause.push_back({index, relation});
            } while (accept(r, '|'));
            expect_end(r);
            result.clauses.push_back(std::move(clause));
        }
        return result;
    }

    // clauses 已检查，signs 与原输入多项式一一对应且只含 -1/0/1。
    inline bool formula_holds(const std::vector<std::vector<formula_atom>> &clauses,
                              const std::vector<int> &signs)
    {
        for (const auto &clause : clauses)
        {
            bool satisfied = false;
            for (const auto &atom : clause)
            {
                assert(atom.polynomial < signs.size());
                assert(atom.relation == '=' || atom.relation == '>' || atom.relation == '<');
                const int sign = signs[atom.polynomial];
                assert(sign >= -1 && sign <= 1);
                satisfied = satisfied || (atom.relation == '=' ? sign == 0 :
                                         atom.relation == '>' ? sign > 0 : sign < 0);
            }
            if (!satisfied)
            {
                return false;
            }
        }
        return true;
    }
}
}
#endif
