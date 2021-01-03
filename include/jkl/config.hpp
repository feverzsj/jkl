#pragma once

#include <cstddef>
#include <boost/config.hpp>
#include <boost/assert.hpp>


namespace boost::asio {}
namespace boost::beast{}
namespace boost::hana {}
namespace boost::mp11 {}


namespace jkl{


namespace asio  = boost::asio;
namespace beast = boost::beast;
namespace hana  = boost::hana;
namespace mp11  = boost::mp11;

using size_t = std::size_t;
using ptrdiff_t = std::ptrdiff_t;


struct null_op_t
{
    template<class... T> constexpr null_op_t(T&&...) noexcept {}
    template<class... T> void operator()(T&&...) const noexcept{}
};

constexpr null_op_t null_op;


template<class T> struct dependent_false{ static constexpr bool value = false; };
template<class T> constexpr bool dependent_false_v = dependent_false<T>::value;

#define JKL_DEPENDENT_FAIL(type, msg) static_assert(::jkl::dependent_false_v<type>, msg)


template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
template<class... Ts> overload(Ts...) -> overload<Ts...>; // should be redundant in C++20, but no compiler implements that yet.


#define JKL_FORWARD(v) std::forward<decltype(v)>(v)
#define JKL_DECL_NO_CVREF_T(v) std::remove_cvref_t<decltype(v)>


// a constexpr function only evaluates at compile time when all it's parameters' lifetime begins within the evaluation of expression.
// so a constexpr function with reference parameter may or may not be evaluates at compile time, depend on the context.
// https://stackoverflow.com/questions/47269936/constexpr-expression-and-variable-lifetime-an-example-where-g-and-clang-disag
// but, if only the return type is cared, decltype(f(...)) would always work.
#define JKL_CEVL(e) decltype(e)::value


#ifdef BOOST_MSVC
#   define _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR template<int = 0>
#else
#   define _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
#endif


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr bool is_oneof(auto const& v, auto const&... es) noexcept
{
    return (... || (v == es));
}


} // namespace jkl
