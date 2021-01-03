#pragma once

#include <jkl/config.hpp>
#include <boost/endian/conversion.hpp>


namespace jkl{


namespace bendian = boost::endian;


// NOTE:
// sizeof(T) must be 1, 2, 4, or 8
// 1 <= N <= sizeof(T)
// T is TriviallyCopyable
// if N < sizeof(T), T must be integral or enum

template<size_t N, class T>
void store_le(void* p, T v) noexcept { bendian::endian_store<T, N, bendian::order::little>(reinterpret_cast<unsigned char*>(p), v); }
template<size_t N, class T>
void store_be(void* p, T v) noexcept { bendian::endian_store<T, N, bendian::order::big   >(reinterpret_cast<unsigned char*>(p), v); }

template<class T, size_t N>
T load_le(void const* p) noexcept { return bendian::endian_load<T, N, bendian::order::little>(reinterpret_cast<unsigned char const*>(p)); }
template<class T, size_t N>
T load_be(void const* p) noexcept { return bendian::endian_load<T, N, bendian::order::big   >(reinterpret_cast<unsigned char const*>(p)); }

template<size_t N, class T>
void load_le(void const* p, T& v) noexcept { v = load_le<T, N>(p); }
template<size_t N, class T>
void load_be(void const* p, T& v) noexcept { v = load_be<T, N>(p); }


// N = sizeof(T)

template<class T>
void store_le(void* p, T v) noexcept { store_le<sizeof(T)>(p, v); }
template<class T>
void store_be(void* p, T v) noexcept { store_be<sizeof(T)>(p, v); }

template<class T>
T load_le(void const* p) noexcept { return load_le<T, sizeof(T)>(p); }
template<class T>
T load_be(void const* p) noexcept { return load_be<T, sizeof(T)>(p); }

template<class T>
void load_le(void const* p, T& v) noexcept { v = load_le<T>(p); }
template<class T>
void load_be(void const* p, T& v) noexcept { v = load_be<T>(p); }


// bool

inline void store_le(void* p, bool v) noexcept { reinterpret_cast<char*>(p)[0] = v ? 'T' : 'F'; }
inline void store_be(void* p, bool v) noexcept { store_le(p, v); }

template<>
inline bool load_le<bool>(void const* p) noexcept { return reinterpret_cast<char const*>(p)[0] == 'T'; }
template<>
inline bool load_be<bool>(void const* p) noexcept { return load_le<bool>(p); }


} // namespace jkl
