#include <tools/cad_input.hh>
#include "clpoly_test.hh"
#include <random>

using namespace clpoly;
using namespace clpoly::cad_cli;

static cad_formula read(const std::string &text)
{
    std::istringstream input(text);
    return read_formula(input);
}

static polynomial_QQ polynomial(const std::string &text)
{
    return read("x y\n\n" + text + "\n\n[0,=]\n").polynomials.front();
}

static bool rejected(const std::string &text)
{
    try
    {
        read(text);
    }
    catch (const std::invalid_argument &)
    {
        return true;
    }
    return false;
}

int main()
{
    variable x("x"), y("y");
    using P = polynomial_QQ;
    CLPOLY_TEST("exact polynomial grammar and precedence");
    CLPOLY_ASSERT(polynomial("-x^2") == -P(x*x));
    CLPOLY_ASSERT(polynomial("(-x)^2") == P(x*x));
    CLPOLY_ASSERT(polynomial("--x + -+-y") == P(x+y));
    CLPOLY_ASSERT(polynomial("(x+y)**3") == pow(P(x+y), 3));
    CLPOLY_ASSERT(polynomial("(x+y)^0 + 0^0") == P(2));
    CLPOLY_ASSERT(polynomial("x/3 + y/(-2)") == P(x)*QQ(1,3) - P(y)*QQ(1,2));
    CLPOLY_ASSERT(polynomial("x/(y-y+3)") == P(x)*QQ(1,3));
    CLPOLY_ASSERT(polynomial("x/2/3") == P(x)*QQ(1,6));
    CLPOLY_ASSERT(polynomial("2*x + 3*x - 5*x") == P(0));
    CLPOLY_ASSERT(polynomial("000123") == P(123));
    const ZZ large("123456789012345678901234567890123456789012345678901");
    CLPOLY_ASSERT(polynomial("123456789012345678901234567890123456789012345678901/7") == P(QQ(large, ZZ(7))));
    CLPOLY_ASSERT(polynomial("0*x^9") == P(0));

    CLPOLY_TEST("generated binomial expressions against independent expansion");
    std::mt19937 generator(20261106);
    for (size_t i = 0; i < 100; ++i)
    {
        const int a = int(generator()%19)-9;
        const int b = int(generator()%19)-9;
        const std::string expression = "(" + std::to_string(a) + "*x+" +
                                       std::to_string(b) + "*y)^2/3";
        const P expected = P(x*x)*QQ(a*a,3) + P(x*y)*QQ(2*a*b,3) + P(y*y)*QQ(b*b,3);
        CLPOLY_ASSERT(polynomial(expression) == expected);
    }

    CLPOLY_TEST("CNF indexing, disjunction and sign semantics");
    auto formula = read("y x\n\nx\ny\n\n[0,>] | [0,=]\n[1,<]\n");
    CLPOLY_ASSERT(formula.variables == std::vector<variable>({y,x}));
    CLPOLY_ASSERT(formula.polynomials == std::vector<P>({P(x),P(y)}));
    CLPOLY_ASSERT_EQ(formula.clauses.size(), size_t(2));
    CLPOLY_ASSERT_EQ(formula.clauses[0].size(), size_t(2));
    for (int sx = -1; sx <= 1; ++sx)
    {
        for (int sy = -1; sy <= 1; ++sy)
        {
            CLPOLY_ASSERT_EQ(formula_holds(formula.clauses, {sx,sy}), sx >= 0 && sy < 0);
        }
    }
    CLPOLY_ASSERT(formula_holds({}, {}));
    CLPOLY_ASSERT(!formula_holds({{}}, {}));

    CLPOLY_TEST("line endings, BOM, blank separators and UTF-8 names");
    auto crlf = read("\xef\xbb\xbfx\r\n\r\n\t\r\nx/3\r\n\r\n[0,=]\r\n\r\n");
    CLPOLY_ASSERT(crlf.polynomials.front() == P(x)*QQ(1,3));
    auto unicode = read("变量\n\n变量^2-2\n\n[0,=]\n");
    CLPOLY_ASSERT_EQ(unicode.variables.front().name(), std::string("变量"));
    CLPOLY_ASSERT(read("_x x1\n\n_x+x1\n\n[0,=]").variables.size() == 2);

    CLPOLY_TEST("invalid polynomial syntax, division and exponent bounds");
    const std::vector<std::string> invalid = {
        "z+1", "2x", "x^", "x^-1", "x^1.5", "x^y", "x^2^3", "x**2**3",
        "x/(y+1)", "x/0", "x/(y-y)", "x/", "sqrt(x)", "0.5*x", "x=1",
        "x%2", "x;", "()", "(x+1", "x+1)", "x+", "x * * 2",
        "x^9223372036854775808", "x^18446744073709551616",
        "x^9223372036854775807*x", "(x^4611686018427387904)^2"
    };
    for (const auto &expression : invalid)
    {
        CLPOLY_ASSERT(rejected("x y\n\n" + expression + "\n\n[0,=]\n"));
    }
    CLPOLY_ASSERT(polynomial(std::string(256,'(') + "x" + std::string(256,')')) == P(x));
    CLPOLY_ASSERT(rejected("x y\n\n" + std::string(257,'(') + "x" + std::string(257,')') + "\n\n[0,=]\n"));

    CLPOLY_TEST("invalid variable, block and atom syntax");
    const std::vector<std::string> invalid_formulas = {
        "", "x\n\nx", "x x\n\nx\n\n[0,=]", "2x\n\n0\n\n[0,=]",
        "x\ny\n\nx\n\n[0,=]", "x\n\nx\n\n[0,=]\n\njunk",
        "x\n\nx\n\n[1,=]", "x\n\nx\n\n[-1,=]", "x\n\nx\n\n[0,!=]",
        "x\n\nx\n\n[0,>=]", "x\n\nx\n\n[0,==]", "x\n\nx\n\n[0,=]|",
        "x\n\nx\n\n[0,=]junk", "x\n\nx\n\n[]", "x\n\nx\n\n[0 =]",
        "x\n\nx\n\n[18446744073709551616,=]", "x\n\nx\n\n[0,=] [0,<]"
    };
    for (const auto &text : invalid_formulas)
    {
        CLPOLY_ASSERT(rejected(text));
    }
    try
    {
        read("x\n\nx+z\n\n[0,=]\n");
        CLPOLY_ASSERT(false);
    }
    catch (const std::invalid_argument &e)
    {
        CLPOLY_ASSERT(std::string(e.what()).find("line 3, column") != std::string::npos);
    }
    return clpoly_test::test_summary();
}
