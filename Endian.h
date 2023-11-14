//
// Created by Keith on 1/30/2021.
//

#pragma once

#include <bit>
#include <cstdint>
#include <type_traits>

// This is not likely to happen, but it _is_ possible for machines to be an odd mix of big or little endian (at least
// according to some historic documents).
static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little);

template <typename T>
requires std::is_integral_v<T> constexpr T little_endian(T v)
{
    if constexpr (std::endian::native == std::endian::little) {
        return v;
    } else {
        return std::byteswap(v);
    }
}

template <typename T>
requires std::is_integral_v<T> constexpr T big_endian(T v)
{
    if constexpr (std::endian::native == std::endian::little) {
        return std::byteswap(v);
    } else {
        return v;
    }
}

template <typename T>
requires std::is_integral_v<T> constexpr T big_to_native_endian(T v)
{
    if constexpr (std::endian::native == std::endian::big) {
        return v;
    } else {
        return std::byteswap(v);
    }
}

template <typename T>
requires std::is_integral_v<T> constexpr T little_to_native_endian(T v)
{
    if constexpr (std::endian::native == std::endian::little) {
        return v;
    } else {
        return std::byteswap(v);
    }
}

