#pragma once

#include <jkl/config.hpp>
#include <jkl/task.hpp>
#include <map>
#include <mutex>
#include <optional>
#include <type_traits>


namespace jkl{


template<class T, class CoReturnType = void>
class agen
{
    static_assert(! std::is_void_v<T>);

public:
    class promise_type : public apromise<CoReturnType, agen, promise_type>
    {
    public:
        std::optional<T> _yv;

        auto yield_value(auto&& u) noexcept(noexcept(_yv.emplace(JKL_FORWARD(u))))
        {
            _yv.emplace(JKL_FORWARD(u));

            struct yield_value_awaiter
            {
                bool await_ready() const noexcept { return false; }

                auto await_suspend(std::coroutine_handle<promise_type> c)
                {
                    BOOST_ASSERT(c.promise().continuation());
                    return tail_resume(c.promise().continuation());
                }

                void await_resume() const noexcept {}
            };

            return yield_value_awaiter();
        }
    };

    explicit agen(std::coroutine_handle<promise_type> c) noexcept : _h(c) {}

    promise_type& promise() noexcept { BOOST_ASSERT(_h); return _h->promise(); }

    template<bool ThrowUnhandledException>
    struct next_awaiter
    {
        promise_type& p;

        bool await_ready() const noexcept
        {
            return p.coroutine().done();
        }

        template<class Promise>
        auto await_suspend(std::coroutine_handle<Promise> c) noexcept
        {
            if(! p.continuation()) // first co_await g.next()
                p.set_continuation(c);

            return tail_resume(p.coroutine());
        }

        auto await_resume()
        {
            if constexpr(ThrowUnhandledException)
                p.throw_on_exception();
            return std::move(p._yv);
        }
    };

    template<bool ThrowUnhandledException = true>
    auto next()
    {
        return next_awaiter<ThrowUnhandledException>{promise()};
    }

private:
    unique_coro_handle<promise_type> _h;
};


template<bool Move = false, class Iter>
agen<typename std::iterator_traits<Iter>::value_type> gen_range_elems(Iter beg, Iter end)
{
    for(; beg != end; ++beg)
    {
        if constexpr(Move)
            co_yield std::move(*beg);
        else
            co_yield JKL_FORWARD(*beg);
    }
}

template<bool Move = false, class R>                                                                  // vvv: r must live until returned generator done
agen<typename std::iterator_traits<decltype(std::begin(std::declval<R>()))>::value_type> gen_range_elems(R&& r)
{
    for(auto&& e : JKL_FORWARD(r))
    {
        if constexpr(Move)
            co_yield std::move(e);
        else
            co_yield JKL_FORWARD(e);
    }
}

auto gen_moved_range_elems(auto beg, auto end)
{
    return gen_range_elems<true>(beg, end);
}

auto gen_moved_range_elems(auto&& r)
{
    return gen_range_elems<true>(JKL_FORWARD(r));
}



template<class WhileNextPromise, class GenPromise>
struct while_helper
{
    WhileNextPromise& wp;
    GenPromise& gp;
    std::exception_ptr ex = nullptr;
    unordered_flat_map<atask<>::promise_type*, atask<>> pts;
    bool wpSuspended = false;
    std::mutex mtx;

    while_helper(WhileNextPromise& w, GenPromise& g) : wp{w}, gp{g} {}

    atask<>& emplace(atask<>&& task)
    {
        std::lock_guard lg(mtx);
        BOOST_ASSERT(! pts.contains(& task.promise()));
        return pts.try_emplace(& task.promise(), std::move(task)).first->second;
    }

    auto on_suspend_then(atask<>::promise_type& p)
    {
        bool allDone = false;
        
        {
            std::lock_guard lg(mtx);

            BOOST_ASSERT(pts.contains(&p));
            pts.erase(&p);

            allDone = pts.empty() && wpSuspended;
        }

        //if(allDone)
        //    wp.continuation().resume();

        if(allDone)
        {
            wp.result_var() = std::move(gp.result_var());
            return tail_resume(wp.continuation());
        }

        return std::noop_coroutine();
    }
};


// co_await f(T&&)
// collector(std::exception_ptr from 'co_await f(T&&)' if any, decltype(co_await f(T&&))&& if not void)
template<class T, class CoReturnType>
atask<CoReturnType> while_next(agen<T, CoReturnType> gen, auto f, auto collector)
{
    while_helper wh{co_await get_promise(), gen.promise()};

    bool taskStarted = false; // at least 1 task started
                                           // vvvvv: disable gen.next() to throw unhandled_exception
    while(auto r = co_await gen.template next<false>())
    {
        auto& task = wh.emplace(
            [](auto& wh, auto& f, auto& collector, auto r) mutable -> atask<>
            {
                try
                {
                    using atype = await_result_t<decltype(f(std::move(*r)))>;

                    std::exception_ptr ex = nullptr;
                        
                    if constexpr(is_void_v<atype>)
                    {
                        try
                        {
                            co_await f(std::move(*r));
                        }
                        catch(...)
                        {
                            ex = std::current_exception();
                        }

                        if constexpr(_co_awaitable_<decltype(collector(ex))>)
                            co_await collector(ex);
                        else
                            collector(ex);
                    }
                    else
                    {
                        std::conditional_t<is_aresult_v<atype>, atype, std::optional<atype>> ar;

                        try
                        {
                            ar = co_await f(std::move(*r));
                        }
                        catch(...)
                        {
                            ex = std::current_exception();
                        }

                        if constexpr(_co_awaitable_<decltype(collector(ex, ar))>)
                            co_await collector(ex, std::move(ar));
                        else
                            collector(ex, std::move(ar));
                    }
                }
                catch(std::exception const& e)
                {
                    JKL_WARN << "while_next(): unhandled exception: " << e.what();
                }
                catch(...)
                {
                    JKL_WARN << "while_next(): unhandled unknown exception";
                }

                co_await suspend_awaiter([&](auto c)
                {
                    return wh.on_suspend_then(c.promise());

                    // as we'll destroy the coro later, this lambda and its capture will also be deleted.
                    // so we store these pointers on stack (references may not work, as they are only alias)
                    //auto* wh_ = &wh;
                    //auto* wp_ = &wp;
                });
            }(wh, f, collector, std::move(r))
        );

        task.start(wh.wp.get_stop_source());
        taskStarted = true;
    }

    if(taskStarted)
    {
        {
            std::lock_guard lg(wh.mtx);

            BOOST_ASSERT(! wh.wpSuspended);
                
            if(wh.pts.empty()) // all task already finished, so we should co_return here.
               co_return gen.promise().result(); // may also throw (then be captured by wh.wp.unhandled_exception())

            wh.wpSuspended = true; // signal wh.on_suspend_then() to resume when all task finished
        }

        co_await std::suspend_always(); // continuation will be resumed in wh.on_suspend_then()
    }

    co_return gen.promise().result();
}


template<class T = void>
atask<T> while_next(auto&& gen, auto&& f)
{
    return while_next(JKL_FORWARD(gen), JKL_FORWARD(f), [](auto&&...){});
}


} // namespace jkl
