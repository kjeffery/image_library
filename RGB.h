//
// Created by Keith on 2/1/2021.
//

#pragma once

#include <ostream>

struct RGB
{
    constexpr RGB() noexcept
    : r(0)
    , g(0)
    , b(0)
    {
    }

    constexpr RGB(float ir, float ig, float ib) noexcept
    : r(ir)
    , g(ig)
    , b(ib)
    {
    }

    float& operator[](std::uint32_t idx) noexcept
    {
        switch (idx) {
            case 0: return r;
            case 1: return g;
            case 2: return b;
            default: assert(!"Should not get here");
        }
        return r;
    }

    float operator[](std::uint32_t idx) const noexcept
    {
        switch (idx) {
            case 0: return r;
            case 1: return g;
            case 2: return b;
            default: assert(!"Should not get here");
        }
        return r;
    }

    float r;
    float g;
    float b;
};

RGB& operator+=(RGB& a, const RGB& b) noexcept
{
    a.r += b.r;
    a.g += b.g;
    a.b += b.b;
    return a;
}

RGB& operator*=(RGB& a, float b) noexcept
{
    a.r *= b;
    a.g *= b;
    a.b *= b;
    return a;
}

RGB& operator/=(RGB& a, float b) noexcept
{
    a.r /= b;
    a.g /= b;
    a.b /= b;
    return a;
}

// Pass-by-value on purpose
RGB operator+(RGB a, const RGB& b) noexcept
{
    return a += b;
}

// Pass-by-value on purpose
RGB operator*(RGB a, float b) noexcept
{
    return a *= b;
}

// Pass-by-value on purpose
RGB operator*(float b, RGB a) noexcept
{
    return a *= b;
}

// Pass-by-value on purpose
RGB operator/(RGB a, float b) noexcept
{
    return a /= b;
}

inline std::ostream& operator<<(std::ostream& outs, const RGB& c)
{
    return outs << c.r << ' ' << c.g << ' ' << c.b;
}


