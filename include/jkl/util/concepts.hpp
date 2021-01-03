#pragma once

#include <jkl/config.hpp>
#include <jkl/util/type_traits.hpp>
#include <jkl/util/std_concepts.hpp>
#include <memory>
#include <tuple>
#include <climits>


namespace jkl{


// In Most Cases:
//   for type traits, apply std::remove_cv_t on the tested type;
//   for concepts, apply std::remove_cvref_t on the tested type;


template<class T, class... Args> concept _trivially_constructible_ = std::is_trivially_constructible_v<std::remove_cvref_t<T>, Args...>;

template<class T> concept _trivially_copyable_ = std::is_trivially_copyable_v<std::remove_cvref_t<T>>;

template<class T> concept _trivial_ = _trivially_constructible_<T> && _trivially_copyable_<T>;
template<class T> concept _standard_layout_ = std::is_standard_layout_v<std::remove_cvref_t<T>>;

template<class T> concept _pod_   = _trivial_<T> && _standard_layout_<T>;
template<class T> concept _class_ = std::is_class_v<std::remove_cvref_t<T>>;

template<class T> concept _pointer_ = std::is_pointer_v<std::remove_reference_t<T>>;

template<class T> concept _nullptr_ = std::is_null_pointer_v<std::remove_reference_t<T>>;

template<class T> concept _arithmetic_ = std::is_integral_v<std::remove_reference_t<T>> || std::is_floating_point_v<std::remove_reference_t<T>>;
template<class T> concept _unsigned_   = std::is_unsigned_v<std::remove_reference_t<T>>; // for unsigned arithmetic
template<class T> concept _signed_     = std::is_signed_v<std::remove_reference_t<T>>;   // for signed arithmetic, also including floating point
template<class T> concept _strict_integral_ = std::is_integral_v<std::remove_reference_t<T>> && ! std::is_same_v<std::remove_cvref_t<T>, bool>;

template<class T> concept _char_   = is_one_of_v<std::remove_cvref_t<T>, char, signed char, unsigned char, wchar_t, char8_t, char16_t, char32_t>;
template<class T> concept _byte_   = (sizeof(T) == sizeof(std::byte)) && _pod_<T>;
template<class T> concept _8bits_  = (sizeof(T) * CHAR_BIT == 8 ) && _pod_<T>;
template<class T> concept _16bits_ = (sizeof(T) * CHAR_BIT == 16) && _pod_<T>;
template<class T> concept _32bits_ = (sizeof(T) * CHAR_BIT == 32) && _pod_<T>;
template<class T> concept _64bits_ = (sizeof(T) * CHAR_BIT == 64) && _pod_<T>;

template<class T> concept _array_ = std::is_array_v<std::remove_reference_t<T>>; // array types are considered to have the same cv-qualification as their element types, https://stackoverflow.com/questions/27770228/what-type-should-stdremove-cv-produce-on-an-array-of-const-t
template<class T> concept _std_array_ = is_std_array_v<std::remove_reference_t<T>>;

template<class T> concept _std_tuple_ = requires(T& t){ std::tuple_size<std::remove_reference_t<T>>::value; std::get<0>(t); };

template<class T> concept _unique_ptr_ = is_unique_ptr_v<std::remove_reference_t<T>>;
template<class T> concept _shared_ptr_ = is_shared_ptr_v<std::remove_reference_t<T>>;
template<class T> concept _smart_ptr_  = _unique_ptr_<T> || _shared_ptr_<T>;

template<class T> concept _optional_ = is_optional_v<std::remove_reference_t<T>>;


// unlike is_invocable_r which matches void return type to any type, this only matches void return type to void
template<class R, class F, class... Args>
concept _callable_r_ = requires(F&& f, Args&&... args){
    {std::invoke(static_cast<F&&>(f), static_cast<Args&&>(args)...)} -> std::convertible_to<R>;
};


template<class T>
concept _range_ = requires(T& r){ std::begin(r); std::end(r); };

template<class T>
concept _random_access_range_ = _range_<T> && requires(T& r){ {std::end(r) - std::begin(r)} -> std::integral; };

template<_range_ R>
using range_value_t = std::remove_cvref_t<decltype(*std::begin(std::declval<R&>()))>;

template<class T, class R>
concept _similar_random_access_range_ = _random_access_range_<R> && _random_access_range_<T> && std::same_as<range_value_t<T>, range_value_t<R>>;

template<class T, class R>
concept _similar_random_access_range_or_val_ = _random_access_range_<R> && (std::same_as<T, range_value_t<R>>
                                                || (_random_access_range_<T> && std::same_as<range_value_t<T>, range_value_t<R>>));


} // namespace jkl
