#pragma once

#include <jkl/config.hpp>
#include <jkl/ioc.hpp>
#include <jkl/ec_awaiter.hpp>
#include <jkl/util/unordered_map_set.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <list>
#include <queue>
#include <mutex>
#include <functional>


namespace jkl{


template<class T, bool UnusedClearable = false>
class res_pool
{
    // list/forward_list end iterator won't be invalidated by modfiers except swap().
    // we rely on this to test if it is valid without locking in res_holder.
    using res_list = std::list<T>;
    using res_iter = typename res_list::iterator;

public:
    friend class res_holder;

    class res_holder
    {
        res_pool& _p;
        res_iter  _it;

    public:
        res_holder(res_pool& p) : _p(p), _it(p.invalid_iter()) {}
        res_holder(res_pool& p, res_iter it) : _p(p), _it(it) {}

        ~res_holder() { recycle(); }

        res_holder(res_holder const&) = delete;
        res_holder& operator=(res_holder const&) = delete;

        res_holder(res_holder&& t) noexcept : _p(t._p), _it(t._it) { t._it = _p.invalid_iter(); }

        res_holder& operator=(res_holder&& t) noexcept
        {
            BOOST_ASSERT(&_p == &t._p);
            std::swap(_it, t._it);
            return *this;
        }

        res_holder& operator=(res_iter it) noexcept
        {
            return *this = res_holder(_p, it);
        }

        bool valid() const noexcept { return _it != _p.invalid_iter(); }

        explicit operator bool() const noexcept { return valid(); }

        T& value() noexcept { BOOST_ASSERT(valid()); return *_it; }
        T& operator* () noexcept { return value(); }
        T* operator->() noexcept { return std::addressof(value()); }

        res_pool& pool() noexcept { return _p; }

        void recycle()
        {
            if(valid())
            {
                _p.recycle(_it);
                _it = _p.invalid_iter();
            }
        }
    };

private:
    struct awaiter_base;
    using id_type = std::pair<size_t, awaiter_base**>;

    struct awaiter_base
    {
        id_type     _id;
        res_holder  _rh;
        aerror_code _ec;
        std::coroutine_handle<> _coro;

        awaiter_base(res_pool& p)  : _rh(p) {}

        auto const& get_id() const noexcept { return _id; }
        void set_id(id_type const& id) noexcept { _id = id; }

        // when invoked, this awaiter should have been removed from pool
        void complete(res_iter it)
        {
            BOOST_ASSERT(_coro);
            BOOST_ASSERT(! _ec);
            BOOST_ASSERT(! _rh.valid());

            _rh = it;

            _rh.pool().io_ctx().post([this](){
                _coro.resume();
            });
        }

    };

    template<bool EnableStop, class Dur>
    struct awaiter : awaiter_base
    {
        static constexpr bool has_expiryDur = ! std::is_same_v<Dur, null_op_t>;

        [[no_unique_address]] not_null_if_t<has_expiryDur, asio::steady_timer                                      > _timer;
        [[no_unique_address]] not_null_if_t<EnableStop   , std::optional<std::stop_callback<std::function<void()>>>> _stopCb;

        awaiter(res_pool& p, Dur const& expiryDur)
            : awaiter_base(p), _timer(p.io_ctx(), expiryDur)
        {}

        bool await_ready() const noexcept { return false; }

        template<class Promise>
        bool await_suspend(std::coroutine_handle<Promise> c)
        {
            if constexpr(EnableStop)
            {
                if(c.promise().stop_requested())
                {
                    this->_ec = asio::error::operation_aborted;
                    return false;
                }
            }

            BOOST_ASSERT(! this->_rh.valid());

            this->_rh = this->_rh.pool().try_acquire();

            if(this->_rh.valid())
                return false;

            // if constexpr(has_expiryDur)
            // {
            //     if(_timer.expiry() == asio::steady_timer::clock_type::now())
            //     {
            //         this->_ec = gerrc::timeout;
            //         return false;
            //     }
            // }

            this->_coro = c;

            this->_rh.pool().add_awaiter(this);

            if constexpr(has_expiryDur)
            {
                _timer.async_wait(
                    [this, &p = this->_rh.pool(), wid = this->_id]([[maybe_unused]] auto&& ec)
                    {//    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ must store them, since awaiter may be removed, when the timer handle get called.
                        if(!ec && p.remove_awaiter(wid))
                        {
                            this->_ec = gerrc::timeout;
                            this->_coro.resume();
                        }
                    }
                );
            }

            if constexpr(EnableStop)
            {
                BOOST_ASSERT(! _stopCb);
                _stopCb.emplace(c.promise().get_stop_token(),
                    [this]()
                    {
                        if(this->_rh.pool().remove_awaiter(this->_id))
                        {
                            if constexpr(has_expiryDur)
                                _timer.cancel();

                            this->_ec = asio::error::operation_aborted;
                            this->_coro.resume();
                        }
                    }
                );
            }

            return true;
        }

