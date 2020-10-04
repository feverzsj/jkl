#pragma once

#include <jkl/config.hpp>
#include <jkl/error.hpp>
#include <jkl/util/concepts.hpp>


namespace jkl{


// usage:
//
// template<class T>
// aresult<T> fun()
// {
//     aerror_code ec;
//     T t;
//
//     /// if T is void, just return ec
//     return ec;
//
//     /// if T is not void:
//     if(ec)   // to return only ec, you must make
//         ec;  // sure ec actually contains error.
//     return t;           // to return only t,  T(ec) must be invalid,
//     return {no_err, t}; // otherwise use {no_err, args_to_T_ctor...}.
//
//     return {ec, t}; // or you can return both as {ec, args_to_T_ctor...}.
//                     // value is constructed only when ec contains error.
// }
//
// to use a fun/coro that [co_]return aresult easier:
//
// template<class Q>
// aresult<Q>/atask<aresult<Q>> foo()
// {
//     JKL_[CO_]TRY(auto&& v, fun());
//
//     // is equal to:
//
//     auto&& r = fun();
//     if(r.has_error())
//         [co_]return r.error();
//     auto&& v = static_cast<decltype(r)&&>(r).value();
//
//     // single argument would also work as expected
//     JKL_[CO_]TRY(fun());
// }

struct no_err_t {};
constexpr no_err_t no_err;


template<class V = void>
class [[nodiscard]] aresult;


template<>
class aresult<void>
{
    aerror_code _ec;

public:
    aresult() = default;

    constexpr aresult(_ec_or_ecenum_ auto const& e) noexcept
        : _ec{e}
    {}

    constexpr aresult(no_err_t) noexcept {}

    constexpr aerror_code const& error() const noexcept { return _ec; }
    constexpr bool has_error() const noexcept { return _ec.operator bool(); }
    constexpr bool no_error() const noexcept { return ! _ec; }
    constexpr explicit operator bool() const noexcept { return ! _ec; }

    constexpr void ignore() const noexcept {}
    constexpr void throw_on_error() const { if(_ec) throw asystem_error(_ec); }

    constexpr bool aborted() const noexcept { return _ec == asio::error::operation_aborted; }
    constexpr bool timeout() const noexcept { return _ec == gerrc::timeout; }
};


template<class V>
class aresult : public aresult<void>
{
    using base = aresult<void>;
    union{ V _v; };

public:
    using value_type = V;

    aresult() = default;

    ~aresult()
    {
        if constexpr(! std::is_trivially_destructible_v<V>)
        {
            if(has_value())
                _v.~V();
        }
    }

    constexpr aresult(aresult const& r) noexcept(std::is_nothrow_copy_constructible_v<V>)
                                        requires(std::copy_constructible<V>)
        : base(r)
    {
        if(r.has_value())
            new (&_v) V(r._v);
    }

    constexpr aresult& operator=(aresult const& r) noexcept(std::is_nothrow_copy_assignable_v<V>)
                                                   requires(std::is_copy_assignable_v<V>)
    {
        base::operator=(r);
        if(r.has_value())
            _v = r._v;
        return *this;
    }

    constexpr aresult(aresult&& r) noexcept(std::is_nothrow_move_constructible_v<V>)
                                   requires(std::move_constructible<V>)
        : base{std::move(r)}
    {
        if(r.has_value())
            new (&_v) V{std::move(r._v)};
    }

    constexpr aresult& operator=(aresult&& r) noexcept(std::is_nothrow_move_assignable_v<V>)
                                              requires(std::is_move_assignable_v<V>)
    {
        base::operator=(std::move(r));
        if(r.has_value())
            _v = std::move(r._v);
        return *this;
    }

    constexpr aresult(_ec_or_ecenum_ auto const& e) /*noexcept*/
        : base{e}
    {
        BOOST_ASSERT(base::has_error());

        if(has_value())
            throw std::runtime_error("aresult must either contain a error or a value.");
    }

