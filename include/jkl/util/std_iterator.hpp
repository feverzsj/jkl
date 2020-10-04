#pragma once

#include <cstddef> // for _LIBCPP_STD_VER

#if __has_include(<concepts>) && ! defined(_LIBCPP_STD_VER)
#   include <iterator>
#else
#include <jkl/config.hpp>
#include <jkl/util/std_concepts.hpp>


namespace jkl_detail{



} // namespace jkl_detail


namespace std{

template<class T>
using __with_reference = T&;

template<class T>
META_CONCEPT __can_reference = requires { typename __with_reference<T>; };


template<class T>
META_CONCEPT __dereferenceable = requires(T& t) { {*t} -> __can_reference; };


template<__dereferenceable R>
using iter_reference_t = decltype(*std::declval<R&>());
input_iterator

} // namespace std

#endif
