#pragma once

#include <jkl/util/buf.hpp>
#include <jkl/util/assign_append.hpp>
#include <jkl/util/type_traits.hpp>
#include <rapidjson/writer.h>
#include <boost/container/string.hpp>
#include <boost/utility/string_view.hpp>
#include <string>
#include <cstdlib>
#include <charconv>


namespace jkl{


inline constexpr struct npos_t
{
    template<_unsigned_ T>
    consteval operator T() const noexcept
    {
        return static_cast<T>(-1);
    }
} npos;

constexpr bool operator==(_unsigned_ auto p, npos_t) noexcept { return p == static_cast<decltype(p)>(-1); }
constexpr bool operator!=(_unsigned_ auto p, npos_t) noexcept { return p != static_cast<decltype(p)>(-1); }

constexpr bool operator==(npos_t, _unsigned_ auto p) noexcept { return p == npos; }
constexpr bool operator!=(npos_t, _unsigned_ auto p) noexcept { return p != npos; }


template<_char_ C>
constexpr size_t tstrlen(C const* s) noexcept
{
    return std::char_traits<C>::length(s);
}


template<class S>
constexpr auto* str_data(S& s) noexcept requires(_buf_<S> && _char_<buf_value_t<S>>)
{
    return buf_data(s);
}

template<class S>
constexpr size_t str_size(S& s) noexcept requires(_buf_<S> && _char_<buf_value_t<S>>)
{
    return buf_size(s);
}

constexpr auto*  str_data(_char_ auto* s) noexcept { return s; }
constexpr size_t str_size(_char_ auto* s) noexcept { return tstrlen(s); }


constexpr ptrdiff_t str_ssize(auto const& s) noexcept requires(requires{ str_size(s); })
{
    return static_cast<ptrdiff_t>(str_size(s));
}


template<class S>     // v: no need use S&, decay array to pointer is fine for str.
concept _str_ = requires(S s){ str_data(s); str_size(s); };

template<class S>
concept _str_class_ = _str_<S> && _class_<S>;

template<class S>
concept _resizable_str_ = _str_<S> && _resizable_buf_<S>;

template<_str_ S>
using str_value_t = std::remove_cvref_t<decltype(*str_data(std::declval<S&>()))>;

template<class S, class C>
concept _str_of_ = _str_<S> && std::same_as<C, str_value_t<S>>;

template<class S>
concept _char_str_ = _str_of_<S, char>;

template<class S>
concept _byte_str_ = _str_<S> && _byte_<str_value_t<S>>;

template<class S>
concept _u8_str_ = _str_of_<S, char8_t>;

template<class S>
concept _u16_str_ = _str_of_<S, char16_t>;

template<class S>
concept _u32_str_ = _str_of_<S, char32_t>;

template<class T, class C>
concept _str_of_or_same_as_ = _str_of_<T, C> || std::same_as<T, C>;

template<class S, class S1>
concept _similar_str_ = _str_<S1> && _str_<S> && std::same_as<str_value_t<S>, str_value_t<S1>>;

template<class T, class S1>
concept _similar_str_or_val_ = _str_<S1> && (std::same_as<T, str_value_t<S1>>
                                             || (_str_<T> && std::same_as<str_value_t<T>, str_value_t<S1>>));


constexpr size_t str_byte_size(_str_ auto const& s) noexcept
{
    return str_size(s) * sizeof(*str_data(s));
}

constexpr auto* str_begin(_str_ auto& s) noexcept { return str_data(s); }
constexpr auto* str_end  (_str_ auto& s) noexcept { return str_data(s) + str_size(s); }
constexpr auto& str_front(_str_ auto& s) noexcept { return *str_data(s); }
constexpr auto& str_back (_str_ auto& s) noexcept { BOOST_ASSERT(str_size(s) > 0); return *(str_end(s) - 1); }


constexpr auto* cstr_data(auto& s) noexcept requires(requires{ {s.c_str()} noexcept -> std::same_as<decltype(str_data(s))>; })
{
    return s.c_str();
}

constexpr auto* cstr_data(_char_ auto* s) noexcept
{
    return s;
}


template<class S>
concept _cstr_ = _str_<S> && requires(S s){ cstr_data(s); };


/// basic_string_view

template<class C, class Tx = std::char_traits<C>>
class basic_string_view : public std::basic_string_view<C, Tx>
{
    using base = std::basic_string_view<C, Tx>;

public:
    using size_type = typename base::size_type;

