#pragma once


#include <jkl/config.hpp>
#include <jkl/util/str.hpp>
#include <jkl/util/concepts.hpp>
#include <jkl/util/type_traits.hpp>
#include <boost/container_hash/hash.hpp>
#include <utility>


namespace jkl{


// Extended hash framework that supports extra hasher.
// which allows user to customize jkl_hash_value to support any unordered associative container with any hasher.
// just use select_hash<preferred_hash, T...> to replace the Hash template argument of containers.

// Heterogeneous lookups are enabled for string class key to accept c string
// and smart pointer key to accept pointer without construct key_type:


template<class Hash, class T>
concept _hashable_ = requires(Hash hash, T v){ {hash(v)} -> std::same_as<size_t>; };



template<class Hash, class T>
constexpr size_t jkl_hash_value(Hash hash, T const& v) noexcept requires(_hashable_<Hash, T>)
{
    return hash(v);
}


template<bool> struct select_transparent       { using is_transparent = void; };
template<    > struct select_transparent<false>{                              };


// clang workaround
template<class T, class U>
concept __select_hash_cond1 = _smart_ptr_<T> && std::convertible_to<U*, typename T::element_type*>;

template<class Hash, class T>
struct select_hash : select_transparent<_str_class_<T> || _smart_ptr_<T>>
{
    constexpr size_t operator()(T const& v) const noexcept
    {
        return jkl_hash_value(Hash(), v);
    }

    template<_str_ U>
    constexpr size_t operator()(U const& v) const noexcept
                                            requires(_str_class_<T> && _similar_str_<T, U>)
    {
        return jkl_hash_value(Hash(), as_str_class(v));
    }

    template<class U>
    constexpr size_t operator()(U* v) const noexcept
                                      // requires(_smart_ptr_<T> && std::convertible_to<U*, typename T::element_type*>)
                                      requires(__select_hash_cond1<T, U>)
    {
        return jkl_hash_value(Hash(), v);
    }
};


template<template<class> class Hash>
struct hash_wrapper
{
    template<class T>
    constexpr size_t operator()(T const& v) const noexcept requires(_hashable_<Hash<T>, T>)
    {
        return Hash<T>()(v);
    }
};


template<template<class> class Hash, class T>
using select_hash_tpl = select_hash<hash_wrapper<Hash>, T>;


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr void hash_merge(size_t& seed, auto... hashValues) noexcept
{
    (... , boost::hash_detail::hash_combine_impl(seed, hashValues));
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr size_t merged_hash(auto... hashValues) noexcept
{
    size_t seed = 0;
    hash_merge(seed, hashValues...);
    return seed;
}


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr void hash_combine(size_t& seed, auto hash, auto const&... vs) noexcept
{
    (... , hash_merge(seed, jkl_hash_value(hash, vs)));
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr size_t combined_hash(auto hash, auto const&... vs) noexcept
{
    size_t seed = 0;
    hash_combine(seed, hash, vs...);
    return seed;
}


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr void hash_range(size_t& seed, auto hash, auto beg, auto end) noexcept
{
    for(; beg != end; ++beg)
        hash_combine(seed, hash, *beg);
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr void hash_range(size_t& seed, auto hash, auto const& r) noexcept
{
    hash_range(seed, hash, std::begin(r), std::end(r));
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr size_t range_hash(auto hash, auto beg, auto end) noexcept
{
    size_t seed = 0;
    hash_range(seed, hash, beg, end);
    return seed;
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr size_t range_hash(auto hash, auto const& r) noexcept
{
    size_t seed = 0;
    hash_range(seed, hash, r);
    return seed;
}


// type support

template<class Hash, _std_tuple_ T>
constexpr size_t jkl_hash_value(Hash hash, T const& v) noexcept requires(! _hashable_<Hash, T>)
{
    return std::apply([hash](auto const&... e){ return combined_hash(hash, e...); }, v);
}


} // namespace jkl
