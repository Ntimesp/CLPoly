/** Exact bounded rational intervals. No polynomial or root dependencies.
 * Specification and proofs: docs/cad/design/interval-arithmetic/design.md (I01--I13).
 */
#ifndef CLPOLY_RATIONAL_INTERVAL_HH
#define CLPOLY_RATIONAL_INTERVAL_HH
#include <clpoly/number/QQ.hh>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <utility>

namespace clpoly
{
enum class interval_sign
{
    negative = -1,
    zero = 0,
    positive = 1,
    unknown = 2
};

// Nonempty, bounded, closed interval [left,right]. QQ arithmetic is exact.
// Unknown sign means the enclosure meets zero; it does not certify a zero value.
class rational_interval
{
    QQ left_, right_;

  public:
    rational_interval() : left_(0), right_(0) {}
    rational_interval(const QQ& q) : left_(q), right_(q) {}
    // Own temporaries by move; lvalue endpoints are copied once into the parameters.
    rational_interval(QQ a, QQ b) : left_(std::move(a)), right_(std::move(b))
    {
        if (right_ < left_)
        {
            throw std::invalid_argument("rational_interval: reversed endpoints");
        }
    }
    const QQ& left() const
    {
        return left_;
    }
    const QQ& right() const
    {
        return right_;
    }
    bool is_point() const
    {
        return left_ == right_;
    }
    QQ width() const
    {
        return right_ - left_;
    }
    QQ midpoint() const
    {
        return (left_ + right_) / QQ(2);
    }
    bool contains(const QQ& q) const
    {
        return !(q < left_) && !(right_ < q);
    }
    bool contains(const rational_interval& y) const
    {
        return !(y.left_ < left_) && !(right_ < y.right_);
    }
    bool interior_contains(const QQ& q) const
    {
        return left_ < q && q < right_;
    }
    bool interior_contains(const rational_interval& y) const
    {
        return left_ < y.left_ && y.right_ < right_;
    }
    bool contains_zero() const
    {
        return clpoly::sgn(left_) <= 0 && clpoly::sgn(right_) >= 0;
    }
    bool overlaps(const rational_interval& y) const
    {
        return !(right_ < y.left_) && !(y.right_ < left_);
    }
    interval_sign sign() const
    {
        const int left_sign = clpoly::sgn(left_);
        const int right_sign = clpoly::sgn(right_);
        if (left_sign > 0)
        {
            return interval_sign::positive;
        }
        if (right_sign < 0)
        {
            return interval_sign::negative;
        }
        if (left_sign == 0 && right_sign == 0)
        {
            return interval_sign::zero;
        }
        return interval_sign::unknown;
    }
    int sgn() const
    {
        return static_cast<int>(sign());
    }
    std::pair<rational_interval, rational_interval> bisect() const
    {
        const QQ m = midpoint();
        return {{left_, m}, {m, right_}};
    }
    friend rational_interval operator+(const rational_interval& x, const rational_interval& y)
    {
        return {x.left_ + y.left_, x.right_ + y.right_};
    }
    friend rational_interval operator-(const rational_interval& x, const rational_interval& y)
    {
        return {x.left_ - y.right_, x.right_ - y.left_};
    }
    friend rational_interval operator-(const rational_interval& x)
    {
        return {-x.right_, -x.left_};
    }
    friend rational_interval operator*(const rational_interval& x, const rational_interval& y)
    {
        const QQ& a = x.left_;
        const QQ& b = x.right_;
        const QQ& c = y.left_;
        const QQ& d = y.right_;

        // I04: monotonicity selects two extreme corners unless both intervals
        // straddle zero. Test nonnegative first, so [0,0] has one classification.
        if (clpoly::sgn(a) >= 0)
        {
            if (clpoly::sgn(c) >= 0)
            {
                return {a * c, b * d};
            }
            if (clpoly::sgn(d) <= 0)
            {
                return {b * c, a * d};
            }
            return {b * c, b * d};
        }
        if (clpoly::sgn(b) <= 0)
        {
            if (clpoly::sgn(c) >= 0)
            {
                return {a * d, b * c};
            }
            if (clpoly::sgn(d) <= 0)
            {
                return {b * d, a * c};
            }
            return {a * d, a * c};
        }
        if (clpoly::sgn(c) >= 0)
        {
            return {a * d, b * d};
        }
        if (clpoly::sgn(d) <= 0)
        {
            return {b * c, a * c};
        }

        // Both straddle zero: choose between two negative and two positive products.
        QQ ad = a * d;
        QQ bc = b * c;
        QQ ac = a * c;
        QQ bd = b * d;
        QQ lower = ad < bc ? std::move(ad) : std::move(bc);
        QQ upper = ac < bd ? std::move(bd) : std::move(ac);
        return {std::move(lower), std::move(upper)};
    }

    friend rational_interval reciprocal(const rational_interval& x)
    {
        if (x.contains_zero())
        {
            throw std::domain_error("rational_interval: reciprocal includes zero");
        }
        return {QQ(1) / x.right_, QQ(1) / x.left_};
    }
    friend rational_interval operator/(const rational_interval& x, const rational_interval& y)
    {
        return x * reciprocal(y);
    }
    rational_interval& operator+=(const rational_interval& y)
    {
        *this = *this + y;
        return *this;
    }
    rational_interval& operator-=(const rational_interval& y)
    {
        *this = *this - y;
        return *this;
    }
    rational_interval& operator*=(const rational_interval& y)
    {
        *this = *this * y;
        return *this;
    }
    rational_interval& operator/=(const rational_interval& y)
    {
        *this = *this / y;
        return *this;
    }
    friend bool operator==(const rational_interval& x, const rational_interval& y)
    {
        return x.left_ == y.left_ && x.right_ == y.right_;
    }
    friend bool operator!=(const rational_interval& x, const rational_interval& y)
    {
        return !(x == y);
    }
    friend rational_interval hull(const rational_interval& x, const rational_interval& y)
    {
        return {std::min(x.left_, y.left_), std::max(x.right_, y.right_)};
    }
    friend std::optional<rational_interval> intersection(const rational_interval& x,
                                                         const rational_interval& y)
    {
        QQ a = std::max(x.left_, y.left_);
        QQ b = std::min(x.right_, y.right_);
        if (b < a)
        {
            return std::nullopt;
        }
        return rational_interval(std::move(a), std::move(b));
    }
    // Exact range of the power function: even powers may attain zero internally.
    friend rational_interval pow(const rational_interval& x, int64_t n)
    {
        if (n < 0)
        {
            throw std::domain_error("rational_interval: negative power");
        }
        if (n == 0)
        {
            return rational_interval(QQ(1));
        }
        if (n == 1)
        {
            return x;
        }
        if (x.is_point())
        {
            // A point needs one exponentiation; the value is shared mathematically,
            // while the interval still owns two independent endpoint objects.
            return rational_interval(pow(x.left_, static_cast<uint64_t>(n)));
        }
        QQ a = pow(x.left_, static_cast<uint64_t>(n));
        QQ b = pow(x.right_, static_cast<uint64_t>(n));
        if (n & 1)
        {
            return {std::move(a), std::move(b)};
        }
        if (b < a)
        {
            a.swap(b);
        }
        if (x.contains_zero())
        {
            a = 0;
        }
        return {std::move(a), std::move(b)};
    }

    friend std::ostream& operator<<(std::ostream& out, const rational_interval& x)
    {
        return out << '[' << x.left_ << ',' << x.right_ << ']';
    }
};
} // namespace clpoly
#endif
