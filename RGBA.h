//
// Created by Keith on 2/1/2021.
//

#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <ostream>
#include <print>

template <typename T>
constexpr T k_default_alpha = std::numeric_limits<T>::max();

template <>
constexpr float k_default_alpha<float> = 1.0f;

template <typename T>
struct RGBA
{
    using value_type = T;
    using size_type  = std::uint32_t;

    constexpr RGBA() noexcept
    : r(0)
    , g(0)
    , b(0)
    , a(k_default_alpha<T>)
    {
    }

    constexpr RGBA(T ir, T ig, T ib, T ia = k_default_alpha<T>) noexcept
    : r(ir)
    , g(ig)
    , b(ib)
    , a(ia)
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
        case 3:
            return a;
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
        case 3:
            return a;
        default:
            assert(!"Should not get here");
        }
        return r;
    }

    T r;
    T g;
    T b;
    T a;
};

template <typename T>
RGBA<T>& operator+=(RGBA<T>& a, const RGBA<T>& b) noexcept
{
    a.r += b.r;
    a.g += b.g;
    a.b += b.b;
    a.a += b.a;
    return a;
}

template <typename T>
RGBA<T>& operator*=(RGBA<T>& a, T b) noexcept
{
    a.r *= b;
    a.g *= b;
    a.b *= b;
    a.a *= a;
    return a;
}

template <typename T>
RGBA<T>& operator/=(RGBA<T>& a, T b) noexcept
{
    a.r /= b;
    a.g /= b;
    a.b /= b;
    a.a /= b;
    return a;
}

// Pass-by-value on purpose
template <typename T>
RGBA<T> operator+(RGBA<T> a, const RGBA<T>& b) noexcept
{
    return a += b;
}

// Pass-by-value on purpose
template <typename T>
RGBA<T> operator*(RGBA<T> a, T b) noexcept
{
    return a *= b;
}

// Pass-by-value on purpose
template <typename T>
RGBA<T> operator*(T b, RGBA<T> a) noexcept
{
    return a *= b;
}

// Pass-by-value on purpose
template <typename T>
RGBA<T> operator/(RGBA<T> a, T b) noexcept
{
    return a /= b;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& outs, const RGBA<T>& c)
{
    std::print(outs, "{} {} {} {}", c.r, c.g, c.g, c.a);
    return outs;
}

using RGBAf  = RGBA<float>;
using RGBA8  = RGBA<std::uint8_t>;
using RGBA16 = RGBA<std::uint16_t>;
using RGBA32 = RGBA<std::uint32_t>;

template <typename T>
inline RGBAf to_float(const RGBA<T>& c) noexcept
{
    constexpr auto max = std::numeric_limits<typename RGBA<T>::value_type>::max();
    static_assert(max > 0); // That'd be weird, right?
    constexpr float maxf = static_cast<float>(max);

    return { static_cast<float>(c.r) / maxf,
             static_cast<float>(c.g) / maxf,
             static_cast<float>(c.b) / maxf,
             static_cast<float>(c.a) / maxf };
}

inline const RGBAf& to_float(const RGBAf& c) noexcept
{
    return c;
}

inline RGBAf clamp(const RGBAf& c) noexcept
{
    return { std::clamp(c.r, 0.0f, 1.0f),
             std::clamp(c.g, 0.0f, 1.0f),
             std::clamp(c.b, 0.0f, 1.0f),
             std::clamp(c.a, 0.0f, 1.0f) };
}
