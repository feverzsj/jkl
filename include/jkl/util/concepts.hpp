#pragma once

#include <jkl/config.hpp>
#include <jkl/util/type_traits.hpp>
#include <jkl/util/std_concepts.hpp>
#include <memory>
#include <tuple>


namespace jkl{


// NOTE: only cv-qualifiers are stripped.

template<class T, class... Args> concept _trivially_constructible_ = std::is_trivially_constructible_v<T, Args...>;

template<class T> concept _trivially_copyable_ = std::is_trivially_copyable_v<T>;

template<class T> concept _trivial_ = _trivially_constructible_<T> && _trivially_copyable_<T>;
template<class T> concept _standard_layout_ = std::is_standard_layout_v<T>;

template<class T> concept _pod_   = _trivial_<std::remove_cvref_t<T>> && _standard_layout_<std::remove_cvref_t<T>>;
template<class T> concept _class_ = std::is_class_v<std::remove_cvref_t<T>>;

template<class T> concept _array_ = std::is_array_v<T>; // array types are considered to have the same cv-qualification as their element types, so no need of remove_cv_t
template<class T> concept _pointer_ = std::is_pointer_v<T>;

template<class T> concept _nullptr_ = std::is_null_pointer_v<T>;

template<class T> concept _arithmetic_ = std::integral<std::remove_cvref_t<T>> || std::floating_point<std::remove_cvref_t<T>>;
template<class T> concept _unsigned_   = std::is_unsigned_v<std::remove_cvref_t<T>>; // for unsigned arithmetic
template<class T> concept _signed_     = std::is_signed_v<std::remove_cvref_t<T>>;   // for signed arithmetic, also including floating point
template<class T> concept _strict_integral_ = std::integral<std::remove_cvref_t<T>> && ! std::same_as<std::remove_cvref_t<T>, bool>;


template<class T> concept _char_   = is_one_of_v<std::remove_cv_t<T>, char, signed char, unsigned char, wchar_t, char8_t, char16_t, char32_t>;
template<class T> concept _byte_   = (sizeof(T) == sizeof(std::byte)) && _pod_<T>;
template<class T> concept _8bits_  = (sizeof(T) * CHAR_BIT == 8 ) && _pod_<T>;
template<class T> concept _16bits_ = (sizeof(T) * CHAR_BIT == 16) && _pod_<T>;
template<class T> concept _32bits_ = (sizeof(T) * CHAR_BIT == 32) && _pod_<T>;
template<class T> concept _64bits_ = (sizeof(T) * CHAR_BIT == 64) && _pod_<T>;

template<class T> concept _tuple_like_ = requires(T t){ std::tuple_size<T>::value; std::get<0>(t); }; // std::apply() can be called on tuple like objects

template<class T> concept _array_class_ = is_array_class_v<T>;

template<class T> concept _unique_ptr_ = is_unique_ptr_v<T>;
template<class T> concept _shared_ptr_ = std::same_as<std::remove_cv_t<T>, std::shared_ptr<T>>;
template<class T> concept _smart_ptr_  = _unique_ptr_<T> || _shared_ptr_<T>;


// unlike is_invocable_r which matches void return type to any type, this only matches void return type to void
template<class R, class F, class... Args>
concept _callable_r_ = requires(F&& f, Args&&... args){
    {std::invoke(static_cast<F&&>(f), static_cast<Args&&>(args)...)} -> std::convertible_to<R>;
};


} // namespace jkl