#pragma once

#include <jkl/config.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/numeric/conversion/is_subranged.hpp>
#include <array>
#include <memory>
#include <optional>
#include <type_traits>


namespace jkl{


// In Most Cases:
//   for type traits, apply std::remove_cv_t on the tested type;
//   for concepts, apply std::remove_cvref_t on the tested type;


// seems libc++ disables using std::declval in unevaluated context
template<class T>
std::add_rvalue_reference_t<T> declval() noexcept;


template<class T>
using is_null_op = std::is_same<T, null_op_t>;

template<class T>
static constexpr bool is_null_op_v = is_null_op<T>::value;


template<bool Cond, class T>
using null_if_t = std::conditional_t<Cond, null_op_t, T>;

template<bool Cond, class T>
using not_null_if_t = null_if_t<! Cond, T>;


// NOTE: can only be used in class or namespace scope (unless lambda is allowed
// in unevaluated context, so that we can get rid of the static helper function).
//
// all types are lazily evaluated, so something like
// JKL_LAZY_COND_TYPEDEF(type, false, typename T::something_not_exit, void) also works
#define JKL_LAZY_COND_TYPEDEF(Name, Cond, Then, Else)                               \
    static decltype(auto) _jkl_cond_typedef_##Name##_helper()                       \
    {                                                                               \
        if constexpr(Cond)                                                          \
        {                                                                           \
            if constexpr(! std::is_void_v<Then>)                                    \
                return ::jkl::declval<Then>();                                      \
        }                                                                           \
        else                                                                        \
        {                                                                           \
            if constexpr(! std::is_void_v<Else>)                                    \
                return ::jkl::declval<Else>();                                      \
        }                                                                           \
    }                                                                               \
    using Name = std::remove_cvref_t<decltype(_jkl_cond_typedef_##Name##_helper())> \
/**/

#define JKL_LAZY_DEF_MEMBER_IF(Cond, Type, Name)                                       \
    JKL_LAZY_COND_TYPEDEF(jkl_add_member_if_##Name##_t, Cond, Type, ::jkl::null_op_t); \
    [[no_unique_address]] jkl_add_member_if_##Name##_t Name                            \
/**/

#define JKL_DEF_MEMBER_IF(Cond, Type, Name)                                     \
    [[no_unique_address]] std::conditional_t<Cond, Type, ::jkl::null_op_t> Name \
/**/



template<class T          > struct is_std_array_helper                   : std::false_type{};
template<class T, size_t N> struct is_std_array_helper<std::array<T, N>> : std::true_type {};

template<class T> using                 is_std_array   = is_std_array_helper<std::remove_cv_t<T>>;
template<class T> inline constexpr bool is_std_array_v = is_std_array<T>::value;


template<class T         > struct is_unique_ptr_helper                        : std::false_type{};
template<class T, class D> struct is_unique_ptr_helper<std::unique_ptr<T, D>> : std::true_type {};

template<class T> using                 is_unique_ptr   = is_unique_ptr_helper<std::remove_cv_t<T>>;
template<class T> inline constexpr bool is_unique_ptr_v = is_unique_ptr<T>::value;


template<class T> struct is_shared_ptr_helper                     : std::false_type{};
template<class T> struct is_shared_ptr_helper<std::shared_ptr<T>> : std::true_type {};

template<class T> using                 is_shared_ptr   = is_shared_ptr_helper<std::remove_cv_t<T>>;
template<class T> inline constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

template<class T> inline constexpr bool is_smart_ptr_v = is_unique_ptr_v<T> || is_shared_ptr_v<T>;


template<class T> struct is_optional_helper                   : std::false_type{};
template<class T> struct is_optional_helper<std::optional<T>> : std::true_type {};

template<class T> using                 is_optional   = is_optional_helper<std::remove_cv_t<T>>;
template<class T> inline constexpr bool is_optional_v = is_optional<T>::value;


// NOTE: like std::is_same, cv qualicifiers are still taken into account.
template<class U, class T, class... Ts>
using is_one_of = std::bool_constant<(std::is_same_v<U, T> || ... || std::is_same_v<U, Ts>)>;

template<class U, class T, class... Ts>
inline constexpr bool is_one_of_v = is_one_of<U, T, Ts...>::value;


template<class T> std::unwrap_reference_t<T>& unwrap_ref(T& t) noexcept { return t; }


template<bool LvalueRefToRefWrapper, class T>
[[nodiscard]] constexpr decltype(auto) _decay(T&& t)
{
    using DT = std::decay_t<T>;

    if constexpr(std::is_pointer_v<DT> || std::is_member_pointer_v<DT>)
    {
        return static_cast<DT>(t);
    }
    else if constexpr(LvalueRefToRefWrapper && std::is_lvalue_reference_v<T>)
    {
        return std::ref(t);
    }
    else
    {
        return static_cast<T&&>(t);
    }
}

// we write 2 functions for each below to avoid user writing decay_fwd(t), which make t always lvalue references
// https://stackoverflow.com/questions/27501400/the-implementation-of-stdforward

// pointer/function/pointer-to-member/array ref => pointer
// if LvalueRefToRefWrapper, lvalue ref => std::reference_wrapper
// perfect forward others.
template<class T, bool LvalueRefToRefWrapper = false>
[[nodiscard]] constexpr decltype(auto) decay_fwd(std::remove_reference_t<T>& t) noexcept
{
    return _decay<LvalueRefToRefWrapper>(static_cast<T&&>(t));
}

template<class T, bool LvalueRefToRefWrapper = false>
[[nodiscard]] constexpr decltype(auto) decay_fwd(std::remove_reference_t<T>&& t) noexcept
{
    static_assert(! std::is_lvalue_reference_v<T>);
    return _decay<LvalueRefToRefWrapper>(static_cast<T&&>(t));
}


// pointer/function/pointer-to-member/array ref => pointer
// lvalue ref => std::reference_wrapper
// perfect forward others.
// 
// template<class T>
// void foo(T&& t)
// {
//     // this avoids dangling ref, but if ct is reference_wrapper, it has to be implicitly or explicitly converted to actual ref.
//     [ct = decay_capture<T>(t)]()
//     {
//         ct(...);       // reference_wrapper ok
//         // ct.t_method(); // reference_wrapper fail.
//         auto& t = unwrap_ref(ct); // or if you know the expected_type& t = ct;
//         t.t_method(); // ok
//     }
// }
template<class T>
[[nodiscard]] constexpr decltype(auto) decay_capture(std::remove_reference_t<T>& t) noexcept
{
    return _decay<true>(static_cast<T&&>(t));
}

template<class T>
[[nodiscard]] constexpr decltype(auto) decay_capture(std::remove_reference_t<T>&& t) noexcept
{
    static_assert(! std::is_lvalue_reference_v<T>);
    return _decay<true>(static_cast<T&&>(t));
}


template<class T>
using lref_or_val_t = std::conditional_t<std::is_lvalue_reference_v<T>, T, std::remove_cvref_t<T>>;


template<class A, class B>
using is_lossless_convertible = std::disjunction<std::is_same<A, B>, boost::numeric::is_subranged<A, B>>;
template<class A, class B>
inline constexpr bool is_lossless_convertible_v = is_lossless_convertible<A, B>::value;

template<class From, class To>
constexpr bool cvt_num(From from, To& to)
{
    if constexpr(is_lossless_convertible_v<From, To>)
    {
        to = from;
        return true;
    }
    else
    {
        try{
            to = boost::numeric_cast<To>(from);
            return true;
        }
        catch(boost::bad_numeric_cast const&)
        {
            return false;
        }
    }
}


struct nonesuch
{
    ~nonesuch() = delete;
    nonesuch(nonesuch const&) = delete;
    void operator=(nonesuch const&) = delete;
};


namespace detail{

template<class Default, class AlwaysVoid, template<class...> class Op, class... Args>
struct detector
{
    using value_t = std::false_type;
    using type = Default;
};

template<class Default, template<class...> class Op, class... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...>
{
    using value_t = std::true_type;
    using type = Op<Args...>;
};

} // namespace detail


template<template<class...> class Op, class... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

template<template<class...> class Op, class... Args>
using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

template<class Default, template<class...> class Op, class... Args>
using detected_or = detail::detector<Default, void, Op, Args...>;


template<template<class...> class Op, class... Args>
inline constexpr bool is_detected_v = is_detected<Op, Args...>::value;

template<class Default, template<class...> class Op, class... Args>
using detected_or_t = typename detected_or<Default, Op, Args...>::type;

template<class Expected, template<class...> class Op, class... Args>
using is_detected_exact = std::is_same<Expected, detected_t<Op, Args...>>;

template<class Expected, template<class...> class Op, class... Args>
inline constexpr bool is_detected_exact_v = is_detected_exact<Expected, Op, Args...>::value;

template<class To, template<class...> class Op, class... Args>
using is_detected_convertible = std::is_convertible<detected_t<Op, Args...>, To>;

template<class To, template<class...> class Op, class... Args>
inline constexpr bool is_detected_convertible_v = is_detected_convertible<To, Op, Args...>::value;


namespace detail{

template<class T, class U> struct copy_cv{ using type = U; };
template<class T, class U> struct copy_cv<const          T, U>{ using type = std::add_const_t   <U>; };
template<class T, class U> struct copy_cv<      volatile T, U>{ using type = std::add_volatile_t<U>; };
template<class T, class U> struct copy_cv<const volatile T, U>{ using type = std::add_cv_t      <U>; };

template<class T, class U> using copy_cv_t = typename copy_cv<T, U>::type;

template<class T> using cref_t = std::add_lvalue_reference_t<const std::remove_reference_t<T>>;


// Workaround for "term does not evaluate to a function taking 0 arguments"
// error in MSVC 19.22 (issue #75)
#if defined(_MSC_VER) && _MSC_VER >= 1922

    template<class, class, class = void> struct cond_res {};

    template<class T, class U>
    struct cond_res<T, U, std::void_t<decltype(false ? std::declval<T (&)()>()()
                                                     : std::declval<U (&)()>()())>>
    {
        using type = decltype(false ? std::declval<T (&)()>()()
                                    : std::declval<U (&)()>()());
    };

    template<class T, class U>
    using cond_res_t = typename cond_res<T, U>::type;

#else

    template<class T, class U>
    using cond_res_t = decltype(false ? std::declval<T (&)()>()()
                                      : std::declval<U (&)()>()());

#endif


// For some value of "simple"
template<class A, class B,
         class X = std::remove_reference_t<A>,
         class Y = std::remove_reference_t<B>,
         class = void>
struct common_ref{};

template<class A, class B>
using common_ref_t = typename common_ref<A, B>::type;

template<class A, class B,
          class X = std::remove_reference_t<A>,
          class Y = std::remove_reference_t<B>,
          class = void>
struct lval_common_ref{};

template<class A, class B, class X, class Y>
struct lval_common_ref<A, B, X, Y, std::enable_if_t<
    std::is_reference_v<cond_res_t<copy_cv_t<X, Y>&, copy_cv_t<Y, X>&>>>>
{
    using type = cond_res_t<copy_cv_t<X, Y>&, copy_cv_t<Y, X>&>;
};

template<class A, class B>
using lval_common_ref_t = typename lval_common_ref<A, B>::type;

template<class A, class B, class X, class Y>
struct common_ref<A&, B&, X, Y> : lval_common_ref<A&, B&>{};

template<class X, class Y>
using rref_cr_helper_t = std::remove_reference_t<lval_common_ref_t<X&, Y&>>&&;

template<class A, class B, class X, class Y>
struct common_ref<A&&, B&&, X, Y, std::enable_if_t<
    std::is_convertible_v<A&&, rref_cr_helper_t<X, Y>> &&
    std::is_convertible_v<B&&, rref_cr_helper_t<X, Y>>>>
{
    using type = rref_cr_helper_t<X, Y>;
};

template<class A, class B, class X, class Y>
struct common_ref<A&&, B&, X, Y, std::enable_if_t<
    std::is_convertible_v<A&&, lval_common_ref_t<const X&, Y&>>>>
{
    using type = lval_common_ref_t<const X&, Y&>;
};

template<class A, class B, class X, class Y>
struct common_ref<A&, B&&, X, Y>
    : common_ref<B&&, A&>
{};

template<class>
struct xref{ template<class U> using type = U; };

template<class A>
struct xref<A&>
{
    template<class U>
    using type = std::add_lvalue_reference_t<typename xref<A>::template type<U>>;
};

template<class A>
struct xref<A&&>
{
    template<class U>
    using type = std::add_rvalue_reference_t<typename xref<A>::template type<U>>;
};

template<class A>
struct xref<const A>
{
    template<class U>
    using type = std::add_const_t<typename xref<A>::template type<U>>;
};

template<class A>
struct xref<volatile A>
{
    template<class U>
    using type = std::add_volatile_t<typename xref<A>::template type<U>>;
};

template<class A>
struct xref<const volatile A>
{
    template<class U>
    using type = std::add_cv_t<typename xref<A>::template type<U>>;
};

} // namespace detail

template<class T, class U, template<class> class TQual,
         template<class> class UQual>
struct basic_common_reference{};

template<class...>
struct common_reference;

template<class... Ts>
using common_reference_t = typename common_reference<Ts...>::type;

template<> struct common_reference<>{};
template<typename T0>struct common_reference<T0>{ using type = T0; };


namespace detail{

template<class T, class U>
inline constexpr bool has_common_ref_v = is_detected_v<common_ref_t, T, U>;

template<class T, class U>
using basic_common_ref_t =
    typename basic_common_reference<std::remove_cvref_t<T>, std::remove_cvref_t<U>,
                                    detail::xref<T>::template type, detail::xref<U>::template type>::type;

template<class T, class U>
inline constexpr bool has_basic_common_ref_v = is_detected_v<basic_common_ref_t, T, U>;

template<class T, class U>
inline constexpr bool has_cond_res_v = is_detected_v<cond_res_t, T, U>;

template<class T, class U, class = void>
struct binary_common_ref
    : std::common_type<T, U>
{};

template<class T, class U>
struct binary_common_ref<T, U, std::enable_if_t<has_common_ref_v<T, U>>>
    : common_ref<T, U>
{};

template<class T, class U>
struct binary_common_ref<T, U,
                         std::enable_if_t<has_basic_common_ref_v<T, U> &&
                                          !has_common_ref_v<T, U>>>
{
    using type = basic_common_ref_t<T, U>;
};

template<class T, class U>
struct binary_common_ref<T, U,
                         std::enable_if_t<has_cond_res_v<T, U> &&
                                          !has_basic_common_ref_v<T, U> &&
                                          !has_common_ref_v<T, U>>>
{
    using type = cond_res_t<T, U>;
};

} // namespace detail


template<class T1, class T2>
struct common_reference<T1, T2> : detail::binary_common_ref<T1, T2>{
};


namespace detail{

template<class Void, class T1, class T2, class... Rest>
struct multiple_common_reference
{};

template<class T1, class T2, class... Rest>
struct multiple_common_reference<std::void_t<common_reference_t<T1, T2>>, T1, T2,
                                 Rest...>
    : common_reference<common_reference_t<T1, T2>, Rest...>
{};

} // namespace detail


template<class T1, class T2, class... Rest>
struct common_reference<T1, T2, Rest...>
    : detail::multiple_common_reference<void, T1, T2, Rest...>
{};


} // namespace jkl