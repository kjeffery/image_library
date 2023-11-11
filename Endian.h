//
// Created by Keith on 1/30/2021.
//

#pragma once

#include <cstdint>

#if defined(_MSC_FULL_VER)

#include <cstdlib>

inline std::uint16_t little_endian(std::uint16_t v)
{
#if   __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return v;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return _byteswap_ushort(v);
#else
    #error Unsupported endianess
#endif
}

inline std::uint16_t big_endian(std::uint16_t v)
{
#if   __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return _byteswap_ushort(v);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return v;
#else
    #error Unsupported endianess
#endif
}

inline std::uint32_t little_endian(std::uint32_t v)
{
#if   __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return v;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return _byteswap_ulong(v);
#else
    #error Unsupported endianess
#endif
}

inline std::uint32_t big_endian(std::uint32_t v)
{
#if   __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return _byteswap_ulong(v);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return v;
#else
    #error Unsupported endianess
#endif
}

inline std::uint64_t little_endian(std::uint64_t v)
{
#if   __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return v;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return _byteswap_uint64(v);
#else
    #error Unsupported endianess
#endif
}

inline std::uint64_t big_endian(std::uint64_t v)
{
#if   __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return _byteswap_uint64(v);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return v;
#else
    #error Unsupported endianess
#endif
}

#else

#include <byteswap.h>

inline std::uint16_t little_endian(std::uint16_t v)
{
#if   __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return v;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return __bswap_16(v);
#else
    #error Unsupported endianess
#endif
}

inline std::uint16_t big_endian(std::uint16_t v)
{
#if   __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __bswap_16(v);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return v;
#else
    #error Unsupported endianess
#endif
}

inline std::uint32_t little_endian(std::uint32_t v)
{
#if   __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return v;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return __bswap_32(v);
#else
    #error Unsupported endianess
#endif
}

inline std::uint32_t big_endian(std::uint32_t v)
{
#if   __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __bswap_32(v);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return v;
#else
    #error Unsupported endianess
#endif
}

inline std::uint64_t little_endian(std::uint64_t v)
{
#if   __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return v;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return __bswap_64(v);
#else
    #error Unsupported endianess
#endif
}

inline std::uint64_t big_endian(std::uint64_t v)
{
#if   __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __bswap_64(v);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return v;
#else
    #error Unsupported endianess
#endif
}

#endif