    template<class U>
    constexpr aresult(U&& u) noexcept(std::is_nothrow_constructible_v<V, U&&>)
                             requires(std::constructible_from<V, U&&> && ! std::constructible_from<aerror_code, U&&>)
        : _v{std::forward<U>(u)}
    {}

    template<class U0, class... U>
    constexpr aresult(_ec_or_ecenum_ auto const& e, U0&& u0, U&&... u) noexcept(std::is_nothrow_constructible_v<V, U0&&, U&&...>)
        : base{e}
    {
        if(has_value())
            new (&_v) V(std::forward<U0>(u0), std::forward<U>(u)...);
    }

    constexpr aresult(no_err_t) noexcept {}

    template<class U0, class... U>
    constexpr aresult(no_err_t, U0&& u0, U&&... u) noexcept(std::is_nothrow_constructible_v<V, U0&&, U&&...>)
    {
        new (&_v) V(std::forward<U0>(u0), std::forward<U>(u)...);
    }

    constexpr bool has_value() const noexcept { return ! base::has_error(); }

    constexpr V      &  value()      &  noexcept { BOOST_ASSERT(has_value()); return _v; }
    constexpr V const&  value() const&  noexcept { BOOST_ASSERT(has_value()); return _v; }
    constexpr V      && value()      && noexcept { BOOST_ASSERT(has_value()); return static_cast<V      &&>(_v); }
    constexpr V const&& value() const&& noexcept { BOOST_ASSERT(has_value()); return static_cast<V const&&>(_v); }
    constexpr V      &  value_or_throw()      &  { if(has_value()) return _v; throw asystem_error(base::error()); }
    constexpr V const&  value_or_throw() const&  { if(has_value()) return _v; throw asystem_error(base::error()); }
    constexpr V      && value_or_throw()      && { if(has_value()) return static_cast<V      &&>(_v); throw asystem_error(base::error()); }
    constexpr V const&& value_or_throw() const&& { if(has_value()) return static_cast<V const&&>(_v); throw asystem_error(base::error()); }

    constexpr V      &  operator*()      &  noexcept { return value(); }
    constexpr V const&  operator*() const&  noexcept { return value(); }
    constexpr V      && operator*()      && noexcept { return value(); }
    constexpr V const&& operator*() const&& noexcept { return value(); }

    constexpr V      * operator->()       noexcept { return std::addressof(value()); }
    constexpr V const* operator->() const noexcept { return std::addressof(value()); }
};


template<class T> struct is_aresult             : std::false_type{};
template<class T> struct is_aresult<aresult<T>> : std::true_type {};

template<class T> constexpr bool is_aresult_v = is_aresult<T>::value;


template<class T>
constexpr void throw_on_error(aresult<T> const& r)
{
    r.throw_on_error();
}


} // namespace jkl



// #if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 8
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wparentheses"
// #endif


#define JKL_TRY_UNIQUE_NAME BOOST_JOIN(_jkl_result_try_unique_name_temporary, __COUNTER__)

// SOME_MACRO(T<A, B, C>), preprocessor cannot recognize template and considers there are 3 arguments.
// So, we use following mechanism to support up to 8 template arguments for JKL_TRY/JKL_CO_TRY
// in case the first argument is not initializer but template, use JKL_TRYV
#define JKL_TRY_RETURN_ARG_COUNT(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, count, ...) count
#define JKL_TRY_EXPAND_ARGS(args) JKL_TRY_RETURN_ARG_COUNT args
#define JKL_TRY_COUNT_ARGS_MAX8(...) JKL_TRY_EXPAND_ARGS((__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#define JKL_TRY_OVERLOAD_MACRO2(name, count) name##count
#define JKL_TRY_OVERLOAD_MACRO1(name, count) JKL_TRY_OVERLOAD_MACRO2(name, count)
#define JKL_TRY_OVERLOAD_MACRO(name, count) JKL_TRY_OVERLOAD_MACRO1(name, count)
#define JKL_TRY_OVERLOAD_GLUE(x, y) x y
#define JKL_TRY_CALL_OVERLOAD(name, ...)                                                                                                                   \
  JKL_TRY_OVERLOAD_GLUE(JKL_TRY_OVERLOAD_MACRO(name, JKL_TRY_COUNT_ARGS_MAX8(__VA_ARGS__)), (__VA_ARGS__))


