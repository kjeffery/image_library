
#include "Array2D.h"
#include "Image.h"

#include <cmath>
#include <iostream>
#include <random>

struct NoDefault
{
    NoDefault() = delete;
    NoDefault(int, int)         { std::cerr << __FUNCTION__ << '\n'; }
    NoDefault(const NoDefault&) { std::cerr << __FUNCTION__ << '\n'; }
    NoDefault(NoDefault&&)      { std::cerr << __FUNCTION__ << '\n'; }
    ~NoDefault()                { std::cerr << __FUNCTION__ << '\n'; }
};

void test_array()
{
    Array2D<int> array(23, 47);

    int counter = 0;
    for (std::uint32_t y = 0; y < array.height(); ++y) {
        for (std::uint32_t x = 0; x < array.width(); ++x) {
            array(x, y) = counter++;
        }
    }

    counter = 0;
    for (std::uint32_t y = 0; y < array.height(); ++y) {
        for (std::uint32_t x = 0; x < array.width(); ++x) {
            if (array(x, y) != counter++) {
                std::cerr << "Error at (" << x << ", " << y << ")\n";
            }
        }
    }
}

Image rescale_image(const Image& input, Image::size_type width, Image::size_type height)
{
    using size_type = Image::size_type;

    Image out(width, height);
    for (size_type y = 0; y < height; ++y) {
        for (size_type x = 0; x < width; ++x) {
            const float s = static_cast<float>(x) / width;
            const float t = static_cast<float>(y) / height;
            //out(x, y) = sample_nearest_neighbor(input, s, t);
            out(x, y) = sample_bilinear(input, s, t);
        }
    }
    return out;
}

struct Point
{
    float x;
    float y;
};

float mod1(float x) noexcept
{
    float throw_away;
    return std::modf(x, &throw_away);
}

double mod1(double x) noexcept
{
    double throw_away;
    return std::modf(x, &throw_away);
}

template <typename RNG>
float canonical(RNG& rng)
{
    static std::uniform_real_distribution<float> dist;
    float u;
    do {
        u = dist(rng);
    } while (u >= 1.0f);
    return u;
}

float to_float(unsigned n) noexcept
{
    constexpr float d = float(1<<24);
    return (n >> 8)/d;
}

inline unsigned reverseBits(unsigned n) noexcept
{
    n = (n << 16u) | (n >> 16u);
    n = ((n & 0x00ff00ffu) << 8u) | ((n & 0xff00ff00u) >> 8u);
    n = ((n & 0x0f0f0f0fu) << 4u) | ((n & 0xf0f0f0f0u) >> 4u);
    n = ((n & 0x33333333u) << 2u) | ((n & 0xccccccccu) >> 2u);
    n = ((n & 0x55555555u) << 1u) | ((n & 0xaaaaaaaau) >> 1u);
    return n;
}

inline float van_der_corput(unsigned n, unsigned scramble)
{
    n = reverseBits(n);
    n ^= scramble;
    return to_float(n);
}

float sobol2(unsigned n, unsigned seed) noexcept
{
    unsigned s = seed;
    for (unsigned v = 1u << 31u; n != 0; n >>= 1, v ^= (v >> 1)) {
        if (n & 0x1) {
            s ^= v;
        }
    }
    return to_float(s);
}

Point sample02(unsigned n, unsigned seed0, unsigned seed1) noexcept
{
    return Point{van_der_corput(n, seed0), sobol2(n, seed1)};
}

Point fibonacci_additive_recurrence(int n, int total_samples) noexcept
{
    static const float phi = (std::sqrt(5.0f) + 1.0f) / 2.0f;

    const float j = static_cast<float>(n);
    Point p{mod1(0.5f + j * phi), j/total_samples};
    return p;
}

Point r_sequence(int n, double seed = 0.5f) noexcept
{
    constexpr double g = 1.32471795724474602596;
    constexpr double a1 = 1.0/g;
    constexpr double a2 = 1.0/(g*g);

    const float x = std::min(static_cast<float>(mod1(seed + a1*n)), k_max_less_than_one);
    const float y = std::min(static_cast<float>(mod1(seed + a2*n)), k_max_less_than_one);
    Point p{x, y};
    return p;
}

float triangle_filter(float u, float extents) noexcept
{
    float val;
    if (u < 0.5f) {
        val = std::sqrt(2.0f * u) - 1.0f;
    } else {
        val = 1.0f - std::sqrt(2.0f * (1.0f - u));
    }
    // Val in [-1, 1)

    assert(val >= -1.0f);
    assert(val <   1.0f);

    val = ((val + 1.0f) * 0.5f); // Val in [0, 1)
    assert(val >= 0.0f);
    assert(val <  1.0f);

    val *= extents; // Val now in [0, extents)
    assert(val >= 0.0f);
    assert(val <  extents);

    val -= extents * 0.5f;
    return val;
}

