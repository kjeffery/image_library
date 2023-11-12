#pragma once

#include <compare>
#include <type_traits>

template <typename T>
class propagate_const;

template <typename U>
struct is_propagate_const : std::false_type
{
};

template <typename U>
struct is_propagate_const<propagate_const<U>> : std::true_type
{
};

template <typename U>
inline constexpr bool is_propagate_const_v = is_propagate_const<U>::value;

template <typename T>
class propagate_const
{
    static constexpr bool is_raw_pointer = std::is_pointer_v<T>;

    T m_t;

public:
    using element_type = std::remove_reference_t<decltype(*std::declval<T&>())>;

    constexpr propagate_const() = default;
    constexpr propagate_const(propagate_const&&) = default;
    propagate_const(const propagate_const&) = delete;

    template <typename U>
    requires std::is_constructible_v<U, T>
    constexpr explicit(!std::is_convertible_v<U, T>) propagate_const(propagate_const<U>&& pu)
    : m_t(std::move(pu.m_t))
    {
    }

    template <typename U>
    requires (!is_propagate_const_v<std::decay_t<U>> && std::is_constructible_v<U, T>)
    constexpr explicit(!std::is_convertible_v<U, T>) propagate_const(U&& u)
    : m_t(std::forward<U>(u))
    {
    }

    constexpr propagate_const& operator=(propagate_const&&) = default;

    template <typename U>
    requires (std::is_convertible_v<U, T>)
    constexpr propagate_const& operator=(propagate_const<U>&& pu)
    {
        if (std::addressof(pu) != this) {
            m_t = std::move(pu.m_t);
        }
        return *this;
    }

    template <typename U>
    requires (!is_propagate_const_v<std::decay_t<U>> && std::is_convertible_v<U, T>)
    constexpr propagate_const& operator=(U&& u)
    {
        m_t = std::forward<U>(u);
        return *this;
    }

    propagate_const& operator=(const propagate_const&) = delete;

    constexpr void swap(propagate_const& pt) noexcept(std::is_nothrow_swappable_v<T>)
    {
        using std::swap; // Allow ADL
        swap(pt.m_t, this->m_t);
    }

    constexpr element_type* get()
    {
        if constexpr (is_raw_pointer) {
            return m_t;
        } else {
            return m_t.get();
        }
    }

    constexpr const element_type* get() const
    {
        if constexpr (is_raw_pointer) {
            return m_t;
        } else {
            return m_t.get();
        }
    }

    constexpr explicit operator bool() const
    {
        return static_cast<bool>(m_t);
    }

    constexpr element_type& operator*()
    {
        return *m_t;
    }

    constexpr const element_type& operator*() const
    {
        return *m_t;
    }

    constexpr element_type* operator->()
    {
        return get();
    }

    constexpr const element_type* operator->() const
    {
        return get();
    }

    constexpr operator element_type*()
    requires std::is_convertible_v<T, element_type*>
    {
        return get();
    }

    constexpr operator const element_type*() const
    requires std::is_convertible_v<T, element_type*>
    {
        return get();
    }

    auto operator<=>(const propagate_const&) const = default;
};