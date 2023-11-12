//
// Created by Keith on 1/30/2021.
//

#pragma once

#include "Morton.h"
#include "propagate_const.h"

#include <Logger.h>

#include <cassert>
#include <memory>

struct unitialized_t
{
};

constexpr unitialized_t unitialized;

// This array uses a space-filling curve to access elements, allowing greater cache coherency. It does it's own memory
// management, because it allocates more space than it has elements. If it wrapped a std::vector, for instance, we would
// have to put further constraints on the contained type: e.g., it will have to be default constructable (because there
// will be unused gaps in the vector, but the items would still have to be constructed).
template <typename T, std::uint32_t log_tile_size = 4, typename allocator_t = std::allocator<T>>
class Array2DSFC
{
    static constexpr int k_tile_width  = 1 << log_tile_size;
    static constexpr int k_tile_height = 1 << log_tile_size;

    using allocator_traits = std::allocator_traits<allocator_t>;

public:
    using size_type       = std::uint32_t;
    using difference_type = std::ptrdiff_t;
    using allocator_type  = allocator_t;
    using value_type      = T;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = typename allocator_traits::pointer;
    using const_pointer   = typename allocator_traits::const_pointer;

    Array2DSFC() noexcept(noexcept(Impl()))
    : m_impl()
    {
    }

    Array2DSFC(size_type width, size_type height, allocator_type allocator = allocator_type{})
    : m_impl(width, height, allocator)
    {
    }

    Array2DSFC(size_type width, size_type height, const T& val, allocator_type allocator = allocator_type{})
    : m_impl(width, height, val, allocator)
    {
    }

#if 0
    Array2DSFC(size_type width, size_type height, unitialized_t, allocator_type allocator = allocator_type{})
    : m_impl(width, height, allocator)
    , m_data(allocator_traits::allocate(m_impl.get_allocator(), memory_size(width, height)))
    {
        static_assert(std::is_scalar_v<T>, "Expects scalar types");
    }
#endif

    Array2DSFC(const Array2DSFC& other)
    : m_impl(other.m_impl)
    {
    }

    Array2DSFC(Array2DSFC&& other) noexcept
    : m_impl(std::move(other.m_impl))
    {
    }

    ~Array2DSFC() noexcept
    {
    }

    Array2DSFC& operator=(const Array2DSFC& other)
    {
        m_impl = other.m_impl;
        return *this;
    }

    Array2DSFC& operator=(Array2DSFC&& other) noexcept
    {
        m_impl = std::move(other.m_impl);
        return *this;
    }

    void swap(Array2DSFC& other) noexcept(noexcept(std::declval<Array2DSFC>().swap(other)))
    {
        m_impl.swap(other.m_impl);
    }

    allocator_type get_allocator() const noexcept
    {
        return m_impl;
    }

    size_type width() const noexcept
    {
        return m_impl.m_width;
    }

    size_type height() const noexcept
    {
        return m_impl.m_height;
    }

    reference operator()(size_type x, size_type y) noexcept
    {
        const size_type idx = m_impl.get_data_index(x, y);
        return m_impl.m_data[idx];
    }

    const_reference operator()(size_type x, size_type y) const noexcept
    {
        const size_type idx = m_impl.get_data_index(x, y);
        return m_impl.m_data[idx];
    }

private:
    // TODO: replace with [[no_unique_address]] (or [[msvc::no_unique_address]])
    struct Impl : public allocator_type
    {
        Impl() noexcept(noexcept(allocator_type{}))
        : allocator_type()
        , m_width(0)
        , m_height(0)
        , m_data(nullptr)
        {
        }

        Impl(size_type width, size_type height) noexcept(noexcept(allocator_type{}))
        : allocator_type()
        , m_width(width)
        , m_height(height)
        , m_data(allocator_traits::allocate(m_impl.get_allocator(), memory_size(width, height)))
        {
            LOG_DEBUG("Allocated storage for {} objects", memory_size(width, height));
            LOG_DEBUG("Tiles width: {}", num_tiles_width(width));
            LOG_DEBUG("Tiles height: {}", num_tiles_height(height));
            construct();
        }

        Impl(size_type width, size_type height, allocator_type allocator)
        : allocator_type(allocator)
        , m_width(width)
        , m_height(height)
        , m_data(allocator_traits::allocate(this->get_allocator(), memory_size(width, height)))
        {
            LOG_DEBUG("Allocated storage for {} objects", memory_size(width, height));
            LOG_DEBUG("Tiles width: {}", num_tiles_width(width));
            LOG_DEBUG("Tiles height: {}", num_tiles_height(height));
            construct();
        }

