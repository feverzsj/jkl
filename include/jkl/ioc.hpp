#pragma once


#include <jkl/config.hpp>
#include <jkl/util/log.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <deque>
#include <memory>
#include <thread>
#include <atomic>
#include <optional>
#include <exception>
#include <type_traits>


namespace jkl{


template<class F>
void _run_ioc(asio::io_context& ioc, F&& onExcep)
{
    for(;;)
    {
        try
        {
            ioc.run();
            return;
        }
        catch(...)
        {
            JKL_FORWARD(onExcep)();
        }
        // rejoin io_context
    }
}

inline auto default_ioc_excep_handler(char const* rp)
{
    return [rp](){
        try
        {
            throw;
        }
        catch(std::exception& e)
        {
            JKL_ERR << rp << ": " << e.what();
        }
        catch(...)
        {
            JKL_ERR << rp << ": unknown error";
        }
    };
}


// single io_context runs in / manages a set of threads
class mt_ioc_src
{
protected:
    asio::io_context _ioc;
    std::optional<asio::executor_work_guard<asio::io_context::executor_type>> _wg;
    std::deque<std::thread> _threads;

public:
    ~mt_ioc_src()
    {
        join();
    }

    asio::io_context& get_ioc(size_t /*i*/ = 0) noexcept
    {
        return _ioc;
    }

    template<class F>
    void start(size_t threads, F onExcep)
    {
        BOOST_ASSERT(! _wg);
        BOOST_ASSERT(_threads.empty());
        
        _ioc.restart();
        _wg.emplace(_ioc.get_executor());

        for(;threads-- >0;)
        {
            _threads.emplace_back(
                [this, onExcep]()
                {
                    _run_ioc(_ioc, onExcep);
                }
            );
        }
    }

    void start(size_t threads, char const* rp = "mt_ioc_src")
    {
        start(threads, default_ioc_excep_handler(rp));
    }

    void join()
    {
        if(_wg)
        {
            BOOST_ASSERT(_threads.size());

            _wg.reset();
            for(auto& t : _threads)
                t.join();
            _threads.clear();

            BOOST_ASSERT(_ioc.stopped());
        }
    }
    
    bool stopped() const noexcept
    {
        return _threads.empty();
    }

    // the event loop will exit when the control goes back to io_context.
    // in general, you don't need this. Use task.request_stop() and handle it in your coro.
    void signal_event_loop_stop() { _ioc.stop(); }
};

// NOTE: actual data is still hold by user
template<class PerThreadData>
class mtd_ioc_src : public mt_ioc_src
{
    PerThreadData*& data_ptr() noexcept
    {
        thread_local PerThreadData* d = nullptr;
        return d;
    }

public:
    // only use this inside a handler of this context
    PerThreadData& data() noexcept
    {
        PerThreadData* d = data_ptr();
        BOOST_ASSERT(d);
        return *d;
    }

    template<class R, class F>
    void start(R& datas, F&& onExcep)
    {
        BOOST_ASSERT(_threads.empty());

        for(PerThreadData& d : datas)
        {
            _threads.emplace_back(
                [this, &d, onExcep]()
                 {
                     data_ptr() = &d;
                     _run_ioc(_ioc, onExcep);
                 }
            );
        }
    }

    template<class Range>
    void start(Range& datas, char const* rp = "mtd_ioc_src")
    {
        start(datas, default_ioc_excep_handler(rp));
    }
};


// multiple io_context, each runs in / manages a set of threads
// NOTE: handlers could be dispatched to any thread binded to it's io_context
class ioc_pool
{
    std::vector<mt_ioc_src> _srcs;
    std::atomic_size_t      _next = ATOMIC_VAR_INIT(0);

public:
    explicit ioc_pool(size_t n) : _srcs(n) { BOOST_ASSERT(n >= 1); }

    size_t ioc_cnt() const noexcept { return _srcs.size(); }

    asio::io_context& get_ioc(size_t i) noexcept
    {
        BOOST_ASSERT(i < ioc_cnt());
        return _srcs[i].get_ioc();
    }

    asio::io_context& get_ioc()
    {
        // Use a round-robin scheme to choose the next io_context to use.

        // http://preshing.com/20150402/you-can-do-any-kind-of-atomic-read-modify-write-operation/
        // an atomic fetch_inc_modulus
        auto idx = _next.load(std::memory_order_relaxed);
        while(! _next.compare_exchange_weak(idx, (idx + 1) % _srcs.size(), std::memory_order_relaxed))
            ;

        return _srcs[idx].get_ioc();
    }


    template<class F>
    void start(size_t threads, F onExcep)
    {
        BOOST_ASSERT(threads >= ioc_cnt());

        size_t n = ioc_cnt();
        size_t d = threads / n;
        size_t r = threads % n;

        for(size_t i = 0; i < n; ++i)
        {
            _srcs[i].start(d + (i < r), onExcep);
        }
    }

    void start(size_t threads, char const* rp = "ioc_pool")
    {
        start(threads, default_ioc_excep_handler(rp));
    }

    void join()
    {
        for(auto& src : _srcs)
            src.join();
    }

    bool stopped() const noexcept
    {
        for(auto& src : _srcs)
        {
            if(! src.stopped())
                return false;
        }
        return true;
    }

    void signal_event_loop_stop()
    {
        for(auto& src : _srcs)
            src.signal_event_loop_stop();
    }
};

inline mt_ioc_src g_default_ioc_src;


inline auto& default_ioc_src()
{
    return g_default_ioc_src;
//     static mt_ioc_src src;
//     return src;
}

inline asio::io_context& default_ioc()
{
    return default_ioc_src().get_ioc();
}

inline asio::signal_set g_default_signal_set(
    default_ioc(),
    SIGINT, SIGTERM
#ifdef SIGQUIT
    , SIGQUIT
#endif
);

inline asio::signal_set& default_signal_set()
{
    return g_default_signal_set;
//     static asio::signal_set signals(
//         default_ioc(),
//         SIGINT, SIGTERM
//     #ifdef SIGQUIT
//         , SIGQUIT
//     #endif
//     );
// 
//     return signals;
}


class [[nodiscard]] signal_set_request_stop_guard
{
    asio::signal_set& _s;

public:
    template<class Task>
    explicit signal_set_request_stop_guard(Task& t, asio::signal_set& s = default_signal_set())
        : _s{s}
    {
        _s.async_wait([&t](auto&& ec, int /*signal*/){
            if(! ec)
                t.request_stop();
        });
    }

    ~signal_set_request_stop_guard()
    {
        _s.cancel();
    }
};


} // namespace jkl
