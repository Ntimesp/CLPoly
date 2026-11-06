// 读取实验脚本生成的精确系数文件，不在此处解释题意或进行预处理。
#include <clpoly/full_cad.hh>
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace clpoly;
using P = polynomial_QQ;

struct atom
{
    size_t polynomial;
    char relation;
};

// 数字输入是工具的公开边界；负数、溢出和截断必须在调用库前拒绝。
static std::string read_token(std::istream &input)
{
    std::string token;
    if (!(input >> token))
    {
        throw std::invalid_argument("truncated experiment input");
    }
    return token;
}

static size_t read_count(std::istream &input)
{
    const auto token = read_token(input);
    if (token.find_first_not_of("0123456789") != std::string::npos)
    {
        throw std::invalid_argument("invalid nonnegative integer");
    }
    const auto value = std::stoull(token);
    if (value > std::numeric_limits<size_t>::max())
    {
        throw std::invalid_argument("integer out of range");
    }
    return static_cast<size_t>(value);
}

static ZZ read_integer(std::istream &input)
{
    const auto token = read_token(input);
    const size_t start = token[0] == '-' || token[0] == '+' ? 1 : 0;
    if (start == token.size() || token.find_first_not_of("0123456789", start) != std::string::npos)
    {
        throw std::invalid_argument("invalid integer coefficient");
    }
    return ZZ(token[0] == '+' ? token.substr(1) : token);
}

int main(int argc, char **argv)
{
    if (argc < 2 || argc > 3 || (argc == 3 && std::string(argv[2]) != "reverse"))
    {
        return 2;
    }
    try
    {
        std::ifstream input(argv[1]);
        if (!input)
        {
            throw std::invalid_argument("cannot open experiment input");
        }
        const size_t variable_count = read_count(input);
        const size_t polynomial_count = read_count(input);
        const size_t clause_count = read_count(input);
        std::vector<variable> variables;
        for (size_t i = 0; i < variable_count; ++i)
        {
            const auto name = read_token(input);
            variables.emplace_back(name);
        }
        std::vector<P> polynomials;
        for (size_t i = 0; i < polynomial_count; ++i)
        {
            const size_t term_count = read_count(input);
            P p;
            for (size_t j = 0; j < term_count; ++j)
            {
                const ZZ numerator = read_integer(input), denominator = read_integer(input);
                if (denominator == ZZ(0))
                {
                    throw std::invalid_argument("zero denominator");
                }
                P term{QQ(numerator, denominator)};
                for (const auto &v : variables)
                {
                    const size_t exponent = read_count(input);
                    if (exponent > std::numeric_limits<unsigned int>::max())
                    {
                        throw std::invalid_argument("exponent out of range");
                    }
                    if (exponent)
                    {
                        term = term * pow(v, exponent);
                    }
                }
                p = p + term;
            }
            polynomials.push_back(std::move(p));
        }
        std::vector<std::vector<atom>> clauses(clause_count);
        for (auto &clause : clauses)
        {
            const size_t count = read_count(input);
            for (size_t i = 0; i < count; ++i)
            {
                atom a;
                a.polynomial = read_count(input);
                const auto relation = read_token(input);
                if (a.polynomial >= polynomials.size() ||
                    (relation != "=" && relation != ">" && relation != "<"))
                {
                    throw std::invalid_argument("invalid atom reference or relation");
                }
                a.relation = relation[0];
                clause.push_back(a);
            }
        }
        input >> std::ws;
        if (!input.eof())
        {
            throw std::invalid_argument("unexpected trailing input");
        }
        if (argc > 2)
        {
            std::reverse(variables.begin(), variables.end());
        }

        const auto start = std::chrono::steady_clock::now();
        auto cad = full_cad(polynomials, variables);
        const auto built = std::chrono::steady_clock::now();
        size_t feasible = 0, first = SIZE_MAX;
        for (size_t i = 0; i < cad.leaf_count(); ++i)
        {
            bool satisfied = true;
            for (const auto &clause : clauses)
            {
                bool any = false;
                for (const auto &a : clause)
                {
                    const int s = cad.signs(i)[a.polynomial];
                    any = any || (a.relation == '=' ? s == 0 : a.relation == '>' ? s > 0 : s < 0);
                }
                satisfied = satisfied && any;
            }
            if (satisfied)
            {
                ++feasible;
                if (first == SIZE_MAX)
                {
                    first = i;
                }
            }
        }
        if (first != SIZE_MAX)
        {
            // 独立调用普通点查询，核对已选 SAT 样本的每个输入符号。
            auto sample = cad.leaf_sample(first);
            for (size_t i = 0; i < polynomials.size(); ++i)
            {
                if (point_sign(polynomials[i], sample) != cad.signs(first)[i])
                {
                    throw std::runtime_error("SAT sample sign mismatch");
                }
            }
        }
        const auto end = std::chrono::steady_clock::now();
        std::cout << "status=" << (feasible ? "sat" : "unsat")
                  << " leaves=" << cad.leaf_count() << " feasible=" << feasible
                  << " cad_seconds=" << std::chrono::duration<double>(built - start).count()
                  << " total_seconds=" << std::chrono::duration<double>(end - start).count()
                  << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
