#pragma once

#include <jkl/config.hpp>
#include <tuple>
#include <type_traits>


namespace jkl{


template<class Tag, class T>
struct tagged_obj
{
    using tag = Tag;
    T obj;
};


template<class Tag, class V>
constexpr tagged_obj<Tag, V&&> tag_ref(V&& v) noexcept
{
    return {JKL_FORWARD(v)};
}

template<class Tag, class V>
constexpr tagged_obj<Tag, std::remove_cvref_t<V>> tag_val(V&& v) noexcept(std::is_nothrow_copy_constructible_v<V>)
{
    return {JKL_FORWARD(v)};
}


template<class T>
concept _tagged_ = requires{ typename std::remove_cvref_t<T>::tag; };

template<class T, class BaseTag>
concept _tagged_from_ = _tagged_<T> && std::is_base_of_v<BaseTag, typename std::remove_cvref_t<T>::tag>;

template<class T, class Tag>
concept _tagged_as_ = _tagged_from_<T, Tag> && std::same_as<typename std::remove_cvref_t<T>::tag, Tag>;
                   // ^^^^^^^^^^^^^ _tagged_as_ is more constrained than _tagged_from_

template<class Tuple, class Tag, size_t Idx = 0>
consteval size_t tagged_elem_idx()
{
    using tuple_t = std::remove_cvref_t<Tuple>;

    if constexpr(Idx >= std::tuple_size_v<tuple_t>)
        return static_cast<size_t>(-1);
    else if constexpr(_tagged_as_<std::tuple_element_t<Idx, tuple_t>, Tag>)
        return Idx;
    else
        return tagged_elem_idx<Tuple, Tag, Idx + 1>();
}


template<class Tuple, class Tag>
inline constexpr bool has_tag = tagged_elem_idx<Tuple, Tag>() != static_cast<size_t>(-1);

template<class Tuple, class.. Tags>
inline constexpr bool has_oneof_tag = (... || has_tag<Tuple, Tags>);


// get the first element that is _tagged_as_<Tag>, element can be ref.
template<class Tag, class Tuple>
constexpr auto&& get_tagged(Tuple&& t) noexcept requires(has_tag<Tuple, Tag>)
{
    return std::get<tagged_elem_idx<Tuple, Tag>()>(JKL_FORWARD(t));
}


} // namespace jkl
