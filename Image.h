//
// Created by Keith on 1/30/2021.
//

#pragma once

#include "Array2D.h"
#include "Endian.h"
#include "IgnoreLineCommentsBuf.h"
#include "RGB.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

enum class ImageFormat
{
    unknown,
    PPM_binary,
    PPM_ascii,
    PFM
};

struct PNM_header
{
    ImageFormat   format{ ImageFormat::unknown };
    std::uint32_t width{ 0 };
    std::uint32_t height{ 0 };
    std::endian   byte_order{ std::endian::big }; // 16-bit PPM is big: only PFM changes this
    std::uint16_t max_color{ 0 };
};

class ImageError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

using ImageSFCf = Array2DSFC<RGBf>;
using Image_f   = Array2D<RGBf>;
using Image_8   = Array2D<RGB8>;
using Image_16  = Array2D<RGB16>;
using Image_32  = Array2D<RGB32>;

inline float rgb_to_srgb(const float u) noexcept
{
    if (u <= 0.0031308f) {
        return 12.92f * u;
    } else {
        return 1.055f * std::pow(u, 1.0f / 2.4f) - 0.055f;
    }
}

inline float srgb_to_rgb(const float u) noexcept
{
    if (u > 0.04045f) {
        return std::pow((u + 0.055f) / 1.055f, 2.4f);
    } else {
        return u / 12.92f;
    }
}

inline RGBf rgb_to_srgb(const RGBf& c) noexcept
{
    return { rgb_to_srgb(c.r), rgb_to_srgb(c.g), rgb_to_srgb(c.b) };
}

inline RGBf srgb_to_rgb(const RGBf& c) noexcept
{
    return { srgb_to_rgb(c.r), srgb_to_rgb(c.g), srgb_to_rgb(c.b) };
}

class LineCommentStreamBufDecorator
{
public:
    explicit LineCommentStreamBufDecorator(std::istream& ins)
    : m_new_streambuf(ins.rdbuf())
    , m_original_streambuf(ins.rdbuf())
    , m_stream(std::addressof(ins))
    {
        ins.rdbuf(std::addressof(m_new_streambuf));
    }

    ~LineCommentStreamBufDecorator()
    {
        m_stream->rdbuf(m_original_streambuf);
    }

private:
    IgnoreLineCommentsBuf m_new_streambuf;
    std::streambuf*       m_original_streambuf;
    std::istream*         m_stream;
};

// Post-condition: ins is set to read image data values.
PNM_header read_pnm_header(std::istream& ins)
{
    ins.seekg(0);

    // Skip over comments in header reading by wrapping the stream in an RAII construct that changes out the streambuf.
    LineCommentStreamBufDecorator stream_raii(ins);

    PNM_header header;

    std::string format;
    ins >> format;

    if (format == "P3") {
        header.format = ImageFormat::PPM_ascii;
    } else if (format == "P6") {
        header.format = ImageFormat::PPM_binary;
    } else if (format == "PF") {
        header.format = ImageFormat::PFM;
    } else {
        return header;
    }

    ins >> header.width;
    ins >> header.height;

    if (header.format == ImageFormat::PFM) {
        float byte_order;
        ins >> byte_order;
        header.byte_order = (byte_order > 0) ? std::endian::big : std::endian::little;
    } else {
        ins >> header.max_color;
    }
    ins.get(); // Get final whitespace character

    return header;
}

template <typename ImageType>
inline void write_ppm_8(std::ostream& outs, const ImageType& img)
{
    constexpr uint8_t max_value = 255;

    const int nx = img.width();
    const int ny = img.height();
    println(outs, "P6");
    println(outs, "{} {}", nx, ny);
    println(outs, "{}", max_value);
    for (int j = ny - 1; j >= 0; --j) {
        for (int i = 0; i < nx; ++i) {
            const RGBf c  = rgb_to_srgb(clamp(to_float(img(i, j))));
            const auto ir = static_cast<uint8_t>(max_value * c.r);
            const auto ig = static_cast<uint8_t>(max_value * c.g);
            const auto ib = static_cast<uint8_t>(max_value * c.b);
            outs.write(reinterpret_cast<const char*>(&ir), sizeof(uint8_t));
            outs.write(reinterpret_cast<const char*>(&ig), sizeof(uint8_t));
            outs.write(reinterpret_cast<const char*>(&ib), sizeof(uint8_t));
        }
    }
}

template <typename ImageType>
inline void write_ppm_16(std::ostream& outs, const ImageType& img)
{
    constexpr uint16_t max_value = 65'535;

    const int nx = img.width();
    const int ny = img.height();
    println(outs, "P6");
    println(outs, "{} {}", nx, ny);
    println(outs, "{}", max_value);
    for (int j = ny - 1; j >= 0; --j) {
        for (int i = 0; i < nx; ++i) {
            const RGBf c  = rgb_to_srgb(clamp(to_float(img(i, j))));
            const auto ir = big_endian(static_cast<uint16_t>(max_value * c.r));
            const auto ig = big_endian(static_cast<uint16_t>(max_value * c.g));
            const auto ib = big_endian(static_cast<uint16_t>(max_value * c.b));
            outs.write(reinterpret_cast<const char*>(&ir), sizeof(uint16_t));
            outs.write(reinterpret_cast<const char*>(&ig), sizeof(uint16_t));
            outs.write(reinterpret_cast<const char*>(&ib), sizeof(uint16_t));
        }
    }
}

