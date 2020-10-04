#pragma once


#include <jkl/config.hpp>
#include <jkl/params.hpp>
#include <jkl/std_coro.hpp>

#include <boost/smart_ptr/detail/spinlock.hpp>

#include <atomic>


namespace jkl{


struct is_stop_requested{};


class stoppable
{
public:
    // must be thread safe;
    virtual void stop(int lv) = 0;
};

		
class stopper_manager
{
    std::atomic_int _lv = ATOMIC_VAR_INIT(stop_lv_none);

    // TODO: use C++20 atomic<weak_ptr>
    std::weak_ptr<stoppable> _stp;
    boost::detail::spinlock _l;

public:
    template<class T>
    void set_stopper(std::shared_ptr<T> const& s) noexcept
    {
        boost::detail::spinlock::scoped_lock lg(_l);
        _stp = s;
    }

    void stop(int lv)
    {
        BOOST_ASSERT(lv > stop_lv_none);

        _lv.store(lv, std::memory_order_relaxed);

        shared_ptr<stoppable> s;
        {
            boost::detail::spinlock::scoped_lock lg(_l);
            s = _stp.lock();
        }
        if(s)
            s->stop(lv);
    }

    void request_stop() { stop(stop_lv_requested); }
    void force_stop  () { stop(stop_lv_forced  ); }

    auto stop_lv() const noexcept
    {
        return _lv.load(std::memory_order_relaxed);
    }

    bool stop_requested() const noexcept { return stop_lv() > stop_lv_none; }
    bool stop_forced   () const noexcept { return stop_lv() > stop_lv_requested; }
};


} // namespace jkl