template <typename RNG>
std::vector<Point> multijitter(int n, int m, RNG& rng)
{
    const int n_samples = n*m;
    std::vector<Point> samples(n_samples);

    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < m; ++i) {
            samples[j*m + i].x = std::min((i + (j + canonical(rng)) / n) / m, k_max_less_than_one);
            samples[j*m + i].y = std::min((j + (i + canonical(rng)) / m) / n, k_max_less_than_one);
        }
    }

    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < m; ++i) {
            const float u = canonical(rng);
            const int k = j + static_cast<int>(u * (n - j));
            std::swap(samples[j * m + i].x, samples[k * m + i].x);
        }
    }

    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            const float u = canonical(rng);
            const int k = i + static_cast<int>(u * (m - i));
            std::swap(samples[j * m + i].y, samples[j * m + k].y);
        }
    }
    std::shuffle(samples.begin(), samples.end(), rng);
    return samples;
}
#if 0
template <typename RNG>
std::vector<Point> multijitter(int sqrt_n_samples, RNG& rng)
{
    const int n_samples = sqrt_n_samples*sqrt_n_samples;
    std::vector<Point> samples(n_samples);

    const float subcell_width = 1.0f/n_samples;

    for (int i = 0; i < sqrt_n_samples; ++i) {
        for (int j = 0; j < sqrt_n_samples; ++j) {
            samples[i*sqrt_n_samples + j].x = i*sqrt_n_samples*subcell_width + j*subcell_width + canonical(rng)*subcell_width;
            samples[i*sqrt_n_samples + j].y = j*sqrt_n_samples*subcell_width + i*subcell_width + canonical(rng)*subcell_width;
        }
    }
}
#endif

struct SamplerBilinear
{
    RGB operator()(const Image& img, float s, float t)
    {
        return sample_bilinear(img, s, t);
    }
};

class SamplerGauss
{
public:
    SamplerGauss(int n_samples, float extents)
    : m_num_samples(n_samples)
    , m_extents(extents)
    , m_rng()
    , m_dist(0.0f, 3.0f*extents)
    {
    }

    RGB operator()(const Image& img, float s, float t)
    {
        RGB ret;
        for (int i = 0; i < m_num_samples; ++i) {
            const float x = gen_sample(s);
            const float y = gen_sample(t);
            ret += sample_bilinear(img, x, y);
        }
        return ret / m_num_samples;
    }

private:
    float gen_sample(float f)
    {
        float s;
        do {
            //s = f + std::clamp(m_dist(m_rng), -m_extents, m_extents);
            s = f + m_dist(m_rng);
        } while (s < 0.0f || s >= 1.0f);
        return s;
    }

    int m_num_samples;
    float m_extents;
    std::mt19937 m_rng;
    std::normal_distribution<float> m_dist;
};

class SamplerTriangle
{
public:
    explicit SamplerTriangle(int nsamples) noexcept
    : m_rng()
    , m_num_samples(nsamples)
    {
    }

    RGB operator()(const Image& img, float s, float t)
    {
        RGB ret;

        constexpr float extents = 0.05f;

        int samples_taken = 0;

        const auto points = multijitter(m_num_samples, m_num_samples, m_rng);
        for (const auto& p : points) {
            const float offset_s = s + triangle_filter(p.x, extents);
            const float offset_t = t + triangle_filter(p.y, extents);
            if (offset_s < 0.0f || offset_s >= 1.0f || offset_t < 0.0f || offset_t >= 1.0f) {
                continue;
            }
            ret += sample_bilinear(img, offset_s, offset_t);
            ++samples_taken;
        }
        if (samples_taken > 0) {
            return ret / samples_taken;
        } else {
            return ret;
        }
    }

private:
    std::mt19937 m_rng;
    int m_num_samples;
};

class SharedSamplerTriangle
{
public:
    // Need image samples * nsamples for multijittered
    explicit SharedSamplerTriangle(int nsamples, std::vector<Point>&& samples) noexcept
    : m_num_samples(nsamples)
    , m_shared_sample_num(0)
    , m_samples(std::move(samples))
    {
    }

    RGB operator()(const Image& img, float s, float t)
    {
        RGB ret;

        constexpr float extents = 0.05f;

        int samples_taken = 0;

        for (int i = 0; i < m_num_samples; ++i) {
            const auto& p = m_samples[m_shared_sample_num++];
            const float offset_s = s + triangle_filter(p.x, extents);
            const float offset_t = t + triangle_filter(p.y, extents);
            if (offset_s < 0.0f || offset_s >= 1.0f || offset_t < 0.0f || offset_t >= 1.0f) {
                continue;
            }
            ret += sample_bilinear(img, offset_s, offset_t);
            ++samples_taken;
        }
        if (samples_taken > 0) {
            return ret / samples_taken;
        } else {
            return ret;
        }
    }

private:

