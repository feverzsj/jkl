#pragma once

#include <jkl/config.hpp>
#include <jkl/util/concepts.hpp>
#include <type_traits>


namespace jkl{


template<class T, T V>
using integral_constant = std::integral_constant<T, V>;


template<auto V, class T = decltype(V)>
inline constexpr auto integral_c = integral_constant<T, V>{};

template<size_t V>
inline constexpr auto size_c = integral_c<V>;

template<bool V>
inline constexpr auto bool_c  = integral_c<V>;
inline constexpr auto true_c  = bool_c<true>;
inline constexpr auto false_c = bool_c<false>;

template<class T     > struct is_integral_constant_helper                                       : std::false_type{};
template<class T, T N> struct is_integral_constant_helper<        std::integral_constant<T, N>> : std::true_type {};
// template<class T, T N> struct is_integral_constant_helper<boost::hana::integral_constant<T, N>> : std::true_type {};

template<class T> using                 is_integral_constant   = is_integral_constant_helper<std::remove_cv_t<T>>;
template<class T> inline constexpr bool is_integral_constant_v = is_integral_constant<T>::value;

template<class T> concept _integral_constant_ = is_integral_constant_v<std::remove_reference_t<T>>;


#define _JKL_DEF_INTEGRAL_CONSTANT_UNARY_OP(OP)    \
    template<class T, T V>                                        \
    constexpr auto operator OP (integral_constant<T, V>) noexcept \
    {                                                             \
        return integral_constant<decltype(OP V), OP V>{};         \
    }                                                             \
/**/

#define _JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP(OP)    \
    template<class TL, TL L, class TR, TR R>                                                 \
    constexpr auto operator OP (integral_constant<TL, L>, integral_constant<TR, R>) noexcept \
    {                                                                                        \
        return integral_constant<decltype(L OP R), (L OP R)>{};                              \
    }                                                                                        \
                                                                                             \
    template<class TL, TL L>                                                                 \
    constexpr auto operator OP (integral_constant<TL, L>, _arithmetic_ auto r) noexcept      \
    {                                                                                        \
        return L OP r;                                                                       \
    }                                                                                        \
                                                                                             \
    template<class TR, TR R>                                                                 \
    constexpr auto operator OP (_arithmetic_ auto l, integral_constant<TR, R>) noexcept      \
    {                                                                                        \
        return l OP R;                                                                       \
    }                                                                                        \
/**/


_JKL_DEF_INTEGRAL_CONSTANT_UNARY_OP(!)
_JKL_DEF_INTEGRAL_CONSTANT_UNARY_OP(~)

_JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP(==)
_JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP(!=)
_JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP(<)
_JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP(<=)
_JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP(>)
_JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP(>=)

_JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP(+)
_JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP(-)
_JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP(*)
_JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP(/)
_JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP(%)

_JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP(&&)
_JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP(||)

_JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP(&)
_JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP(|)
_JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP(^)
_JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP(<<)
_JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP(>>)


#undef _JKL_DEF_INTEGRAL_CONSTANT_UNARY_OP
#undef _JKL_DEF_INTEGRAL_CONSTANT_BINARY_OP


} // namespace jkl
