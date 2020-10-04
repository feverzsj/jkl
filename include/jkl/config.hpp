#pragma once

#include <cstddef>
#include <boost/config.hpp>
#include <boost/assert.hpp>


namespace boost::asio{}
namespace boost::beast{}


namespace jkl{


namespace asio  = boost::asio;
namespace beast = boost::beast;

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


#define JKL_FORWARD(t) std::forward<decltype(t)>(t)


} // namespace jkl
