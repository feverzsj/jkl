#pragma once

#include <jkl/config.hpp>
#include <jkl/traits.hpp>
#include <jkl/params.hpp>
#include <jkl/result.hpp>
#include <jkl/std_coro.hpp>
#include <jkl/std_stop_token.hpp>
#include <mutex>
#include <variant>
#include <exception>
#include <condition_variable>
#if !defined(JKL_DISABLE_CORO_RECYCLING)
#include <boost/asio/awaitable.hpp>
#endif


namespace jkl{


template<class F>
struct direct_return_awaiter
{
    F f;

    explicit direct_return_awaiter(auto&& u): f{JKL_FORWARD(u)} {}

    bool await_ready() const noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    [[nodiscard]] decltype(auto) await_resume() { return f(); }
};

template<class F>
direct_return_awaiter(F) -> direct_return_awaiter<F>;


template<class F>
struct suspend_awaiter
{
    F f;

    explicit suspend_awaiter(auto&& u): f{JKL_FORWARD(u)} {}

    bool await_ready() const noexcept { return false; }

    template<class P>
    decltype(auto) await_suspend(std::coroutine_handle<P> c)
    {
        return f(c);
    }

    void await_resume() const noexcept {}
};

template<class F>
suspend_awaiter(F) -> suspend_awaiter<F>;


struct get_promise{};

struct get_stop_token_t{};
inline get_stop_token_t get_stop_token() noexcept { return {}; }
struct stop_requested_t{};
inline stop_requested_t stop_requested() noexcept { return {}; }

template<class CB>
struct create_stop_callback
{
    CB& cb;
    template<class F>
    create_stop_callback(F&& f) : cb(f) {}
};

template<class CB>
create_stop_callback(CB) -> create_stop_callback<CB>;



template<class T, class Derived>
class apromise_return
{
public:
    void return_value(auto&& v) noexcept(noexcept(static_cast<Derived*>(this)->_rv.template emplace<2>(JKL_FORWARD(v))))
    {
        static_cast<Derived*>(this)->_rv.template emplace<2>(JKL_FORWARD(v));
    }
};

template<class T, class Derived>
class apromise_return<T&, Derived>
{
public:
    void return_value(T& v) noexcept
    {
        static_cast<Derived*>(this)->_rv.emplace<2>(&v);
    }
};

template<class Derived>
class apromise_return<void, Derived>
{
public:
    void return_void() noexcept {}
};

template<class T = void>
class [[nodiscard]] atask;

template<class T, class Ret, class Derived = void>
class apromise : public apromise_return<T, apromise<T, Ret, Derived>>
{
    static_assert(std::is_void_v<T> || std::is_lvalue_reference_v<T> || ! std::is_reference_v<T>);

    friend class apromise_return<T, apromise<T, Ret, Derived>>;
    friend class atask<T>;

    using result_variant = std::variant<std::monostate,
                                        std::exception_ptr,
                                        std::conditional_t<(std::is_void_v<T> || std::is_lvalue_reference_v<T>), T*, T>>;
    result_variant _rv;
    std::coroutine_handle<> _continuation = nullptr;
    std::stop_source _stpSrc{std::nostopstate};

    using dpromise = std::conditional_t<std::is_void_v<Derived>, apromise, Derived>;

protected:
    void set_stop_source(auto&& u) noexcept
    {
        _stpSrc = JKL_FORWARD(u);
    }

public:
    std::stop_source const& get_stop_source() const noexcept { return _stpSrc; }
    std::stop_token get_stop_token() const noexcept { return _stpSrc.get_token(); }
    bool stop_requested() const noexcept { return _stpSrc.stop_requested(); }
    bool request_stop() noexcept { return _stpSrc.request_stop(); }

    auto coroutine() noexcept { return std::coroutine_handle<dpromise>::from_promise(static_cast<dpromise&>(*this)); }

