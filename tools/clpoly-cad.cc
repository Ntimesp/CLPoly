#include <clpoly/clpoly.hh>
#include "cad_input.hh"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace clpoly;

static void usage(std::ostream &out)
{
    out << "Usage: clpoly-cad [--sample | --check] [--reverse] FILE\n"
        << "       clpoly-cad --help\n"
        << "FILE is a three-block polynomial CNF file; use - for standard input.\n"
        << "Prints SAT or UNSAT. --sample also prints an exact real-point certificate.\n"
        << "--check validates input only and prints VALID. --reverse reverses projection order.\n";
}

static void write_sample(std::ostream &out, const cad_sample &sample)
{
    out << "sample_variables:";
    for (const auto &v : sample.chain().variables())
    {
        out << ' ' << v;
    }
    out << "\nequations:\n";
    for (const auto &p : sample.chain().equations())
    {
        out << p << " = 0\n";
    }
    out << "isolating_intervals:\n";
    for (size_t i = 0; i < sample.size(); ++i)
    {
        out << sample.chain().variables()[i] << " in (" << sample.box()[i].left()
            << ", " << sample.box()[i].right() << ")\n";
    }
    out << "shifts:";
    for (const auto &q : sample.shifts())
    {
        out << ' ' << q;
    }
    out << '\n';
}

int main(int argc, char **argv)
{
    bool sample = false, check = false, reverse = false, options = true;
    std::string path;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg(argv[i]);
        if (options && (arg == "--help" || arg == "-h"))
        {
            usage(std::cout);
            return 0;
        }
        if (options && arg == "--")
        {
            options = false;
        }
        else if (options && arg == "--sample")
        {
            sample = true;
        }
        else if (options && arg == "--check")
        {
            check = true;
        }
        else if (options && arg == "--reverse")
        {
            reverse = true;
        }
        else if ((!arg.empty() && arg[0] == '-' && arg != "-" && options) || !path.empty())
        {
            std::cerr << "clpoly-cad: unexpected argument: " << arg << '\n';
            usage(std::cerr);
            return 2;
        }
        else
        {
            path = arg;
        }
    }
    if (path.empty() || (sample && check))
    {
        usage(std::cerr);
        return 2;
    }

    cad_cli::cad_formula formula;
    try
    {
        if (path == "-")
        {
            formula = cad_cli::read_formula(std::cin);
        }
        else
        {
            std::ifstream input(path);
            if (!input)
            {
                throw std::invalid_argument("cannot open input file: " + path);
            }
            formula = cad_cli::read_formula(input);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "clpoly-cad: " << e.what() << '\n';
        return 2;
    }
    if (check)
    {
        std::cout << "VALID\n";
        return 0;
    }
    try
    {
        if (reverse)
        {
            std::reverse(formula.variables.begin(), formula.variables.end());
        }
        auto cad = full_cad(formula.polynomials, formula.variables);
        size_t selected = cad.leaf_count();
        for (size_t i = 0; i < cad.leaf_count(); ++i)
        {
            if (cad_cli::formula_holds(formula.clauses, cad.signs(i)))
            {
                selected = i;
                break;
            }
        }
        // 全部输出先准备完成，求解或证书输出异常时不留下成功答案。
        std::ostringstream answer;
        answer << (selected < cad.leaf_count() ? "SAT\n" : "UNSAT\n");
        if (sample && selected < cad.leaf_count())
        {
            write_sample(answer, cad.leaf_sample(selected));
        }
        std::cout << answer.str();
    }
    catch (const std::exception &e)
    {
        std::cerr << "clpoly-cad: computation failed: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