        Impl(size_type width, size_type height, const T& val, allocator_type allocator)
        : allocator_type(allocator)
        , m_width(width)
        , m_height(height)
        , m_data(allocator_traits::allocate(this->get_allocator(), memory_size(width, height)))
        {
            LOG_DEBUG("Allocated storage for {} objects", memory_size(width, height));
            LOG_DEBUG("Tiles width: {}", num_tiles_width(width));
            LOG_DEBUG("Tiles height: {}", num_tiles_height(height));
            construct(val);
        }

        Impl(const Impl& other)
        : allocator_type(allocator_traits::select_on_container_copy_construction(other))
        , m_width(other.m_width)
        , m_height(other.m_height)
        , m_data(allocator_traits::allocate(this->get_allocator(), memory_size(m_width, m_height)))
        {
            for (size_type y = 0; y < m_height; ++y) {
                for (size_type x = 0; x < m_width; ++x) {
                    const auto idx = get_data_index(x, y);
                    T* const   p   = m_data + idx;
                    allocator_traits::construct(this->get_allocator(), p, other.m_data[idx]);
                }
            }
        }

        Impl(Impl&& other) noexcept
        : allocator_type(std::move(other))
        , m_width(other.m_width)
        , m_height(other.m_height)
        , m_data(std::move(other.m_data))
        {
            other.m_width  = 0;
            other.m_height = 0;
            other.m_data   = nullptr;
        }

        ~Impl()
        {
            destroy();
            allocator_traits::deallocate(this->get_allocator(), m_data, memory_size(m_width, m_height));
        }

        static bool allocators_equal(const allocator_type& a, const allocator_type& b) noexcept
        {
            return allocator_traits::is_always_equal || a == b;
        }

        Impl& operator=(const Impl& other)
        {
            // If we have to propagate allocators, and the allocators are not equal, we have to deallocate and
            // re-allocate. If we don't have to propagate, we have to reallocate if the size has changed.

            //            | allocator == | allocator != |
            // -----------+--------------+--------------+
            //  propagate |            N |            Y |
            // -----------+--------------+--------------+
            // !propagate |
            // -----------+--------------+--------------+

            constexpr bool propagate = allocator_traits::propagate_on_container_copy_assignment;
            const bool     realloc   = (propagate && !allocators_equal(*this, other)) ||
                                 (m_width != other.m_width || m_height != other.m_height);

            destroy();

            if (realloc) {
                allocator_traits::deallocate(this->get_allocator(), m_data, memory_size(m_width, m_height));
            }

            if (propagate) {
                static_cast<allocator_type>(*this) = static_cast<allocator_type>(other);
            }

            // These may already be equal
            m_width  = other.m_width;
            m_height = other.m_height;

            if (realloc) {
                m_data = allocator_traits::allocate(m_impl.get_allocator(), memory_size(m_width, m_height));
            }

            for (size_type y = 0; y < m_height; ++y) {
                for (size_type x = 0; x < m_width; ++x) {
                    const auto idx = get_data_index(x, y);
                    T* const   p   = m_data + idx;
                    allocator_traits::construct(this->get_allocator(), p, other.m_data[idx]);
                }
            }
            return *this;
        }

        Impl& operator=(Impl&& other) // TODO: noexcept clause
        {
            if constexpr (allocator_traits::propagate_on_container_move_assignment) {
                destroy();
                allocator_traits::deallocate(this->get_allocator(), m_data, memory_size(m_width, m_height));

                static_cast<allocator_type>(*this) = std::move(static_cast<allocator_type>(other));

                m_width  = other.m_width;
                m_height = other.m_height;
                m_data   = other.m_data;

                other.m_width  = 0;
                other.m_height = 0;
                other.m_data   = nullptr;
            } else {
                if (allocators_equal(*this, other)) {
                    using std::swap; // Allow ADL
                    swap(m_width, other.m_width);
                    swap(m_height, other.m_height);
                    swap(m_data, other.m_data);
                } else {
                    destroy();
                    allocator_traits::deallocate(this->get_allocator(), m_data, memory_size(m_width, m_height));

                    m_width  = other.m_width;
                    m_height = other.m_height;
                    m_data   = allocator_traits::allocate(m_impl.get_allocator(), memory_size(m_width, m_height));

                    for (size_type y = 0; y < m_height; ++y) {
                        for (size_type x = 0; x < m_width; ++x) {
                            const auto idx = get_data_index(x, y);
                            T* const   p   = m_data + idx;
                            allocator_traits::construct(this->get_allocator(),
                                                        p,
                                                        std::move_if_noexcept(other.m_data[idx]));
                        }
                    }
                }
            }
            return *this;
        }

        allocator_type& get_allocator() noexcept
        {
            return *this;
        }

        const allocator_type& get_allocator() const noexcept
        {
            return *this;
        }

        void swap_allocator(Impl& other, std::true_type)
        {
            using std::swap; // Allow ADL
            swap(get_allocator(), other.get_allocator());
        }

        void swap_allocator(Impl& other, std::false_type) noexcept
        {
            // No-op
        }

