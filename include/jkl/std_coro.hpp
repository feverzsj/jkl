#pragma once

#include <jkl/config.hpp>
#include <jkl/util/handle.hpp>
#include <jkl/util/type_traits.hpp>

#if __has_include(<coroutine>)
#   include <coroutine>
#elif __has_include(<experimental/coroutine>)
#   include <experimental/coroutine>
    namespace std{
        using std::experimental::coroutine_handle;
        using std::experimental::coroutine_traits;
        using std::experimental::suspend_always;
        using std::experimental::suspend_never;
        using std::experimental::noop_coroutine;
    }
#else
#   error "no coroutine support found"
#endif



namespace jkl{


struct coro_handle_deleter
{
    void operator()(std::coroutine_handle<> c)
    {
        c.destroy();
    }
};

template<class Promise = void>
using unique_coro_handle = unique_handle<
    std::coroutine_handle<Promise>,
    coro_handle_deleter,
    nullptr
>;


// usage:
// auto await_suspend(std::coroutine_handle<>)
// {
//     ...
//     return tail_resume(some_coro);
// }
inline auto tail_resume(std::coroutine_handle<> coro)
{
    return coro ? coro : std::noop_coroutine();
}

template<class Promise>
auto tail_resume(unique_coro_handle<Promise> const& h)
{
    return tail_resume(h.get());
}


} // namespace jkl