template <typename ImageType>
inline void write_plain_ppm(std::ostream& outs, const ImageType& img)
{
    const int nx = img.width();
    const int ny = img.height();
    println(outs, "P3");
    println(outs, "{} {}", nx, ny);
    println(outs, "255");
    for (int j = ny - 1; j >= 0; --j) {
        for (int i = 0; i < nx; ++i) {
            const RGBf c  = rgb_to_srgb(clamp(to_float(img(i, j))));
            const auto ir = static_cast<int>(255.0f * c.r);
            const auto ig = static_cast<int>(255.0f * c.g);
            const auto ib = static_cast<int>(255.0f * c.b);
            std::println(outs, "{} {} {}", ir, ig, ib);
        }
    }
}

template <typename ImageType>
inline void write_ppm_8(const std::filesystem::path& file, const ImageType& img)
{
    std::ofstream outs(file, std::ios_base::binary | std::ios_base::out);
    if (!outs) {
        throw ImageError("Unable to open " + file.string());
    }
    write_ppm_8(outs, img);
}

template <typename ImageType>
inline void write_ppm_16(const std::filesystem::path& file, const ImageType& img)
{
    std::ofstream outs(file, std::ios_base::binary | std::ios_base::out);
    if (!outs) {
        throw ImageError("Unable to open " + file.string());
    }
    write_ppm_16(outs, img);
}

template <typename ImageType>
inline void write_plain_ppm(const std::filesystem::path& file, const ImageType& img)
{
    std::ofstream outs(file, std::ios_base::binary | std::ios_base::out);
    if (!outs) {
        throw ImageError("Unable to open " + file.string());
    }
    write_plain_ppm(outs, img);
}

inline void write_pfm(std::ostream& outs, const Image_f& img)
{
    constexpr int byte_order = (std::endian::native == std::endian::little) ? -1 : +1;

    const int nx = img.width();
    const int ny = img.height();
    outs << "PF\n" << nx << ' ' << ny << '\n' << byte_order << '\n';
    for (int j = ny - 1; j >= 0; --j) {
        for (int i = 0; i < nx; ++i) {
            const auto& c = img(i, j);
            outs.write(reinterpret_cast<const char*>(&c.r), sizeof(float));
            outs.write(reinterpret_cast<const char*>(&c.g), sizeof(float));
            outs.write(reinterpret_cast<const char*>(&c.b), sizeof(float));
        }
    }
}

inline void write_pfm(const std::filesystem::path& file, const Image_f& img)
{
    std::ofstream outs(file, std::ios_base::binary | std::ios_base::out);
    if (!outs) {
        throw ImageError("Unable to open " + file.string());
    }
    write_pfm(outs, img);
}

inline Image_8 read_ppm_8(std::istream& ins)
{
    constexpr auto max_value = std::numeric_limits<std::uint8_t>::max();

    const auto header = read_pnm_header(ins);

    if (header.format != ImageFormat::PPM_binary) {
        throw ImageError("Unexpected format");
    }

    if (header.max_color > max_value) {
        throw ImageError("Unexpected color depth");
    }

    Image_8 img(header.width, header.height);

    for (int j = header.height - 1; j >= 0; --j) {
        for (int i = 0; i < header.width; ++i) {
            std::uint8_t r;
            std::uint8_t g;
            std::uint8_t b;
            ins.read(reinterpret_cast<char*>(&r), sizeof(std::uint8_t));
            ins.read(reinterpret_cast<char*>(&g), sizeof(std::uint8_t));
            ins.read(reinterpret_cast<char*>(&b), sizeof(std::uint8_t));

            const auto fc = srgb_to_rgb(to_float(RGB8{r, g, b}));
            r = static_cast<uint8_t>(fc.r * max_value);
            g = static_cast<uint8_t>(fc.g * max_value);
            b = static_cast<uint8_t>(fc.b * max_value);

            img(i, j) = RGB8(r, g, b);
        }
    }

    return img;
}

inline Image_8 read_ppm_8(const std::filesystem::path& file)
{
    std::ifstream ins(file, std::ios_base::binary | std::ios_base::in);
    if (!ins) {
        throw ImageError("Unable to open " + file.string());
    }
    return read_ppm_8(ins);
}

