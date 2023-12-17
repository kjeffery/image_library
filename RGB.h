//
// Created by Keith on 2/1/2021.
//

#pragma once

#include <algorithm>
#include <cstdint>
#include <ostream>
#include <print>

template <typename T>
struct RGB
{
    using value_type = T;
    using size_type  = std::uint32_t;

    constexpr RGB() noexcept
    : r(0)
    , g(0)
    , b(0)
    {
    }

    explicit constexpr RGB(T i) noexcept
    : r(i)
    , g(i)
    , b(i)
    {
    }

    constexpr RGB(T ir, T ig, T ib) noexcept
    : r(ir)
    , g(ig)
    , b(ib)
    {
    }

    T& operator[](size_type idx) noexcept
    {
        switch (idx) {
        case 0:
            return r;
        case 1:
            return g;
        case 2:
            return b;
        default:
            assert(!"Should not get here");
        }
        return r;
    }

    T operator[](size_type idx) const noexcept
    {
        switch (idx) {
        case 0:
            return r;
        case 1:
            return g;
        case 2:
            return b;
        default:
            assert(!"Should not get here");
        }
        return r;
    }

    T r;
    T g;
    T b;
};

template <typename T>
RGB<T>& operator+=(RGB<T>& a, const RGB<T>& b) noexcept
{
    a.r += b.r;
    a.g += b.g;
    a.b += b.b;
    return a;
}

template <typename T>
RGB<T>& operator*=(RGB<T>& a, T b) noexcept
{
    a.r *= b;
    a.g *= b;
    a.b *= b;
    return a;
}

template <typename T>
RGB<T>& operator/=(RGB<T>& a, T b) noexcept
{
    a.r /= b;
    a.g /= b;
    a.b /= b;
    return a;
}

// Pass-by-value on purpose
template <typename T>
RGB<T> operator+(RGB<T> a, const RGB<T>& b) noexcept
{
    return a += b;
}

// Pass-by-value on purpose
template <typename T>
RGB<T> operator*(RGB<T> a, T b) noexcept
{
    return a *= b;
}

// Pass-by-value on purpose
template <typename T>
RGB<T> operator*(T b, RGB<T> a) noexcept
{
    return a *= b;
}

// Pass-by-value on purpose
template <typename T>
RGB<T> operator/(RGB<T> a, T b) noexcept
{
    return a /= b;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& outs, const RGB<T>& c)
{
    std::print(outs, "{} {} {}", c.r, c.g, c.g);
    return outs;
}

using RGBf  = RGB<float>;
using RGB8  = RGB<std::uint8_t>;
using RGB16 = RGB<std::uint16_t>;
using RGB32 = RGB<std::uint32_t>;

template <typename T>
inline RGBf to_float(const RGB<T>& c) noexcept
{
    constexpr auto max = std::numeric_limits<typename RGB<T>::value_type>::max();
    static_assert(max > 0); // That'd be weird, right?
    constexpr float maxf = static_cast<float>(max);

    return { static_cast<float>(c.r) / maxf, static_cast<float>(c.g) / maxf, static_cast<float>(c.b) / maxf };
}

inline const RGBf& to_float(const RGBf& c) noexcept
{
    return c;
}

inline RGBf clamp(const RGBf& c) noexcept
{
    return { std::clamp(c.r, 0.0f, 1.0f), std::clamp(c.g, 0.0f, 1.0f), std::clamp(c.b, 0.0f, 1.0f) };
}
