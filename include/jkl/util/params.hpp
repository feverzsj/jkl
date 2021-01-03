#pragma once

#include <jkl/config.hpp>
#include <tuple>
#include <type_traits>


namespace jkl{


template<class... P>
class make_params : private std::tuple<P...>
{
    using base = std::tuple<P...>;

    template<class Tag>
    static constexpr typename Tag::__ERROR__parameter_not_found _get()
    {
        return {};
    }

    template<class Tag, class E0, class... E>
    static constexpr decltype(auto) _get(E0& e0 [[maybe_unused]], E&... e [[maybe_unused]])
    {
        if constexpr(std::is_invocable_v<E0, Tag>)
            return e0(Tag{});
        else
            return _get<Tag>(e...);
    }

public:
    using base::base;

    template<class Tag>
    constexpr decltype(auto) operator()(Tag) const
    {
        return std::apply(
            [](auto&... e) -> decltype(auto)
            {
                return _get<Tag>(e...);
            },
            static_cast<base const&>(*this));
    }

    template<class Tag>
    static constexpr bool has(Tag)
    {
        return std::is_invocable_v<make_params, Tag>;
    }
};

template<class... P>
make_params(P...)->make_params<P...>;


// usage:
//  [constexpr] auto params = make_params(p..., p_default_option...); // put default options last,
//   ^^^^^^^^^ if all p are constexpr.                                // or use params.has(tag) to detect if an option exists
//  [constexpr] auto o = params(option_tag);
//   ^^^^^^^^^ if this option is constexpr.


// generate a param for Tag that forwards v
template<class Tag>
inline constexpr auto p_forward = []<class T>(T&& v) { return [&v](Tag) -> T&& { return static_cast<T&&>(v); }; };


} // namespace jkl
