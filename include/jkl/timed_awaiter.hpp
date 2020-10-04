#pragma once

#include <jkl/config.hpp>
#include <jkl/error.hpp>
#include <jkl/params.hpp>
#include <jkl/result.hpp>
#include <jkl/std_stop_token.hpp>
#include <jkl/util/type_traits.hpp>
#include <optional>


namespace jkl{


// awaiter should be stoppable, i.e. it will handle stop_token
template<class Awaiter, bool EnableStop>
struct timed_awaiter
{
    Awaiter _awaiter;
    atask<> _awaiterTask;
    asio::steady_timer _timer;
    aerror_code _ec;
    std::coroutine_handle<> _coro;

    [[no_unique_address]] not_null_if_t<EnableStop,
        std::optional<std::stop_callback<std::function<void()>>>> _stopCb;

    template<class A, class ExC>
    timed_awaiter(A&& a, ExC&& exc, asio::steady_timer::duration const& dur)
        : _awaiter(std::forward<A>(a)), _timer(std::forward(exc), dur)
    {}

    bool await_ready() { return _awaiter.await_ready(); }

    template<class Promise>
    bool await_suspend(std::coroutine_handle<Promise> c)
    {
        if constexpr(EnableStop)
        {
            if(c.promise().stop_requested())
            {
                _ec = asio::error::operation_aborted;
                return false;
            }
        }

        _coro = c;

        // this class will be destructed after coro.resume()d,
        // since only the first path setting this flag calls coro.resume(),
        // there's no need to put other data in shared_ptr
        auto anyReached = std::make_shared<std::atomic_flag>();

        _awaiterTask =
            [](timed_awaiter& self, auto anyReached)->atask<>
            {
                co_await std::suspend_always(); // task must be start()ed to construct stop_source, so we stop it's coro explicitly
                                                // which will be later resumed by _awaiter

                if(! anyReached.test_and_set(memory_order::memory_order_relaxed))
                {
                    self._timer.cancel();
                    self._coro.resume();
                }
            }
        (*this, anyReached);


        _awaiterTask.start(); // necessary to construct stop_source


        bool directReturn = false;

        if constexpr(requires{ directReturn = _awaiter.await_suspend(_task.coroutine()); })
        {
            directReturn = _awaiter.await_suspend(_task.coroutine());
        }
        else if constexpr(requires(std::coroutine_handle<> dest){ dest = _awaiter.await_suspend(_task.coroutine()); })
        {
            auto dest = _awaiter.await_suspend(_task.coroutine());
            BOOST_ASSERT(_task.coroutine() == dest || std::noop_coroutine() == dest)
            directReturn = (_task.coroutine() == dest);
        }

        if(! directReturn)
        {
            _timer.async_wait(
                [this, anyReached](auto&& ec)
                {
                    if(anyReached.test_and_set(memory_order::memory_order_relaxed))
                        return;
                    BOOST_ASSERT(! ec);
                    _ec = gerrc::timeout;
                    _awaiterTask.request_stop();
                    _coro.resume();
                }
            );

            if constexpr(EnableStop)
            {
                BOOST_ASSERT(! _stopCb);
                _stopCb.emplace(c.promise().get_stop_token(),
                    [this, anyReached]()
                    {
                        if(anyReached.test_and_set(memory_order::memory_order_relaxed))
                            return;
                        _ec = asio::error::operation_aborted;
                        _timer.cancel();
                        _awaiterTask.request_stop();
                        _coro.resume();
                    }
                );
            }
        }

        return ! directReturn;
    }

    using atype = await_result_t<Awaiter>;
    using rtype = std::conditional_t<is_aresult_v<atype>, atype, aresult<atype>>;

    rtype await_resume()
    {
        if(_ec)
            return _ec;
        else
            return _awaiter.await_resume();
    }
};

template<class Awaitable, class ExC, class... P>
auto expires_after(Awaitable&& a, ExC&& e, asio::steady_timer::duration const& dur, P... p)
{
    auto params = make_params(p..., p_disable_stop); // put default options last

    return timed_awaiter<awaiter_t, params(t_stop_enabled)>(
        get_awaiter(std::forward<Awaitable>(a)), get_executor(std::forward<ExC>(e)), dur);
}


} // namespace jkl