inline Image_16 read_ppm_16(std::istream& ins)
{
    constexpr auto max_value = std::numeric_limits<std::uint16_t>::max();

    const auto header = read_pnm_header(ins);

    if (header.format != ImageFormat::PPM_binary) {
        throw ImageError("Unexpected format");
    }

    if (header.max_color <= std::numeric_limits<std::uint8_t>::max() || header.max_color > max_value) {
        throw ImageError("Unexpected color depth");
    }

    Image_16 img(header.width, header.height);

    for (int j = header.height - 1; j >= 0; --j) {
        for (int i = 0; i < header.width; ++i) {
            std::uint16_t r;
            std::uint16_t g;
            std::uint16_t b;
            ins.read(reinterpret_cast<char*>(&r), sizeof(std::uint16_t));
            ins.read(reinterpret_cast<char*>(&g), sizeof(std::uint16_t));
            ins.read(reinterpret_cast<char*>(&b), sizeof(std::uint16_t));

            r = big_to_native_endian(r);
            g = big_to_native_endian(g);
            b = big_to_native_endian(b);

            const auto fc = srgb_to_rgb(to_float(RGB16{r, g, b}));
            r = static_cast<uint16_t>(fc.r * max_value);
            g = static_cast<uint16_t>(fc.g * max_value);
            b = static_cast<uint16_t>(fc.b * max_value);

            img(i, j) = RGB16(r, g, b);
        }
    }

    return img;
}

inline Image_16 read_ppm_16(const std::filesystem::path& file)
{
    std::ifstream ins(file, std::ios_base::binary | std::ios_base::in);
    if (!ins) {
        throw ImageError("Unable to open " + file.string());
    }
    return read_ppm_16(ins);
}

// TODO: this should also work with space-filling version
inline Image_f read_pfm(std::istream& ins)
{
    const auto header = read_pnm_header(ins);
    if (header.format != ImageFormat::PFM) {
        throw ImageError("Unexpected format");
    }

    using ConvertFunction = std::uint32_t (*)(std::uint32_t);

    const ConvertFunction be      = &big_endian;
    const ConvertFunction le      = &little_endian;
    const ConvertFunction convert = (header.byte_order == std::endian::big) ? be : le;

    Image_f img(header.width, header.height);

    for (int j = header.height - 1; j >= 0; --j) {
        for (int i = 0; i < header.width; ++i) {
            std::uint32_t r;
            std::uint32_t g;
            std::uint32_t b;
            ins.read(reinterpret_cast<char*>(&r), sizeof(std::uint32_t));
            ins.read(reinterpret_cast<char*>(&g), sizeof(std::uint32_t));
            ins.read(reinterpret_cast<char*>(&b), sizeof(std::uint32_t));

            r = convert(r);
            g = convert(g);
            b = convert(b);

            const float fr = *reinterpret_cast<float*>(&r);
            const float fg = *reinterpret_cast<float*>(&g);
            const float fb = *reinterpret_cast<float*>(&b);

            img(i, j) = RGB(fr, fg, fb);
        }
    }

    return img;
}

inline Image_f read_pfm(const std::filesystem::path& file)
{
    std::ifstream ins(file, std::ios_base::binary | std::ios_base::in);
    if (!ins) {
        throw ImageError("Unable to open " + file.string());
    }
    return read_pfm(ins);
}

constexpr float k_max_less_than_one = 0x1.fffffe0000000p-1f;

template <typename ImageType>
inline auto sample_nearest_neighbor(const ImageType& img, float s, float t)
{
    // clang-format off
    assert(s >= 0.0f);
    assert(t >= 0.0f);
    assert(s <  1.0f);
    assert(t <  1.0f);
    // clang-format on

    using size_type = typename ImageType::size_type;

    const float u = std::round(s * img.width());
    const float v = std::round(t * img.height());

    const size_type x = std::min(static_cast<size_type>(u), img.width() - 1);
    const size_type y = std::min(static_cast<size_type>(v), img.height() - 1);

    return img(x, y);
}

template <typename ImageType>
inline auto sample_bilinear(const ImageType& img, float s, float t)
{
    // clang-format off
    assert(s >= 0.0f);
    assert(t >= 0.0f);
    assert(s <  1.0f);
    assert(t <  1.0f);
    // clang-format on

    using size_type = typename ImageType::size_type;

    const float u = s * img.width();
    const float v = t * img.height();

    const float u_lower = std::floor(u);
    const float u_upper = std::ceil(u);
    const float v_lower = std::floor(v);
    const float v_upper = std::ceil(v);

    const float u_bias = u_upper - u_lower;
    const float v_bias = v_upper - v_lower;

    const size_type x_lower = std::min(static_cast<size_type>(u_lower), img.width() - 1);
    const size_type x_upper = std::min(static_cast<size_type>(u_upper), img.width() - 1);
    const size_type y_lower = std::min(static_cast<size_type>(v_lower), img.height() - 1);
    const size_type y_upper = std::min(static_cast<size_type>(v_upper), img.height() - 1);

    const auto& c0 = img(x_lower, y_lower);
    const auto& c1 = img(x_upper, y_lower);
    const auto& c2 = img(x_lower, y_upper);
    const auto& c3 = img(x_upper, y_upper);

    // clang-format off
    return v_bias *
           (u_bias          * c0 + (1.0f - u_bias) * c1) +
           (1.0f - v_bias)  *
           (u_bias          * c2 + (1.0f - u_bias) * c3);
    // clang-format on
}
