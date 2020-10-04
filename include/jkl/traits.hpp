#pragma once


#include <jkl/config.hpp>
#include <jkl/std_coro.hpp>
#include <jkl/util/concepts.hpp>
#include <boost/beast/core/stream_traits.hpp>


namespace jkl{


template<class T>
decltype(auto) get_executor(T&& t) noexcept
{
    if constexpr(requires{ std::forward<T>(t).get_executor(); })
        return std::forward<T>(t).get_executor();
    else
        return std::forward<T>(t);
}

template<class T>
using executor_t = std::remove_cvref_t<decltype(get_executor(std::declval<T>()))>;

template<class T, class Ex>
using rebind_executor_t =  typename T::template rebind_executor<Ex>::type;


template<class T>
decltype(auto) get_next_layer(T&& t)
{
    if constexpr(requires{ std::forward<T>(t).next_layer(); })
        return std::forward<T>(t).next_layer();
    else
        return std::forward<T>(t).get_executor();
}

using beast::get_lowest_layer;

template<class T> using next_layer_t   = std::remove_cvref_t<decltype(get_next_layer(std::declval<T>()))>;
template<class T> using lowest_layer_t = std::remove_cvref_t<decltype(get_lowest_layer(std::declval<T>()))>;


// get asio.AsyncStream
template<class T>
decltype(auto) get_astream(T& t)
{
    if constexpr(requires{ t.astream(); })
        return t.astream();
    else
        return t;
}

template<class T> using astream_t = std::remove_cvref_t<decltype(get_astream(std::declval<T>()))>;


template<class T>
concept _awaiter_ = requires(T t, std::coroutine_handle<> h)
                    {
                        {t.await_ready()} -> std::same_as<bool>;
                        t.await_suspend(h);
                        t.await_resume();
                    };

template<class T>
concept _awaitable_ = requires(T&& t){ {std::forward<T>(t).operator co_await()} -> _awaiter_; }
                      || requires(T&& t){ {operator co_await(std::forward<T>(t))} -> _awaiter_; };


template<class T>
concept _co_awaitable_ = _awaiter_<T> || _awaitable_<T>;


template<_awaiter_ T>
decltype(auto) get_awaiter(T&& t) noexcept
{
    return std::forward<T>(t);
}

template<_awaitable_ T>
decltype(auto) get_awaiter(T&& t)
{
    if constexpr(requires{ std::forward<T>(t).operator co_await(); })
        return std::forward<T>(t).operator co_await();
    else
        return operator co_await(std::forward<T>(t));
}


template<class T> requires(_co_awaitable_<T>)
using awaiter_t = std::remove_cvref_t<decltype(get_awaiter(std::declval<T>()))>;


// determine what the resulting type of a co_await expression will be if applied to an expression of type T,
// assuming the value of type T is being awaited in a context where it is unaffected by any promise.await_transform().
template<class T> requires(_co_awaitable_<T>)
using await_result_t = decltype(get_awaiter(std::declval<T>()).await_resume());


} // namespace jkl