//
// Created by Keith on 1/30/2021.
//

#pragma once

#include "Array2D.h"
#include "Endian.h"
#include "RGB.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

class ImageError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

using Image = Array2D<RGB>;

inline float rgb_to_srgb(float u) noexcept
{
    if (u <= 0.0031308f) {
        return 12.92f * u;
    } else {
        return 1.055f * std::pow(u, 1.0f/2.4f) - 0.055f;
    }
}

inline RGB rgb_to_srgb(const RGB& c) noexcept
{
    return RGB{rgb_to_srgb(c.r), rgb_to_srgb(c.g), rgb_to_srgb(c.b)};
}

inline void write_ppm(std::ostream& outs, const Image& img)
{
    const int nx = img.width();
    const int ny = img.height();
    outs << "P3\n" << nx << ' ' << ny << "\n255\n";
    for (int j = ny-1; j >= 0; --j) {
        for (int i = 0; i < nx; ++i) {
            const RGB c = rgb_to_srgb(img(i, j));
            //const RGB& c = fb[pixel_index];
            const int ir = static_cast<int>(255.99f*c.r);
            const int ig = static_cast<int>(255.99f*c.g);
            const int ib = static_cast<int>(255.99f*c.b);
            outs << ir << ' ' << ig << ' ' << ib << '\n';
        }
    }
}

inline void write_ppm(const std::filesystem::path& file, const Image& img)
{
    std::ofstream outs(file, std::ios_base::binary | std::ios_base::out);
    if (!outs) {
        throw ImageError("Unable to open " + file.string());
    }
    write_ppm(outs, img);
}

inline void write_pfm(std::ostream& outs, const Image& img)
{
#if   __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    const int byte_order = -1;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    const int byte_order = +1;
#endif

    const int nx = img.width();
    const int ny = img.height();
    outs << "PF\n" << nx << ' ' << ny << '\n' << byte_order << '\n';
    for (int j = ny-1; j >= 0; --j) {
        for (int i = 0; i < nx; ++i) {
            const auto& c = img(i, j);
            //std::cout << c.r << " " << c.g << " " << c.b << '\n';
            outs.write(reinterpret_cast<const char*>(&c.r), sizeof(float));
            outs.write(reinterpret_cast<const char*>(&c.g), sizeof(float));
            outs.write(reinterpret_cast<const char*>(&c.b), sizeof(float));
        }
    }
}

inline void write_pfm(const std::filesystem::path& file, const Image& img)
{
    std::ofstream outs(file, std::ios_base::binary | std::ios_base::out);
    if (!outs) {
        throw ImageError("Unable to open " + file.string());
    }
    write_pfm(outs, img);
}

inline void write(const std::filesystem::path& file, const Image& img)
{
    const auto ext = file.extension().string();
    if (ext == ".pfm") {
        write_pfm(file, img);
    } else if (ext == ".ppm") {
        write_ppm(file, img);
    } else {
        throw ImageError("Unknown file extension: " + ext);
    }
}

inline Image read_pfm(std::istream& ins)
{
    std::string format;
    std::getline(ins, format);
    if (format != "PF") {
        throw ImageError("Unexpected format");
    }
    int nx;
    int ny;
    ins >> nx;
    ins >> ny;
    float byte_order;
    ins >> byte_order;
    ins.get(); // Get last '\n'

    using ConvertFunction = std::uint32_t (*)(std::uint32_t);
    const ConvertFunction be = &big_endian;
    const ConvertFunction le = &little_endian;
    const ConvertFunction convert = (byte_order > 0) ? be : le;

    Image img(nx, ny);

    for (int j = ny-1; j >= 0; --j) {
        for (int i = 0; i < nx; ++i) {
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

inline Image read_pfm(const std::filesystem::path& file)
{
    std::ifstream ins(file, std::ios_base::binary | std::ios_base::in);
    if (!ins) {
        throw ImageError("Unable to open " + file.string());
    }
    return read_pfm(ins);
}

inline Image read(const std::filesystem::path& file)
{
    const auto ext = file.extension().string();
    if (ext == ".pfm") {
        return read_pfm(file);
    } else {
        throw ImageError("Unknown file extension: " + ext);
    }
}

constexpr float k_max_less_than_one = 0x1.fffffe0000000p-1f;

inline RGB sample_nearest_neighbor(const Image& img, float s, float t)
{
    assert(s >= 0.0f);
    assert(t >= 0.0f);
    assert(s <  1.0f);
    assert(t <  1.0f);

    using size_type = Image::size_type;

    const float u = std::round(s * img.width());
    const float v = std::round(t * img.height());

    const size_type x = std::min(static_cast<size_type>(u), img.width() - 1);
    const size_type y = std::min(static_cast<size_type>(v), img.height() - 1);

    return img(x, y);
}

inline RGB sample_bilinear(const Image& img, float s, float t)
{
    assert(s >= 0.0f);
    assert(t >= 0.0f);
    assert(s <  1.0f);
    assert(t <  1.0f);

    using size_type = Image::size_type;

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

    return v_bias *
           (u_bias          * c0 + (1.0f - u_bias) * c1) +
           (1.0f - v_bias)  *
           (u_bias          * c2 + (1.0f - u_bias) * c3);
}
