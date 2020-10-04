#pragma once

#include <jkl/config.hpp>
#include <jkl/config.hpp>
#include <boost/asio/connect.hpp>


namespace jkl{


template<class S, class T, class... P>
auto connect(S& s, T&& t, P... p)
{
    using endpoint_type = decltype(get_lowest_layer(s).remote_endpoint());

    if constexpr(std::is_same_v<std::remove_cvref_t<T>, endpoint_type>)
    {
        return make_ec_awaiter<void>(
            [&, t](auto&& h){ get_astream(s).async_connect(t, std::move(h)); },
            p...);
    }
    else if constexpr()
    {

    }

}


} // namespace jkl