// NOTE: JKL[_CO]_TRY must be in a single scope
// if(...) JKL_TRY(...); // error, undeclared variable
// if(...){ JKL_TRY(...); } // ok

#define JKL_TRYV2(xreturn, r, ...)  \
  auto&& r = (__VA_ARGS__);         \
  if(BOOST_UNLIKELY(r.has_error())) \
    xreturn r.error()               \
/**/

#define JKL_TRY2(xreturn, r, v, ...)       \
  JKL_TRYV2(xreturn, r, __VA_ARGS__);      \
  v = static_cast<decltype(r)>(r).value(); \
/**/


#define JKL_TRYV(...) JKL_TRYV2(return, JKL_TRY_UNIQUE_NAME, __VA_ARGS__)
#define JKL_TRYA(v, ...) JKL_TRY2(return, JKL_TRY_UNIQUE_NAME, v, __VA_ARGS__)
#define JKL_CO_TRYV(...) JKL_TRYV2(co_return, JKL_TRY_UNIQUE_NAME, __VA_ARGS__)
#define JKL_CO_TRYA(v, ...) JKL_TRY2(co_return, JKL_TRY_UNIQUE_NAME, v, __VA_ARGS__)


#define JKL_TRY_INVOKE_TRY8(a, b, c, d, e, f, g, h) JKL_TRYA(a, b, c, d, e, f, g, h)
#define JKL_TRY_INVOKE_TRY7(a, b, c, d, e, f, g) JKL_TRYA(a, b, c, d, e, f, g)
#define JKL_TRY_INVOKE_TRY6(a, b, c, d, e, f) JKL_TRYA(a, b, c, d, e, f)
#define JKL_TRY_INVOKE_TRY5(a, b, c, d, e) JKL_TRYA(a, b, c, d, e)
#define JKL_TRY_INVOKE_TRY4(a, b, c, d) JKL_TRYA(a, b, c, d)
#define JKL_TRY_INVOKE_TRY3(a, b, c) JKL_TRYA(a, b, c)
#define JKL_TRY_INVOKE_TRY2(a, b) JKL_TRYA(a, b)
#define JKL_TRY_INVOKE_TRY1(a) JKL_TRYV(a)

#define JKL_TRY(...) JKL_TRY_CALL_OVERLOAD(JKL_TRY_INVOKE_TRY, __VA_ARGS__)


#define JKL_CO_TRY_INVOKE_TRY8(a, b, c, d, e, f, g, h) JKL_CO_TRYA(a, b, c, d, e, f, g, h)
#define JKL_CO_TRY_INVOKE_TRY7(a, b, c, d, e, f, g) JKL_CO_TRYA(a, b, c, d, e, f, g)
#define JKL_CO_TRY_INVOKE_TRY6(a, b, c, d, e, f) JKL_CO_TRYA(a, b, c, d, e, f)
#define JKL_CO_TRY_INVOKE_TRY5(a, b, c, d, e) JKL_CO_TRYA(a, b, c, d, e)
#define JKL_CO_TRY_INVOKE_TRY4(a, b, c, d) JKL_CO_TRYA(a, b, c, d)
#define JKL_CO_TRY_INVOKE_TRY3(a, b, c) JKL_CO_TRYA(a, b, c)
#define JKL_CO_TRY_INVOKE_TRY2(a, b) JKL_CO_TRYA(a, b)
#define JKL_CO_TRY_INVOKE_TRY1(a) JKL_CO_TRYV(a)

#define JKL_CO_TRY(...) JKL_TRY_CALL_OVERLOAD(JKL_CO_TRY_INVOKE_TRY, __VA_ARGS__)


// #if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 8
// #pragma GCC diagnostic pop
// #endif