        void swap(Impl& other) noexcept(!allocator_traits::propagate_on_container_swap) // TODO: or the swap is noexcept
        {
            std::swap(m_width, other.m_width);
            std::swap(m_height, other.m_height);
            swap_allocator(other, allocator_traits::propagate_on_container_swap);
        }

        size_type get_data_index(size_type x, size_type y) const noexcept
        {
            const size_type     tile_x   = x / k_tile_width;
            const size_type     tile_y   = y / k_tile_height;
            const std::uint16_t offset_x = x % k_tile_height;
            const std::uint16_t offset_y = y % k_tile_height;

            const size_type index_in_tile = morton_encode(offset_x, offset_y);
            assert(index_in_tile < k_tile_width * k_tile_height);
            const size_type tile_index = tile_y * num_tiles_width(m_width) + tile_x;
            const size_type idx        = tile_index * (k_tile_width * k_tile_height) + index_in_tile;
            assert(idx < memory_size(m_width, m_height));
            return idx;
        }

        static constexpr size_type memory_size(size_type width, size_type height) noexcept
        {
            return num_tiles_width(width) * k_tile_width * num_tiles_height(height) * k_tile_height;
        }

        static constexpr size_type num_tiles_width(size_type width) noexcept
        {
            return (width + k_tile_width - 1) / k_tile_width;
        }

        static constexpr size_type num_tiles_height(size_type height) noexcept
        {
            return (height + k_tile_height - 1) / k_tile_height;
        }

        void construct(const T& val)
        {
            for (size_type y = 0; y < m_height; ++y) {
                for (size_type x = 0; x < m_width; ++x) {
                    T* const p = m_data + get_data_index(x, y);
                    allocator_traits::construct(this->get_allocator(), p, val);
                }
            }
        }

        void construct()
        {
            for (size_type y = 0; y < m_height; ++y) {
                for (size_type x = 0; x < m_width; ++x) {
                    T* const p = m_data + get_data_index(x, y);
                    allocator_traits::construct(this->get_allocator(), p);
                }
            }
        }

        void destroy() noexcept
        {
            for (size_type y = 0; y < m_height; ++y) {
                for (size_type x = 0; x < m_width; ++x) {
                    T* const p = m_data + get_data_index(x, y);
                    allocator_traits::destroy(this->get_allocator(), p);
                }
            }
        }

        size_type           m_width;
        size_type           m_height;
        propagate_const<T*> m_data;
        // T* m_data;
    };

    Impl m_impl;
};

template <typename T, typename allocator_t = std::allocator<T>>
class Array2D
{
    using MemoryContainer = std::vector<T, allocator_t>;

    using allocator_traits = std::allocator_traits<allocator_t>;

public:
    using size_type       = std::uint32_t;
    using difference_type = std::ptrdiff_t;
    using allocator_type  = allocator_t;
    using value_type      = T;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = typename allocator_traits::pointer;
    using const_pointer   = typename allocator_traits::const_pointer;

    Array2D() = default;

    Array2D(size_type width, size_type height, allocator_type allocator = allocator_type{})
    : m_width(width)
    , m_data(width * height, allocator)
    {
    }

    Array2D(size_type width, size_type height, const T& val, allocator_type allocator = allocator_type{})
    : m_width(width)
    , m_data(width * height, val, allocator)
    {
    }

#if 0
    Array2D(size_type width, size_type height, unitialized_t, allocator_type allocator = allocator_type{})
    {
    }
#endif

    Array2D(const Array2D&) = default;
    Array2D(Array2D&&)      = default;

    Array2D& operator=(const Array2D&)  = default;
    Array2D& operator=(Array2D&& other) = default;

    void swap(Array2D& other) noexcept(noexcept(std::declval<MemoryContainer>().swap(other.m_data)))
    {
        using std::swap; // Allow ADL
        swap(m_width, other.m_width);
        swap(m_data, other.m_data);
    }

    allocator_type get_allocator() const noexcept
    {
        return m_data.get_allocator();
    }

    size_type width() const noexcept
    {
        return m_width;
    }

    size_type height() const noexcept
    {
        // One of the drawbacks of storing in a vector is that we either over-specify by storing width and height along
        // with the vector's storing of size, or we do this calculation.
        assert(m_width > 0);
        return m_data.size() / m_width;
    }

    reference operator()(size_type x, size_type y) noexcept
    {
        const size_type idx = get_data_index(x, y);
        return m_data[idx];
    }

    const_reference operator()(size_type x, size_type y) const noexcept
    {
        const size_type idx = get_data_index(x, y);
        return m_data[idx];
    }

    pointer data()
    {
        return m_data.data();
    }

    const_pointer data() const
    {
        return m_data.data();
    }

private:
    size_type get_data_index(size_type x, size_type y) const noexcept
    {
        return m_width * y + x;
    }

    size_type       m_width;
    MemoryContainer m_data;
};
