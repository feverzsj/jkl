#pragma once

#include <jkl/config.hpp>
#include <jkl/util/concepts.hpp>
#include <jkl/util/integral_constant.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <variant>


namespace jkl{


// jkl's tuple: can be _std_tuple_ or _array_, or anything support tuple_size and tuple_get

template<class T>
struct tuple_size;

template<class T> requires(requires{ std::tuple_size<T>::value; })
struct tuple_size<T> : std::tuple_size<T>{};

template<class T, size_t N>
struct tuple_size<T[N]>: std::integral_constant<size_t, N>{};

template<class T>
inline constexpr size_t tuple_size_v = tuple_size<T>::value;


template<size_t I, class T>
struct tuple_element;

template<size_t I, class T> requires(requires{ typename std::tuple_element<I, T>::type; })
struct tuple_element<I, T> : std::tuple_element<I, T>{};

template<size_t I, class T, size_t N>
struct tuple_element<I, T[N]>: std::type_identity<T>{ static_assert(I < N); };

template<size_t I, class T>
using tuple_element_t = typename tuple_element<I, T>::type;


template<size_t I>
constexpr auto tuple_get(_std_tuple_ auto&& t) noexcept(noexcept(std::get<I>(JKL_FORWARD(t))))
                                               -> decltype(std::get<I>(JKL_FORWARD(t)))
{
    return std::get<I>(JKL_FORWARD(t));
}

template<size_t I, class T, size_t N>
constexpr T& tuple_get(T (&t)[N]) noexcept
{
    static_assert(I < N);
    return t[I];
}

template<size_t I, class T, size_t N>
constexpr T&& tuple_get(T (&&t)[N]) noexcept
{
    static_assert(I < N);
    return std::move(t[I]);
}


template<class T> concept _tuple_ = requires(T& t){ tuple_size<std::remove_reference_t<T>>::value; tuple_get<0>(t); };


template<bool WithIdx = false, size_t... I>
constexpr decltype(auto) unpack(_tuple_ auto&& t, auto&& f, [[maybe_unused]] std::index_sequence<I...> indices)
{
    if constexpr(WithIdx)
        return std::invoke(JKL_FORWARD(f), indices, tuple_get<I>(JKL_FORWARD(t))...);
    else
        return std::invoke(JKL_FORWARD(f), tuple_get<I>(JKL_FORWARD(t))...);
}

template<bool WithIdx = false>
constexpr decltype(auto) unpack(_tuple_ auto&& t, auto&& f)
{
    return unpack<WithIdx>(JKL_FORWARD(t), JKL_FORWARD(f), std::make_index_sequence<tuple_size_v<JKL_DECL_NO_CVREF_T(t)>>{});
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr decltype(auto) unpack_with_idx(_tuple_ auto&& t, auto&& f)
{
    return unpack<true>(JKL_FORWARD(t), JKL_FORWARD(f));
}


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr decltype(auto) unpack_or_as_one(auto&& t, auto&& f)
{
    if constexpr(_tuple_<decltype(t)>)
        return unpack(JKL_FORWARD(t), JKL_FORWARD(f));
    else
        return JKL_FORWARD(f)(JKL_FORWARD(t));
}


template<size_t... I>
constexpr decltype(auto) tuple_foreach(_tuple_ auto&& t, auto&& f, std::index_sequence<I...>)
{
    return (... , JKL_FORWARD(f)(tuple_get<I>(JKL_FORWARD(t))));
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr decltype(auto) tuple_foreach(_tuple_ auto&& t, auto&& f)
{
    return tuple_foreach(JKL_FORWARD(t), JKL_FORWARD(f), std::make_index_sequence<tuple_size_v<JKL_DECL_NO_CVREF_T(t)>>{});
}


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr decltype(auto) tuple_foreach_or_as_one(auto&& t, auto&& f)
{
    if constexpr(_tuple_<decltype(t)>)
        return tuple_foreach(JKL_FORWARD(t), JKL_FORWARD(f));
    else
        return JKL_FORWARD(f)(JKL_FORWARD(t));
}


// iterate over tuple elements until f(e, I) returns something convertible to false

template<size_t... I>
constexpr auto __tuple_foreach_until(_tuple_ auto&& t, auto&& f, std::index_sequence<I...>) // NOTE: max(I) == tuple_size - 1
{
    auto r = JKL_FORWARD(f)(tuple_get<0>(JKL_FORWARD(t)), size_c<0>); // to allow non default constructible

    r && (... && (r = JKL_FORWARD(f)(tuple_get<I + 1>(JKL_FORWARD(t)), size_c<I + 1>)));
    
    return r;
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr auto tuple_foreach_until(_tuple_ auto&& t, auto&& f)
{
    return __tuple_foreach_until(JKL_FORWARD(t), JKL_FORWARD(f),
        std::make_index_sequence<tuple_size_v<JKL_DECL_NO_CVREF_T(t)> - 1>{}); // NOTE: tuple_size - 1
}


// these functions unifies _tuple_ and _range_.
template<size_t I, class U>
constexpr auto&& tr_get(U&& u) noexcept requires(_tuple_<U> || _range_<U>)
{
    if constexpr(_tuple_<U>)
        return tuple_get<0>(JKL_FORWARD(u));
    else if constexpr(std::is_lvalue_reference_v<U>)
        return *std::next(std::begin(u), I);
    else
        return std::move(*std::next(std::begin(u), I));
}

// NOTE: for _tuple_ return integral_constant
template<class U>
constexpr auto tr_size(U const& u) noexcept requires(_tuple_<U> || _range_<U>)
{
    if constexpr(_tuple_<U>)
        return tuple_size<JKL_DECL_NO_CVREF_T(u)>{};
    else
        return std::size(u);
}


template<size_t I = 0>
BOOST_FORCEINLINE decltype(auto) visit1(auto&& v, auto&& f)
{
    if constexpr(I == 0)
    {
        if(BOOST_UNLIKELY(v.valueless_by_exception()))
            throw std::bad_variant_access{};
    }

    constexpr auto vs = std::variant_size_v<JKL_DECL_NO_CVREF_T(v)>;

#define _VISIT_CASE_CNT 32
#define _VISIT_CASE(Z, N, D) \
        case I + N:                                                                                 \
        {                                                                                           \
            if constexpr(I + N < vs)                                                                \
            {                                                                                       \
                return JKL_FORWARD(f)(std::get<I + N>(JKL_FORWARD(v)), size_c<I + N>);              \
            }                                                                                       \
            else                                                                                    \
            {                                                                                       \
                BOOST_UNREACHABLE_RETURN(JKL_FORWARD(f)(std::get<0>(JKL_FORWARD(v)), size_c<0>));   \
            }                                                                                       \
        }                                                                                           \
        /**/

    switch(v.index())
    {
        BOOST_PP_REPEAT(
            _VISIT_CASE_CNT,
            _VISIT_CASE, _)
    }

    constexpr auto next_idx = std::min(I + _VISIT_CASE_CNT, vs);

    // if constexpr(next_idx < vs) causes some weird msvc bug
    if constexpr(next_idx + 0 < vs)
    {
        return visit1<next_idx>(JKL_FORWARD(v), JKL_FORWARD(f));
    }

    BOOST_UNREACHABLE_RETURN(JKL_FORWARD(f)(std::get<0>(JKL_FORWARD(v)), size_c<0>));

#undef _VISIT_CASE_CNT
#undef _VISIT_CASE
}


} // namespace jkl
