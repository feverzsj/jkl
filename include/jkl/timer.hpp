#pragma once


#include <jkl/config.hpp>
#include <jkl/ec_awaiter.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/high_resolution_timer.hpp>


namespace jkl{


template<class Timer>
class awaitable_timer : public Timer
{
public:
    using duration   = typename Timer::duration;
    using time_point = typename Timer::time_point;

    template<class... P>
    auto wait(P... p)
    {
        return make_ec_awaiter(
            [&](auto&& h){ Timer::async_wait(std::move(h)); },
            p...
        );
    }

    template<class... P>
    auto wait(duration const& d, P... p) { Timer::expires_after(d); return wait(p...); }

    template<class... P>
    auto wait_until(time_point const& t, P... p) { Timer::expires_at(t); return wait(p...); }
};


using steady_timer          = awaitable_timer<asio::steady_timer>;
using system_timer          = awaitable_timer<asio::system_timer>;
using deadline_timer        = awaitable_timer<asio::deadline_timer>;
using high_resolution_timer = awaitable_timer<asio::high_resolution_timer>;


} // namespace jkl