    using base::base;
    using base::operator=;

    constexpr basic_string_view(_str_of_<C> auto const& s) noexcept
        : base{str_data(s), str_size(s)}
    {}

    constexpr basic_string_view(C const* b, C const* e) noexcept
        : base(b, e - b)
    {
        BOOST_ASSERT(b <= e);
    }

    constexpr basic_string_view substr(size_type pos = 0, size_type count = base::npos) const
    {
        return subview(pos, count);
    }

    // just be consist with basic_string::subview().
    constexpr basic_string_view subview(size_type pos = 0, size_type count = base::npos) const
    {
        if(pos > base::size())
            throw std::out_of_range{"invalid pos"};
        return {base::data() + pos, std::min(count, base::size() - pos)};
    }

    template<_arithmetic_ T>
    constexpr std::optional<T> to(int integerBase = 10) const noexcept
    {
        return to_num<T>(*this, integerBase);
    }

    friend constexpr bool operator==(basic_string_view const& l, basic_string_view const& r) noexcept { return static_cast<base const&>(l) == static_cast<base const&>(r); }
    friend constexpr bool operator!=(basic_string_view const& l, basic_string_view const& r) noexcept { return static_cast<base const&>(l) != static_cast<base const&>(r); }
    friend constexpr bool operator< (basic_string_view const& l, basic_string_view const& r) noexcept { return static_cast<base const&>(l) <  static_cast<base const&>(r); }
    friend constexpr bool operator> (basic_string_view const& l, basic_string_view const& r) noexcept { return static_cast<base const&>(l) >  static_cast<base const&>(r); }
    friend constexpr bool operator<=(basic_string_view const& l, basic_string_view const& r) noexcept { return static_cast<base const&>(l) <= static_cast<base const&>(r); }
    friend constexpr bool operator>=(basic_string_view const& l, basic_string_view const& r) noexcept { return static_cast<base const&>(l) >= static_cast<base const&>(r); }
};

using string_view    = basic_string_view<char>;
using wstring_view   = basic_string_view<wchar_t>;
using u8string_view  = basic_string_view<char8_t>;
using u16string_view = basic_string_view<char16_t>;
using u32string_view = basic_string_view<char32_t>;



/// basic_string

template<class C ,class Tx = std::char_traits<C> ,class Ax = void>
class basic_string : public boost::container::basic_string<C, Tx, void>
{
    using base = boost::container::basic_string<C, Tx, void>;

public:
    using size_type = typename base::size_type;

    using base::base;
    using base::assign;
    using base::append;
    using base::operator=;
    using base::operator+=;

    constexpr basic_string(_str_of_<C> auto const& s)
        : base(str_data(s), str_size(s))
    {}

    constexpr basic_string& assign(_str_of_<C> auto const& s) { return base::assign(str_data(s), str_size(s)); }
    constexpr basic_string& append(_str_of_<C> auto const& s) { return base::append(str_data(s), str_size(s)); }

    constexpr basic_string& operator= (_str_of_<C> auto const& s) { return assign(s); }
    constexpr basic_string& operator+=(_str_of_<C> auto const& s) { return append(s); }

    constexpr basic_string substr(size_type pos = 0, size_type count = base::npos) const
    {
        return subview(pos, count);
    }

    constexpr basic_string_view<C, Tx> subview(size_type pos = 0, size_type count = base::npos) const
    {
        if(pos > base::size())
            throw std::out_of_range{"invalid pos"};
        return {base::data() + pos, std::min(count, base::size() - pos)};
    }

    template<_arithmetic_ T>
    constexpr std::optional<T> to(int integralBase = 10) const noexcept
    {
        return to_num<T>(*this, integralBase);
    }

    bool starts_with(auto&& s) const noexcept
    {
        return string_view{*this}.starts_with(JKL_FORWARD(s));
    }

    bool ends_with(auto&& s) const noexcept
    {
        return string_view{*this}.ends_with(JKL_FORWARD(s));
    }

