#pragma once

#include <jkl/util/buf.hpp>
#include <jkl/util/assign_append.hpp>
#include <jkl/util/type_traits.hpp>
#include <rapidjson/writer.h>
#include <boost/container/string.hpp>
#include <boost/utility/string_view.hpp>
#include <string>
#include <compare>
#include <charconv>
#include <cstdlib>


namespace jkl{


inline constexpr struct npos_t
{
    template<_unsigned_ T>
    consteval operator T() const noexcept
    {
        return static_cast<T>(-1);
    }
} npos;

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr bool operator==(_unsigned_ auto p, npos_t) noexcept { return p == static_cast<decltype(p)>(-1); }
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr bool operator!=(_unsigned_ auto p, npos_t) noexcept { return p != static_cast<decltype(p)>(-1); }

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr bool operator==(npos_t, _unsigned_ auto p) noexcept { return p == npos; }
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
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

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr auto*  str_data(_char_ auto* s) noexcept { return s; }
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr size_t str_size(_char_ auto* s) noexcept { return tstrlen(s); }


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
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
using str_value_t = JKL_DECL_NO_CVREF_T(*str_data(std::declval<S&>()));

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


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr size_t str_byte_size(_str_ auto const& s) noexcept
{
    return str_size(s) * sizeof(*str_data(s));
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr auto* str_begin(_str_ auto& s) noexcept { return str_data(s); }
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr auto* str_end  (_str_ auto& s) noexcept { return str_data(s) + str_size(s); }
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr auto& str_front(_str_ auto& s) noexcept { return *str_data(s); }
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr auto& str_back (_str_ auto& s) noexcept { BOOST_ASSERT(str_size(s) > 0); return *(str_end(s) - 1); }


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr auto* cstr_data(auto& s) noexcept requires(requires{ {s.c_str()} noexcept -> std::same_as<decltype(str_data(s))>; })
{
    return s.c_str();
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr auto* cstr_data(_char_ auto* s) noexcept
{
    return s;
}


template<class S>
concept _cstr_ = _str_<S> && requires(S s){ cstr_data(s); };

// MSVC_WORKAROUND
template<class L, class R>
concept __preferred_char_traits_helper_cond1 = requires{ typename L::traits_type; };
template<class L, class R>
concept __preferred_char_traits_helper_cond2 = requires{ typename R::traits_type; };

// prefer L::traits_type, then R::traits_type, otherwise std::char_traits
template<class L, class R> requires(_similar_str_<L, R>)
constexpr auto&& preferred_char_traits_helper()
{
    // if constexpr(requires{ typename L::traits_type; })
    if constexpr(__preferred_char_traits_helper_cond1<L, R>)
        return declval<typename L::traits_type>();
    //else if constexpr(requires{ typename R::traits_type; })
    if constexpr(__preferred_char_traits_helper_cond2<L, R>)
        return declval<typename R::traits_type>();
    else
        return declval<std::char_traits<str_value_t<L>>>();
}

template<class L, class R> requires(_similar_str_<L, R>)
using preferred_char_traits = JKL_DECL_NO_CVREF_T((preferred_char_traits_helper<L, R>()));


template<class L, class R> requires(_similar_str_<L, R>)
constexpr int str_compare(L const& l, R const& r) noexcept
{
    size_t nl = str_size(l);
    size_t nr = str_size(r);

    int t = preferred_char_traits<L, R>::compare(str_data(l), str_data(r), std::min(nl, nr));

    if(t !=  0) return  t;
    if(nl < nr) return -1;
    if(nl > nr) return  1;
    return 0;
}

template<class L, class R> requires(_similar_str_<L, R>)
constexpr bool str_equal(L const& l, R const& r) noexcept
{
    size_t nl = str_size(l);
    size_t nr = str_size(r);
    return nl == nr && 0 == preferred_char_traits<L, R>::compare(str_data(l), str_data(r), nl);
}

template<class L, class R> requires(_similar_str_<L, R>)
constexpr bool operator == (L const& l, R const& r) noexcept
{
    return str_equal(l, r);
}

template<class L, class R> requires(_similar_str_<L, R>)
constexpr bool operator != (L const& l, R const& r) noexcept
{
    return !(l == r);
}

template<class L, class R> requires(_similar_str_<L, R>)
constexpr bool operator < (L const& l, R const& r) noexcept
{
    return str_compare(l, r) < 0;
}

template<class L, class R> requires(_similar_str_<L, R>)
constexpr bool operator <= (L const& l, R const& r) noexcept
{
    return str_compare(l, r) <= 0;
}

template<class L, class R> requires(_similar_str_<L, R>)
constexpr bool operator > (L const& l, R const& r) noexcept
{
    return str_compare(l, r) > 0;
}

template<class L, class R> requires(_similar_str_<L, R>)
constexpr bool operator >= (L const& l, R const& r) noexcept
{
    return str_compare(l, r) >= 0;
}

template<class L, class R> requires(_similar_str_<L, R>)
constexpr std::strong_ordering operator <=> (L const& l, R const& r) noexcept
{
    auto const t = str_compare(l, r);
    if(t <  0) return std::strong_ordering::less;
    if(t == 0) return std::strong_ordering::equal;
    return std::strong_ordering::greater;
}


/// basic_string_view

template<class C, class Tx = std::char_traits<C>>
class basic_string_view : public std::basic_string_view<C, Tx>
{
    using base = std::basic_string_view<C, Tx>;

public:
    using size_type = typename base::size_type;

    using base::base;
    //using base::operator=; ambiguous overload

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr basic_string_view(_str_of_<C> auto const& s) noexcept
        : base{str_data(s), str_size(s)}
    {}

    constexpr basic_string_view(C const* b, C const* e) noexcept
        : base{b, e - b}
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
};

using string_view    = basic_string_view<char>;
using wstring_view   = basic_string_view<wchar_t>;
using u8string_view  = basic_string_view<char8_t>;
using u16string_view = basic_string_view<char16_t>;
using u32string_view = basic_string_view<char32_t>;



/// basic_string

template<class C, class Tx = std::char_traits<C>, class Ax = std::allocator<C>>
class basic_string : public std::basic_string<C, Tx, Ax>
{
    using base = std::basic_string<C, Tx, Ax>;

public:
    using size_type = typename base::size_type;

    using base::base;
    using base::assign;
    using base::append;
    using base::operator=;
    using base::operator+=;

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr basic_string(_str_of_<C> auto const& s)
        : base(str_data(s), str_size(s))
    {}

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr basic_string& assign(_str_of_<C> auto const& s) { return static_cast<basic_string&>(base::assign(str_data(s), str_size(s))); }
    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr basic_string& append(_str_of_<C> auto const& s) { return static_cast<basic_string&>(base::append(str_data(s), str_size(s))); }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr basic_string& operator= (_str_of_<C> auto const& s) { return assign(s); }
    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
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

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    bool starts_with(auto&& s) const noexcept
    {
        return string_view{*this}.starts_with(JKL_FORWARD(s));
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    bool ends_with(auto&& s) const noexcept
    {
        return string_view{*this}.ends_with(JKL_FORWARD(s));
    }
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

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
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

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
auto as_cstr(_str_ auto const& s)
{
    return basic_string<str_value_t<decltype(s)>>(str_data(s), str_size(s));
}

template<class C>
constexpr auto   as_str_class(C const*           s) noexcept { return basic_string_view<C>{s}; }
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr auto&& as_str_class(_str_class_ auto&& s) noexcept { return JKL_FORWARD(s); }


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr decltype(auto) as_str_class_or_as_is(_str_ auto&& s) noexcept { return as_str_class(JKL_FORWARD(s)); }
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr decltype(auto) as_str_class_or_as_is(      auto&& s) noexcept { return JKL_FORWARD(s); }


template<_str_ S> constexpr          auto&& as_str_class_or_val(  str_value_t<S>     && v) noexcept { return              JKL_FORWARD(v) ; }
template<_str_ S> constexpr decltype(auto)  as_str_class_or_val(_similar_str_<S> auto&& s) noexcept { return as_str_class(JKL_FORWARD(s)); }



/// append_str, assign_str, cat_str
// append/assign str or character to ptr or buf, cat str or character to a buf

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
decltype(auto) append_str(auto& d, auto&&... svs)
{
    return append_rv(d, as_str_class_or_as_is(JKL_FORWARD(svs))...);
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
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


namespace std{


template<class C, class Tx>
struct hash<jkl::basic_string_view<C, Tx>>
{
    size_t operator()(jkl::basic_string_view<C, Tx> const& s) const noexcept
    {
        return hash<basic_string_view<C, Tx>>{}(s);
    }
};

template<class C, class Tx, class Ax>
struct hash<jkl::basic_string<C, Tx, Ax>>
{
    size_t operator()(jkl::basic_string<C, Tx, Ax> const& s) const noexcept
    {
        return hash<basic_string<C, Tx, Ax>>{}(s);
    }
};


} // namespace std