    int m_num_samples;
    int m_shared_sample_num;
    std::vector<Point> m_samples;
};

template <typename Sampler>
Image reconstruct_image(Sampler& sampler, const Image& input, Image::size_type width, Image::size_type height, Image::size_type nsamples)
{
    using size_type = Image::size_type;

    Image ret(width, height);
    Array2D<int> count(width, height, 0);

    for (size_type i = 0; i < nsamples; ++i) {
        const Point st = r_sequence(i);
        const size_type pixel_x = std::min(static_cast<size_type>(st.x * width),  width - 1);
        const size_type pixel_y = std::min(static_cast<size_type>(st.y * height), height - 1);
        ret(pixel_x, pixel_y) += sampler(input, st.x, st.y);
        count(pixel_x, pixel_y) += 1;
    }

    for (size_type y = 0; y < height; ++y) {
        for (size_type x = 0; x < width; ++x) {
            const int c = count(x, y);
            if (c > 0) {
                ret(x, y) /= count(x, y);
            }

        }
    }
    return ret;
}

int main()
{
    using size_type = Image::size_type;

    test_array();

    Image img = read(R"(C:\Users\Keith\Downloads\memorial.pfm)");

    //SamplerBilinear sampler_bilinear;
    //write("binliear.pfm", reconstruct_image(sampler_bilinear, img));

    //SamplerGauss sampler_gauss1(1, 0.001f);
    //write("gauss1.pfm", reconstruct_image(sampler_gauss1, img));

    //SamplerGauss sampler_gauss2(2, 0.001f);
    //write("gauss2.pfm", reconstruct_image(sampler_gauss2, img));

    //SamplerGauss sampler_gauss3(3, 0.001f);
    //write("gauss3.pfm", reconstruct_image(sampler_gauss3, img));

#if 1
    const size_type width  = img.width() * 3 / 2;
    const size_type height = img.height() * 3 / 2;
    const size_type max_dimension = std::max(width, height);
    const size_type n_pixel_samples_sqrt = max_dimension*2;
    const size_type n_pixel_samples = n_pixel_samples_sqrt*n_pixel_samples_sqrt;

    SamplerTriangle sampler_triangle1(1); // total samples = n_pixel_samples * 1*1
    write("triangle1.pfm", reconstruct_image(sampler_triangle1, img, width, height, n_pixel_samples));

    SamplerTriangle sampler_triangle2(2); // total samples = n_pixel_samples * 2*2
    write("triangle2.pfm", reconstruct_image(sampler_triangle2, img, width, height, n_pixel_samples));

    SamplerTriangle sampler_triangle3(3); // total samples = n_pixel_samples * 3*3
    write("triangle3.pfm", reconstruct_image(sampler_triangle3, img, width, height, n_pixel_samples));

    SamplerTriangle sampler_triangle4(4); // total samples = n_pixel_samples * 4*4
    write("triangle4.pfm", reconstruct_image(sampler_triangle4, img, width, height, n_pixel_samples));

    std::mt19937 rng;
    std::vector<Point> shared_points1 = multijitter(n_pixel_samples_sqrt*1, n_pixel_samples_sqrt*1, rng);
    SharedSamplerTriangle shared_sampler_triangle1(1*1, std::move(shared_points1));
    write("shared_triangle1.pfm", reconstruct_image(shared_sampler_triangle1, img, width, height, n_pixel_samples));

    std::vector<Point> shared_points2 = multijitter(n_pixel_samples_sqrt*2, n_pixel_samples_sqrt*2, rng);
    SharedSamplerTriangle shared_sampler_triangle2(2*2, std::move(shared_points2));
    write("shared_triangle2.pfm", reconstruct_image(shared_sampler_triangle2, img, width, height, n_pixel_samples));

    std::vector<Point> shared_points3 = multijitter(n_pixel_samples_sqrt*3, n_pixel_samples_sqrt*3, rng);
    SharedSamplerTriangle shared_sampler_triangle3(3*3, std::move(shared_points3));
    write("shared_triangle3.pfm", reconstruct_image(shared_sampler_triangle3, img, width, height, n_pixel_samples));

    std::vector<Point> shared_points4 = multijitter(n_pixel_samples_sqrt*4, n_pixel_samples_sqrt*4, rng);
    SharedSamplerTriangle shared_sampler_triangle4(4*4, std::move(shared_points4));
    write("shared_triangle4.pfm", reconstruct_image(shared_sampler_triangle4, img, width, height, n_pixel_samples));
#else
    std::mt19937 rng;
    std::vector<Point> test_points = multijitter(16, 16, rng);
    std::ofstream outs("points.dat");
    for (const auto& p : test_points) {
        outs << p.x << ' ' << p.y << '\n';
    }
#endif
}