    friend constexpr bool operator==(basic_string const& l, basic_string const& r) noexcept { return static_cast<base const&>(l) == static_cast<base const&>(r); }
    friend constexpr bool operator!=(basic_string const& l, basic_string const& r) noexcept { return static_cast<base const&>(l) != static_cast<base const&>(r); }
    friend constexpr bool operator< (basic_string const& l, basic_string const& r) noexcept { return static_cast<base const&>(l) <  static_cast<base const&>(r); }
    friend constexpr bool operator> (basic_string const& l, basic_string const& r) noexcept { return static_cast<base const&>(l) >  static_cast<base const&>(r); }
    friend constexpr bool operator<=(basic_string const& l, basic_string const& r) noexcept { return static_cast<base const&>(l) <= static_cast<base const&>(r); }
    friend constexpr bool operator>=(basic_string const& l, basic_string const& r) noexcept { return static_cast<base const&>(l) >= static_cast<base const&>(r); }

    friend constexpr bool operator==(basic_string const& l, C const* r) noexcept { return static_cast<base const&>(l) == r; }
    friend constexpr bool operator!=(basic_string const& l, C const* r) noexcept { return static_cast<base const&>(l) != r; }
    friend constexpr bool operator< (basic_string const& l, C const* r) noexcept { return static_cast<base const&>(l) <  r; }
    friend constexpr bool operator> (basic_string const& l, C const* r) noexcept { return static_cast<base const&>(l) >  r; }
    friend constexpr bool operator<=(basic_string const& l, C const* r) noexcept { return static_cast<base const&>(l) <= r; }
    friend constexpr bool operator>=(basic_string const& l, C const* r) noexcept { return static_cast<base const&>(l) >= r; }

    friend constexpr bool operator==(C const* l, basic_string const& r) noexcept { return l == static_cast<base const&>(r); }
    friend constexpr bool operator!=(C const* l, basic_string const& r) noexcept { return l != static_cast<base const&>(r); }
    friend constexpr bool operator< (C const* l, basic_string const& r) noexcept { return l <  static_cast<base const&>(r); }
    friend constexpr bool operator> (C const* l, basic_string const& r) noexcept { return l >  static_cast<base const&>(r); }
    friend constexpr bool operator<=(C const* l, basic_string const& r) noexcept { return l <= static_cast<base const&>(r); }
    friend constexpr bool operator>=(C const* l, basic_string const& r) noexcept { return l >= static_cast<base const&>(r); }
};

using string    = basic_string<char>;
using wstring   = basic_string<wchar_t>;
using u8string  = basic_string<char8_t>;
using u16string = basic_string<char16_t>;
using u32string = basic_string<char32_t>;


// USAGE:
//   fun_require_cstr(as_cstr(s).data() or cstr_data(as_cstr(s)));
//   or
//   auto&& cstr = as_cstr(s); then get data/size

constexpr auto as_cstr(_cstr_ auto const& s) noexcept
{
    struct wrapper
    {
        decltype(s) _s;
        auto c_str() const { return cstr_data(_s); }
        auto data () const { return cstr_data(_s); }
        auto size () const { return str_size (_s); }
    };

    return wrapper{s};
}

auto as_cstr(_str_ auto const& s)
{
    return basic_string<str_value_t<decltype(s)>>(str_data(s), str_size(s));
}

template<class C>
constexpr auto   as_str_class(C const*           s) noexcept { return basic_string_view<C>{s}; }
constexpr auto&& as_str_class(_str_class_ auto&& s) noexcept { return JKL_FORWARD(s); }


constexpr decltype(auto) as_str_class_or_as_is(_str_ auto&& s) noexcept { return as_str_class(JKL_FORWARD(s)); }
constexpr decltype(auto) as_str_class_or_as_is(      auto&& s) noexcept { return JKL_FORWARD(s); }


template<_str_ S> constexpr          auto&& as_str_class_or_val(  str_value_t<S>     && v) noexcept { return              JKL_FORWARD(v) ; }
template<_str_ S> constexpr decltype(auto)  as_str_class_or_val(_similar_str_<S> auto&& s) noexcept { return as_str_class(JKL_FORWARD(s)); }



/// append_str, assign_str, cat_str
// append/assign str or character to ptr or buf, cat str or character to a buf

decltype(auto) append_str(auto& d, auto&&... svs)
{
    return append_rv(d, as_str_class_or_as_is(JKL_FORWARD(svs))...);
}

void assign_str(auto& d, auto&&... svs)
{
    assign_rv(d, as_str_class_or_as_is(JKL_FORWARD(svs))...);
}

template<class D = string>
D cat_str(auto&&... svs)
{
    D d;
    append_str(d, svs...);
    return d;
}


} // namespace jkl