    template<class UP>
    void set_continuation(std::coroutine_handle<UP> c) noexcept
    {
        BOOST_ASSERT(! _continuation && ! _stpSrc.stop_possible());

        _continuation = c;
        _stpSrc = c.promise().get_stop_source();
    }

    auto continuation() const noexcept { return _continuation; }
    bool has_continuation() const noexcept { return _continuation.operator bool(); }

    auto await_transform(get_promise) noexcept
    {
        return direct_return_awaiter([&]() mutable -> auto& { return static_cast<dpromise&>(*this); });
    }

    auto await_transform(get_stop_token_t) noexcept
    {
        return direct_return_awaiter([&](){ return _stpSrc.get_token(); });
    }
    
    auto await_transform(stop_requested_t) noexcept
    {
        return direct_return_awaiter([&](){ return _stpSrc.stop_requested(); });
    }

    template<class CB>
    auto await_transform(create_stop_callback<CB> const& t) noexcept
    {
        return direct_return_awaiter([&](){ return std::stop_callback(_stpSrc.get_token(), t.cb); });
    }

    auto&& await_transform(auto&& awaitable) noexcept
    {
        return JKL_FORWARD(awaitable);
    }


    Ret get_return_object() noexcept { return Ret{coroutine()}; }

    std::suspend_always initial_suspend() noexcept { return {}; }

    auto final_suspend() noexcept
    {
        struct final_suspend_awaiter
        {
            bool await_ready() const noexcept { return false; }

            auto await_suspend(std::coroutine_handle<dpromise> c) noexcept
            {
                return tail_resume(c.promise().continuation());
            }

            void await_resume() noexcept {} // never reach this
        };

        return final_suspend_awaiter{};
    }

    void unhandled_exception() noexcept
    {
        _rv.template emplace<1>(std::current_exception());
    }

    void throw_on_exception()
    {
        if(_rv.index() == 1)
            std::rethrow_exception(std::get<1>(_rv));
    }

    // should be only called once.
    T result()
    {
        throw_on_exception();

        BOOST_ASSERT(_rv.index() == 2);

        if constexpr(std::is_void_v<T>)
            return;
        else if constexpr(std::is_lvalue_reference_v<T>)
            return *std::get<2>(_rv);
        else
            return std::move(std::get<2>(_rv));
    }

    result_variant& result_var() { return _rv; }

#if !defined(JKL_DISABLE_CORO_RECYCLING)
    void* operator new(size_t size)
    {
        return boost::asio::detail::thread_info_base::allocate(
            boost::asio::detail::thread_info_base::awaitable_frame_tag(),
            boost::asio::detail::thread_context::thread_call_stack::top(),
            size);
    }

    void operator delete(void* pointer, size_t size)
    {
        boost::asio::detail::thread_info_base::deallocate(
            boost::asio::detail::thread_info_base::awaitable_frame_tag(),
            boost::asio::detail::thread_context::thread_call_stack::top(),
            pointer, size);
    }
#endif // JKL_DISABLE_CORO_RECYCLING
};




// When you call a coroutine that returns a atask, the coroutine
// simply captures any passed parameters and returns execution to the
// caller. Execution of the coroutine body does not start until the
// coroutine is first co_await'ed.
template<class T>
class atask
{
public:
    using promise_type = apromise<T, atask>;

private:
    unique_coro_handle<promise_type> _h;

public:
    atask() = default;

    explicit atask(std::coroutine_handle<promise_type> c) noexcept : _h(c) {}

    std::coroutine_handle<promise_type> coroutine() const noexcept { return _h.get(); }
    promise_type& promise() noexcept { BOOST_ASSERT(_h); return _h->promise(); }
    promise_type const& promise() const noexcept { BOOST_ASSERT(_h); return _h->promise(); }

