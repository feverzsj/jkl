#pragma once

#include <jkl/config.hpp>
#include <jkl/util/concepts.hpp>
#include <jkl/util/type_traits.hpp>
#include <boost/container/container_fwd.hpp>
#include <utility>


namespace jkl{


/// general

constexpr auto* buf_data(auto& b) noexcept requires(requires{ {b.data()} noexcept -> _pointer_; })
{
    return b.data();
}

constexpr size_t buf_size(auto const& b) noexcept requires(requires{ {b.size()} noexcept -> std::convertible_to<size_t>; })
{
    return b.size();
}

/// array
// for M(M > 1) dim array T a[N0]...[Nm-1], buf_data(a) returns (T[N1]...[Nm-1])*, buf_size(a) returns N0

template<class T, std::size_t N>
constexpr T* buf_data(T (&b)[N]) noexcept
{
    return b;
}

template<class T, size_t N>
constexpr size_t buf_size(T const (&)[N]) noexcept
{
    return N;
}

/// std::initializer_list
template<class T, size_t N>
constexpr T const* buf_data(std::initializer_list<T>& b) noexcept
{
    return b.begin();
}


/// buf_ssize
constexpr ptrdiff_t buf_ssize(auto const& b) noexcept
{
    return static_cast<ptrdiff_t>(buf_size(b));
};


template<class B>
concept _buf_ = requires(B& b){ buf_data(b); buf_size(b); };

template<class B>
concept _buf_class_ = _buf_<B> && _class_<B>;

template<_buf_ B>
using buf_value_t = std::remove_cvref_t<decltype(*buf_data(std::declval<B&>()))>;


template<class B>
concept _trivially_copyable_buf_ = _buf_<B> && _trivially_copyable_<buf_value_t<B>>;


// fun(_buf_of_<V> auto) first concept argument is left for type.
template<class B, class V>
concept _buf_of_ = _buf_<B> && std::same_as<V, buf_value_t<B>>;

template<class B>
concept _char_buf_ = _buf_of_<B, char>;

template<class B>
concept _byte_buf_ = _buf_<B> && _byte_<buf_value_t<B>>;

template<class T, class V>
concept _buf_of_or_same_as_ = _buf_of_<T, V> || std::same_as<T, V>;

template<class B, class B1>
concept _similar_buf_ = _buf_<B1> && _buf_<B> && std::same_as<buf_value_t<B>, buf_value_t<B1>>;

template<class T, class B1>
concept _similar_buf_or_val_ = _buf_<B1> && (std::same_as<T, buf_value_t<B1>>
                                             || (_buf_<T> && std::same_as<buf_value_t<T>, buf_value_t<B1>>));


constexpr size_t buf_byte_size(_buf_ auto const& b) noexcept
{
    return buf_size(b) * sizeof(*buf_data(b));
};


constexpr auto* buf_begin(_buf_ auto& b) noexcept { return buf_data(b); }
constexpr auto* buf_end  (_buf_ auto& b) noexcept { return buf_data(b) + buf_size(b); }
constexpr auto& buf_front(_buf_ auto& b) noexcept { return *buf_data(b); }
constexpr auto& buf_back (_buf_ auto& b) noexcept { BOOST_ASSERT(buf_size(b) > 0); return *(buf_end(b) - 1); }



constexpr void resize_buf(_buf_ auto& b, size_t n) noexcept(noexcept( b.resize(n)  ))
                                                   requires(requires{ b.resize(n); })
{
    if constexpr(requires{ b.resize(n, boost::container::default_init); })
        b.resize(n, boost::container::default_init);
    //else if constexpr(requires{ b.resize_default_init(n, [n](auto&&...){ return n; }); }) // p1072r5
    //    b.resize_default_init(n, [n](auto&&...){ return n; });
    else if constexpr(requires{ b.__resize_default_init(n); }) // std::basic_string of libc++ >= 8.0
        b.__resize_default_init(n);
    else
        b.resize(n);
}


template<class B> concept _resizable_buf_ = _buf_<B> && requires(B& b, size_t n){ resize_buf(b, n); };
template<class B, class V> concept _resizable_buf_of_ = _resizable_buf_<B> && std::same_as<V, buf_value_t<B>>;
template<class B> concept _resizable_char_buf_ = _resizable_buf_of_<B, char>;
template<class B> concept _resizable_byte_buf_ = _resizable_buf_<B> && _byte_<buf_value_t<B>>;
template<class B> concept _resizable_16bits_buf_ = _resizable_buf_<B> && _16bits_<buf_value_t<B>>;


constexpr void clear_buf(_resizable_buf_ auto& b) noexcept(noexcept(resize_buf(b, 0)))
{
    if constexpr(requires{ b.clear(); })
        b.clear();
    else
        resize_buf(b, 0);
}

// resize without preserving existing data
constexpr void rescale_buf(_resizable_buf_ auto& b, size_t n) noexcept(noexcept(resize_buf(b, n)))
{
    if constexpr(requires{ b.capacity(); })
    {
        if(n > static_cast<size_t>(b.capacity()))
            clear_buf(b);
    }

    resize_buf(b, n);
}


// "buy" a sub buffer of size 'n' from 'b' start at end of 'b',
// return the begin of sub buffer.
constexpr auto* buy_buf(_resizable_buf_ auto& b, size_t n) noexcept(noexcept(resize_buf(b, n)))
{
    size_t off = buf_size(b);
    resize_buf(b, off + n);
    return buf_data(b) + off;
}


} // namespace jkl