        aresult<res_holder> await_resume()
        {
            return {this->_ec, std::move(this->_rh)};
        }
    };

    asio::io_context& _ioc;

    std::mutex _mut;

    size_t _cap = 0;
    res_list _unused;
    res_list _inuse;
    std::function<T()> _creator;
    std::function<void(T&)> _onAcquire = [](T&){};

    std::deque<awaiter_base*>   _awaiterQueue;
    unordered_flat_set<id_type> _awaiters;

    size_t _idSeq = 0;

    auto invalid_iter() { return _inuse.end(); }

    void add_awaiter(awaiter_base* w)
    {
        BOOST_ASSERT(w);

        std::lock_guard lg(_mut);

        _awaiterQueue.emplace_back(w);

        id_type id(++_idSeq, &_awaiterQueue.back());

        if(! _awaiters.emplace(id).second)
            throw std::runtime_error("res_pool: id clash");

        w->set_id(id);
    }

    bool remove_awaiter(id_type const& id)
    {
        std::lock_guard lg(_mut);

        if(_awaiters.erase(id))
        {
            * id.second = nullptr;
            return true;
        }
        return false;
    }

    void recycle(res_iter it)
    {
        BOOST_ASSERT(it != invalid_iter());

        awaiter_base* w = nullptr;
        {
            std::lock_guard lg(_mut);

            while(_awaiterQueue.size() && ! _awaiterQueue.front())
                _awaiterQueue.pop_front();

            if(_awaiterQueue.empty())
            {
                _unused.splice(_unused.begin(), _inuse, it);
                return;
            }

            w = _awaiterQueue.front();
            BOOST_VERIFY(_awaiters.erase(w->get_id()));
            _awaiterQueue.pop_front();
        }

        _onAcquire(*it);
        w->complete(it);
    }

    res_holder try_acquire()
    {
        std::lock_guard lg(_mut);

        if(_unused.empty())
        {
            if(_inuse.size() < _cap)
                _unused.emplace_front(_creator());
            else
                return res_holder(*this, invalid_iter());
        }

        _inuse.splice(_inuse.begin(), _unused, _unused.begin());
        _onAcquire(*_inuse.begin());
        return res_holder(*this, _inuse.begin());
    }

public:
    explicit res_pool(size_t cap, asio::io_context& ioc = default_ioc()) requires(std::is_default_constructible_v<T>)
        : res_pool(cap, [](){ return T{}; }, ioc)
    {}

    template<class F>
    explicit res_pool(size_t cap, F&& create, asio::io_context& ioc = default_ioc())
        : _ioc(ioc), _cap(cap), _creator(std::forward<F>(create))
    {}

    ~res_pool()
    {
        BOOST_ASSERT(_inuse.empty() && _awaiterQueue.empty() && _awaiters.empty());
    }

    res_pool(res_pool const&) = delete;
    res_pool& operator=(res_pool const&) = delete;
    res_pool(res_pool&&) = delete;
    res_pool& operator=(res_pool&&) = delete;

    asio::io_context& io_ctx() noexcept { return _ioc; }

    size_t size() const { std::lock_guard lg(_mut); return _unused.size() + _inuse.size; }
    size_t in_use() const { std::lock_guard lg(_mut); return _inuse.size; }
    size_t unused() const { std::lock_guard lg(_mut); return _unused.size; }
    size_t capacity() const { std::lock_guard lg(_mut); return _cap; }

    // whether you can clear unused when running depends on the resource type and your use case.
    void clear_unused() requires(UnusedClearable) { std::lock_guard lg(_mut);  _unused.clear(); }

    // newCap >= size()
    void reserve(size_t n) { std::lock_guard lg(_mut);  if(n >= _unused.size() + _inuse.size) _cap = n; }

    void reserve_by(double ratio, size_t maxCap = SIZE_MAX)
    {
        BOOST_ASSERT(ratio > 0);
        std::lock_guard lg(_mut);
        size_t newCap = std::min(maxCap, boost::numeric_cast<size_t>(
                                 std::floor(boost::numeric_cast<double>(_cap) * ratio)));
        if(newCap >= _unused.size() + _inuse.size())
            _cap = newCap;
    }

    // how to create the resource
    template<class F>
    void set_creator(F&& f)
    {
        std::lock_guard lg(_mut);
        _creator = std::forward<F>(f);
    }

    // do something to the resource when it is acquired
    template<class F>
    void on_acquire(F&& f)
    {
        _onAcquire = std::forward<F>(f);
    }

    void create_all()
    {
        std::lock_guard lg(_mut);

        BOOST_ASSERT(_cap >= _unused.size() + _inuse.size);

        for(size_t n = _cap - _unused.size() - _inuse.size(); n-- > 0;)
            _unused.emplace_front(_creator());
    }

    template<class... P>
    auto acquire(P... p)
    {
        auto params = make_params(p..., p_disable_stop, p_expires_never);
        return awaiter<params(t_stop_enabled), decltype(params(t_expiry_dur))>(*this, params(t_expiry_dur));
    }
};


} // namespace jkl