    std::stop_source const& get_stop_source() noexcept { return promise().get_stop_source(); }
    std::stop_token get_stop_token() const noexcept { return promise().get_stop_token(); }
    bool stop_requested() const noexcept { return promise().stop_requested(); }
    bool request_stop() noexcept { return promise().request_stop(); }
    bool done() const noexcept { return _h->done(); }

    void start(std::stop_source const& s = std::stop_source{}) // pass in std::nostopstate to fully disable stop
    {
        BOOST_ASSERT(_h);
        BOOST_ASSERT(! promise().get_stop_source().stop_possible());

        promise().set_stop_source(s);

        _h->resume();
    }

    decltype(auto) start_join(std::stop_source const& s = std::stop_source{});

    bool await_ready() const noexcept { return false; }

    template<class Promise>
    auto await_suspend(std::coroutine_handle<Promise> c) noexcept
    {
        promise().set_continuation(c);
        return tail_resume(_h); // once co_awaited, immediately resume the coro.
    }

    decltype(auto) await_resume()
    {
        return promise().result();
    }
};


template<class Awaiter>
struct skip_await_resume
{
    Awaiter& awaiter;

    explicit skip_await_resume(Awaiter& a) : awaiter{a} {}

    bool await_ready() noexcept(noexcept(awaiter.await_ready()))
    {
        return awaiter.await_ready();
    }

    template<class Promise>
    auto await_suspend(std::coroutine_handle<Promise> c) noexcept(noexcept(awaiter.await_suspend(c)))
    {
        return awaiter.await_suspend(c);
    }

    void await_resume() const noexcept {}
};


decltype(auto) sync_await(_awaiter_ auto&& a, std::stop_source const& s = std::stop_source{std::nostopstate})
{
    std::mutex m;
    std::condition_variable c;
    bool done = false;

    auto task = [&]()->atask<void>
    {
        co_await skip_await_resume{a};
        {
            std::lock_guard lg{m};
            done = true;
        }
        c.notify_one();
    }();

    task.start(s);

    {
        std::unique_lock lk(m);
        while(! done)
            c.wait(lk);
    }

    return a.await_resume();
}


template<class T>
decltype(auto) atask<T>::start_join(std::stop_source const& s)
{
    return sync_await(*this, s);
}


template<class T = void>
using aresult_task = atask<aresult<T>>;



// post: submit the handle for later execution. The handle may be executed before the caller returns(if there is other
//       work after ex.post() in caller). The thread pool implementation usually require a lock for enqueue the job
//       queue and a condition_variable notify one thread to wake up to process the handle.
//       
// defer: submit the handle for later execution(least eager). If the caller's executor and handle's executor are same,
//        the handle will be executed after the caller returned, otherwise do a post. The thread pool implementation
//        usually just enqueue the handle to a thread_local queue if caller and handle are on same pool. When control
//        returns to the thread pool(i.e. the caller returns), copy the thread_local queue to main queue, and schedule
//        the handle to free thread in pool. 
// 
// dispatch: if the executor's rules allow it(e.g.: executors are same), run the handle immediately in the calling thread;
//           otherwise post.
// ref:
//      https://chriskohlhoff.github.io/executors/
//      http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0113r0.html
//      http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4242.html


// appearntly, only post() is meaningful for coroutine when you want to schedule the coroutine to resume on another executor,
// while defer() and dispatch() should be used with callbacks.


// schedule surrounded coroutine on the executor or execution context
auto schedule_on(auto&& exc)
{
    return suspend_awaiter([ex = get_executor(JKL_FORWARD(exc))](auto c)
                           {
                               boost::asio::post(ex, [c](){ c.resume(); });
                           });
}

auto schedule_on(auto&& exc, _co_awaitable_ auto&& a) 
{
    return
        [](auto ex, auto a) -> atask<await_result_t<decltype(a)>>
        {
            co_await schedule_on(ex);
            co_return co_await a;
        }
        (get_executor(JKL_FORWARD(exc)), JKL_FORWARD(a));
}


} // namespace jkl