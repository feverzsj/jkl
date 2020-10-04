#pragma once

#include <jkl/config.hpp>
#include <jkl/atask.hpp>

#include <tuple>
#include <array>
#include <atomic>
#include <type_traits>


namespace jkl{


// this coro should be hold by some executer's completion handle, i.e. co_await awaiter should actually results such.
template<std::size_t I, class Awaiter, class Data>
atask<void> _untill_all_coro(Awaiter awaiter, Data& d, std::shared_ptr<coro_frame_stack_holder> cfsh)
{
    try
    {
        if constexpr(std::is_void_v<co_await_result_t<Awaiter>>)
            co_await awaiter;
        else
            std::get<I>(d.results) = co_await awaiter;
    }
    catch(...)
    {
        std::get<I>(d.eps) = std::current_exception();
    }

    if(d.unfinished.fetch_sub(1, std::memory_order_acq_rel) == 1)
        cfsh.resume();
}



// NOTE: awaiters will be moved in
template<bool ThrowOnAnyFail, class... Awaiters>
auto _until_all(Awaiters&... awaiters)
{
    struct until_all_awaiter
    {
        std::tuple<Awaiters&...> awaiters;

        until_all_awaiter(Awaiters&...a) : awaiters(a...) {}

        coro_frame_stack_holder cfsh;
        std::atomic_int unfinished = sizeof...(awaiters);
        std::tuple<typename co_await_result<Awaiters>::atype...> results;
        std::array<std::exception_ptr, sizeof...(awaiters)> eps;

        template<class Promise, std::size_t... I>
        void _start_tasks(std::coroutine_handle<Promise> coro, std::index_sequence<I...>)
        {
            auto cfsh = std::make_shared<coro_frame_stack_holder>(coro);

            (..., _untill_all_coro<I>(std::move(std::get<I>(awaiters)), *this, cfsh).start());
        }

        bool await_ready() const noexcept { return false; }

        template<class Promise>
        void await_suspend(std::coroutine_handle<Promise> coro)
        {
            _start_tasks(coro, std::make_index_sequence<sizeof...(Awaiters)>{});
        }

        auto await_resume()
        {
            if constexpr(ThrowOnAnyFail)
            {
                for(auto& e : eps)
                {
                    if(e)
                        std::rethrow_exception(e);
                }

                return std::move(results);
            }
            else
            {
                return std::tuple(std::move(results), std::move(eps));
            }
        }
    };

    return until_all_awaiter{awaiters...};
}



// return std::tuple<results...>
// if co_await awaiter returns void, void_r will be used.
// throw if any task failed.
template<class... Awaiters>
auto until_all(Awaiters... awaiters)
{
    return _until_all<true>(Awaiters...);
}

// return std::tuple<std::tuple<results...>, std::array<expcetion_ptr, N>>
template<class... Awaiters>
auto until_all_r(Awaiters... awaiters)
{
    return _until_all<false>(Awaiters...);
}


} // namespace jkl