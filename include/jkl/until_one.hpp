#pragma once

#include <jkl/config.hpp>
#include <jkl/atask.hpp>

#include <any>
#include <tuple>
#include <array>
#include <atomic>
#include <type_traits>


namespace jkl{


// this coro should be hold by some executer's completion handle, i.e. co_await awaiter should actually results such.
template<std::size_t I, class Awaiter, class SharedData, class SharedUCoro>
atask<void> _untill_one_coro(Awaiter awaiter, SharedData d, SharedUCoro next)
{
    using result_t = typename co_await_result<Awaiter>::type;

    if(d->finished.load(std::memory_order_acquire)) // if other coro finished
        co_return;

    try
    {
        if constexpr(std::is_void_v<result_t>)
            co_await awaiter;
        else
            std::get<I>(d->results) = co_await awaiter;

        if(! d->finished.exchange(true, std::memory_order_release)) // if this coro is first finished successfully
        {
            d->idx = I;
            move_back_ucoro_and_resume(*next);
        }
    }
    catch(...)
    {
        std::get<I>(d->eps) = std::current_exception();
    }
}



// NOTE: awaiters will be moved in
template<class... Awaiters>
auto _until_one(Awaiters&... awaiters)
{
    struct until_one_awaiter
    {
        std::tuple<Awaiters&...> awaiters;

        until_one_awaiter(Awaiters&...a) : awaiters(a...) {}

        struct data_t
        {
            std::atomic_bool finished = false;
            int idx = -1;
            std::array<std::any, sizeof...(awaiters)> results;
            std::array<std::exception_ptr, sizeof...(awaiters)> eps;
        };

        std::shared_ptr<data_t> d;

        template<class Promise, std::size_t... I>
        void _start_tasks(std::coroutine_handle<Promise> coro, std::index_sequence<I...>)
        {
            d = std::make_shared<data_t>();
            auto next = std::make_shared<unique_coro<Promise>>(move_out_ucoro(coro));

            (..., _untill_one_coro<I>(std::move(std::get<I>(awaiters)), d, next).start());
        }

        bool await_ready() const noexcept { return false; }

        template<class Promise>
        void await_suspend(std::coroutine_handle<Promise> coro)
        {
            _start_tasks(coro, std::make_index_sequence<sizeof...(Awaiters)>{});
        }

        auto await_resume()
        {
            if(d->idx < 0) // all failed
            {
                BOOST_ASSERT(d->eps[0]);
                std::rethrow_exception(d->eps[0]);
            }

            return std::move(d->results[d->idx]);
        }
    };

    return until_one_awaiter{awaiters...};
}



// return the first successfully finished;
// return std::any as result;
// if co_await awaiter returns void, an empty std::any will be returned
// throw one of the exceptions if all failed.
template<class... Awaiters>
auto until_one(Awaiters... awaiters)
{
    return _until_one(Awaiters...);
}


} // namespace jkl