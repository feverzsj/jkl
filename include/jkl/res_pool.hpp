#pragma once

#include <jkl/config.hpp>
#include <jkl/ioc.hpp>
#include <jkl/error.hpp>
#include <jkl/result.hpp>
#include <jkl/params.hpp>
#include <jkl/std_coro.hpp>
#include <jkl/std_stop_token.hpp>
#include <jkl/variant_awaiter.hpp>
#include <jkl/util/unordered_map_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <list>
#include <queue>
#include <mutex>
#include <functional>


namespace jkl{


template<bool Cond, class Mutex = std::mutex>
using conditional_lock_guard = std::conditional_t<Cond, std::lock_guard<Mutex>, null_op_t>;


enum res_pool_map_lock_policy
{
    res_pool_map_single_lock,
    res_pool_map_lock_per_pool
};


template<class T, res_pool_map_lock_policy LockPolicy = res_pool_map_lock_per_pool>
class res_pool
{
    static constexpr bool has_res = ! std::is_void_v<T>;
    static constexpr bool lock_outside = (LockPolicy != res_pool_map_lock_per_pool);

    // list/forward_list end iterator won't be invalidated by modfiers except swap().
    // we rely on this to test if it is valid without locking in res_holder.
    JKL_LAZY_COND_TYPEDEF(list_t  , has_res, std::list<T>             , null_op_t);
    JKL_LAZY_COND_TYPEDEF(res_iter, has_res, typename list_t::iterator, size_t   );

public:
    friend class res_holder;

    class res_holder
    {
        res_pool* _p = nullptr;
        res_iter  _it = {}; // 0 when res_iter is size_t

    public:
        res_holder() = default;
        res_holder(res_pool& p, res_iter it) : _p{&p}, _it{it} {}

        ~res_holder() { recycle(); }

        res_holder(res_holder const&) = delete;
        res_holder& operator=(res_holder const&) = delete;

        res_holder(res_holder&& t) noexcept : _p{t._p}, _it{t._it} { t._it = _p->invalid_iter(); }

        res_holder& operator=(res_holder&& t) noexcept
        {
            std::swap(_p, t._p);
            std::swap(_it, t._it);
            return *this;
        }

        bool valid() const noexcept { return _p && _it != _p->invalid_iter(); }

        explicit operator bool() const noexcept { return valid(); }

        auto& value     () noexcept requires(has_res) { BOOST_ASSERT(valid()); return *_it; }
        auto& operator* () noexcept requires(has_res) { return value(); }
        auto* operator->() noexcept requires(has_res) { return std::addressof(value()); }

        res_pool& pool() noexcept { BOOST_ASSERT(_p); return *_p; }

        void recycle()
        {
            if(valid())
            {
                _p->recycle(_it);
                _it = _p->invalid_iter();
                _p  = nullptr;
            }
        }
    };

private:
    struct awaiter_base;
    using id_type = std::pair<size_t, awaiter_base**>;

    struct awaiter_base
    {
        res_pool&   _pool;
        id_type     _id;
        res_holder  _rh;
        aerror_code _ec;
        std::coroutine_handle<> _coro;

        explicit awaiter_base(res_pool& p)  : _pool{p} {}

        auto const& get_id() const noexcept { return _id; }
        void set_id(id_type const& id) noexcept { _id = id; }

        // when invoked, this awaiter should have been removed from pool
        void complete(res_iter it)
        {
            BOOST_ASSERT(_coro);
            BOOST_ASSERT(! _ec);
            BOOST_ASSERT(! _rh.valid());

            _rh = {_pool, it};

            _pool.io_ctx().post([this](){
                _coro.resume();
            });
        }
    };

    template<bool EnableStop, class Dur>
    struct awaiter : awaiter_base
    {
        static constexpr bool has_expiry_dur = ! std::is_same_v<Dur, null_op_t>;

        std::unique_lock<std::mutex> _lk;

        JKL_DEF_MEMBER_IF(has_expiry_dur, asio::steady_timer      , _timer );
        JKL_DEF_MEMBER_IF(EnableStop    , optional_stop_callback<>, _stopCb);

        awaiter(std::unique_lock<std::mutex>&& lk, res_pool& p, Dur const& expiryDur)
            : awaiter_base{p}, _lk{std::move(lk)}, _timer{p.io_ctx(), expiryDur}
        {}

        bool await_ready() const noexcept { return false; }

        template<class Promise>
        bool await_suspend(std::coroutine_handle<Promise> c)
        {
            {
                std::unique_lock<std::mutex> lk{std::move(_lk)};

                if constexpr(EnableStop)
                {
                    if(c.promise().stop_requested())
                    {
                        this->_ec = asio::error::operation_aborted;
                        return false;
                    }
                }

                BOOST_ASSERT(! this->_rh.valid());

                this->_rh = this->_pool.template try_acquire<false>();

                if(this->_rh.valid())
                    return false;

                // if constexpr(has_expiry_dur)
                // {
                //     if(_timer.expiry() == asio::steady_timer::clock_type::now())
                //     {
                //         this->_ec = gerrc::timeout;
                //         return false;
                //     }
                // }

                this->_coro = c;

                this->_pool.template add_awaiter<false>(this);
            }

            if constexpr(has_expiry_dur)
            {
                _timer.async_wait(
                    [this, &p = this->_pool, wid = this->_id]([[maybe_unused]] auto&& ec)
                    {//    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ must store them, since awaiter may be removed, when the timer handle get called, which means pool must outlive timeout callback
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
                        if(this->_pool.remove_awaiter(this->_id))
                        {
                            if constexpr(has_expiry_dur)
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


    std::conditional_t<lock_outside, std::mutex&, std::mutex> _mut;

    size_t _cap = 0;
    JKL_DEF_MEMBER_IF(! has_res, size_t, _size) = 0; // size of in use
    JKL_DEF_MEMBER_IF(  has_res, list_t, _inuse  );
    JKL_DEF_MEMBER_IF(  has_res, list_t, _unused );

    struct emplace_res_to_unused
    {
        res_pool& p;
        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto& operator()(auto&&... args) requires(has_res)
        {
            return p._unused.emplace_front(JKL_FORWARD(args)...);
        }
    };

    JKL_LAZY_DEF_MEMBER_IF(has_res, std::function<void(emplace_res_to_unused)>, _creator);
    JKL_LAZY_DEF_MEMBER_IF(has_res, std::function<void(T&)>, _onRecycle)= [](auto&){};

    // awaiters waiting for res
    std::deque<awaiter_base*>   _awaiterQueue;
    unordered_flat_set<id_type> _awaiters;

    size_t _idSeq = 0;

    auto invalid_iter()
    {
        if constexpr(has_res)
            return _inuse.end();
        else
            return static_cast<size_t>(0);
    }

    template<bool Lock>
    void add_awaiter(awaiter_base* w)
    {
        BOOST_ASSERT(w);

        conditional_lock_guard<Lock> lg{_mut};

        _awaiterQueue.emplace_back(w);

        id_type id{++_idSeq, &_awaiterQueue.back()};

        if(! _awaiters.emplace(id).second)
            throw std::runtime_error("res_pool: id clash");

        w->set_id(id);
    }

    bool remove_awaiter(id_type const& id)
    {
        std::lock_guard lg{_mut};

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

        if constexpr(has_res)
            _onRecycle(*it);

        awaiter_base* w = nullptr;
        {
            std::lock_guard lg{_mut};

            while(_awaiterQueue.size() && ! _awaiterQueue.front())
                _awaiterQueue.pop_front();

            if(_awaiterQueue.empty())
            {
                if constexpr(has_res)
                {
                    _unused.splice(_unused.begin(), _inuse, it);
                }
                else
                {
                    BOOST_ASSERT(_size > 0);
                    --_size;
                }
                return;
            }

            w = _awaiterQueue.front();
            BOOST_VERIFY(_awaiters.erase(w->get_id()));
            _awaiterQueue.pop_front();
        }

        //if constexpr(has_res)
        //    _onAcq(*it);
        w->complete(it);
    }

    size_t occupied_cap_nolock() const noexcept
    {
        if constexpr(has_res)
            return _unused.size() + _inuse.size();
        else
            return _size;
    }

public:
    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    explicit res_pool(std::mutex& m, size_t cap, auto&& create, auto&& onRecycle, asio::io_context& ioc = default_ioc()) requires(lock_outside)
        : _ioc{ioc}, _mut{m}, _cap{cap}, _creator{JKL_FORWARD(create)}, _onRecycle{JKL_FORWARD(onRecycle)}
    {
        BOOST_ASSERT(_cap > 0);
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    explicit res_pool(std::mutex& m, size_t cap, auto&& create, asio::io_context& ioc = default_ioc()) requires(lock_outside)
        : _ioc{ioc}, _mut{m}, _cap{cap}, _creator{JKL_FORWARD(create)}
    {
        BOOST_ASSERT(_cap > 0);
    }

    explicit res_pool(std::mutex& m, size_t cap, asio::io_context& ioc = default_ioc()) requires(lock_outside && has_res && std::is_default_constructible_v<T>)
        : res_pool{m, cap, [](auto&& emplace){ emplace(T{}); }, ioc}
    {}

    explicit res_pool(std::mutex& m, size_t cap, asio::io_context& ioc = default_ioc()) requires(lock_outside && ! has_res)
        : _ioc{ioc}, _mut{m}, _cap{cap}
    {
        BOOST_ASSERT(_cap > 0);
    }

    ///
    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    explicit res_pool(size_t cap, auto&& create, auto&& onRecycle, asio::io_context& ioc = default_ioc()) requires(! lock_outside)
        : _ioc{ioc}, _cap{cap}, _creator{JKL_FORWARD(create)}, _onRecycle{JKL_FORWARD(onRecycle)}
    {
        BOOST_ASSERT(_cap > 0);
    }

    template<class F> // don't use auto, clang-concepts bug
    explicit res_pool(size_t cap, F&& create, asio::io_context& ioc = default_ioc()) requires(! lock_outside)
        : _ioc{ioc}, _cap{cap}, _creator{JKL_FORWARD(create)}
    {
        BOOST_ASSERT(_cap > 0);
    }

    explicit res_pool(size_t cap, asio::io_context& ioc = default_ioc()) requires(! lock_outside && has_res && std::is_default_constructible_v<T>)
        : res_pool{cap, [](auto&& add){ add(T{}); }, ioc}
    {}

    explicit res_pool(size_t cap, asio::io_context& ioc = default_ioc()) requires(! lock_outside && ! has_res)
        : _ioc{ioc}, _cap{cap}
    {
        BOOST_ASSERT(_cap > 0);
    }


    ~res_pool()
    {
        if constexpr(has_res)
            BOOST_ASSERT(_inuse.empty() && _awaiterQueue.empty() && _awaiters.empty());
        else
            BOOST_ASSERT(_size == 0 && _awaiterQueue.empty() && _awaiters.empty());
    }

    res_pool(res_pool const&) = delete;
    res_pool& operator=(res_pool const&) = delete;
    res_pool(res_pool&&) = delete;
    res_pool& operator=(res_pool&&) = delete;

    asio::io_context& io_ctx() noexcept { return _ioc; }

    size_t created() const requires(has_res)
    {
        std::lock_guard lg{_mut};
        return _unused.size() + _inuse.size();
    }

    size_t in_use() const
    {
        std::lock_guard lg{_mut};

        if constexpr(has_res)
            return _inuse.size;
        else
            return _size;
    }

    size_t unused() const
    {
        std::lock_guard lg{_mut};

        if constexpr(has_res)
            return _unused.size();
        else
            return _cap - _size;
    }

    size_t capacity() const
    {
        std::lock_guard lg{_mut};
        return _cap;
    }

    // whether you can clear unused when running depends on the resource type and your use case.
    void clear_unused() requires(has_res)
    {
        std::lock_guard lg{_mut};
        _unused.clear();
    }

    void reserve(size_t n)
    {
        std::lock_guard lg{_mut};
        if(n > occupied_cap_nolock())
            _cap = n;
    }

    void reserve_by(double ratio, size_t maxCap = SIZE_MAX)
    {
        BOOST_ASSERT(ratio > 0);
        std::lock_guard lg{_mut};
        size_t newCap = std::min(maxCap, boost::numeric_cast<size_t>(
                                 std::floor(boost::numeric_cast<double>(_cap) * ratio)));

        if(newCap > occupied_cap_nolock())
            _cap = newCap;
    }

    // how to create the resource
    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    void set_creator(auto&& f) requires(has_res)
    {
        std::lock_guard lg{_mut};
        _creator = JKL_FORWARD(f);
    }

    // do something to the resource when it is about to be recycled
    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    void on_recycle(auto&& f) requires(has_res)
    {
        std::lock_guard lg{_mut};
        _onRecycle = JKL_FORWARD(f);
    }

    void create_all() requires(has_res)
    {
        std::lock_guard lg{_mut};

        BOOST_ASSERT(_cap >= occupied_cap_nolock());

        for(size_t n = _cap - occupied_cap_nolock(); n-- > 0;)
            _creator(emplace_res_to_unused{*this});
    }

    template<bool Lock = true>
    res_holder try_acquire()
    {
        conditional_lock_guard<Lock> lg{_mut};

        if constexpr(has_res)
        {
            if(_unused.empty())
            {
                if(_inuse.size() < _cap)
                    _creator(emplace_res_to_unused{*this});
                else
                    return {};
            }

            _inuse.splice(_inuse.begin(), _unused, _unused.begin());
            //_onAcq(*_inuse.begin());
            return {*this, _inuse.begin()};
        }
        else
        {
            if(_size < _cap)
                return {*this, ++_size};
            return {};
        }
    }

    // NOTE: the returned awaiter owns(locks) a mutex, you should co_await or destory it ASAP to release the mutex

    template<class... P>
    auto acquire(std::unique_lock<std::mutex>&& lk, P... p)
    {
        BOOST_ASSERT(lk.owns_lock()); // owns means lk.lock()/try_lock() were called successfully or lk was constructed via {mutex, std::adopt_lock};
        auto params = make_params(p..., p_disable_stop, p_expires_never);
        return awaiter<params(t_stop_enabled), decltype(params(t_expiry_dur))>{std::move(lk), *this, params(t_expiry_dur)};
    }

    template<class... P>
    auto acquire(P... p)
    {
        return acquire(std::unique_lock{_mut}, p...);
    }
};



template<class Key, class T, bool AutoAddPool = true, res_pool_map_lock_policy LockPolicy = res_pool_map_single_lock>
class res_pool_map
{
public:
    using key_type = Key;
    using pool_type = res_pool<T, LockPolicy>;
    using res_holder = typename pool_type::res_holder;

private:
    static constexpr bool has_res = ! std::is_void_v<T>;
    static constexpr bool single_lock = (LockPolicy == res_pool_map_single_lock);

    std::mutex _mut;

    unordered_node_map<Key, pool_type> _pools;
    asio::io_context* _ioc = nullptr;
    size_t _cap = 0;
    JKL_LAZY_DEF_MEMBER_IF(has_res, std::function<T()>     , _creator  );
    JKL_LAZY_DEF_MEMBER_IF(has_res, std::function<void(T&)>, _onRecycle) = [](auto&){};

public:
    explicit res_pool_map(size_t cap, asio::io_context& ioc = default_ioc()) requires(! has_res)
        : _ioc{&ioc}, _cap{cap}
    {
        BOOST_ASSERT(_cap > 0);
    }

    explicit res_pool_map(size_t cap, asio::io_context& ioc = default_ioc()) requires(has_res && std::is_default_constructible_v<T>)
        : res_pool_map{cap, [](auto&& add){ add(T{}); }, ioc}
    {}

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    explicit res_pool_map(size_t cap, auto&& create, asio::io_context& ioc = default_ioc()) requires(has_res)
        : _ioc{&ioc}, _cap{cap}, _creator{JKL_FORWARD(create)}
    {
        BOOST_ASSERT(_cap > 0);
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    explicit res_pool_map(size_t cap, auto&& create, auto&& onRecycle, asio::io_context& ioc = default_ioc()) requires(has_res)
        : _ioc{&ioc}, _cap{cap}, _creator{JKL_FORWARD(create)}, _onRecycle{JKL_FORWARD(onRecycle)}
    {
        BOOST_ASSERT(_cap > 0);
    }

    void set_default_capacity(size_t n)
    {
        std::lock_guard lg{_mut};
        _cap = n;
    }

    void set_default_io_ctx(asio::io_context& ioc)
    {
        std::lock_guard lg{_mut};
        _ioc = &ioc;
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    void set_default_creator(auto&& f) requires(has_res)
    {
        std::lock_guard lg{_mut};
        _creator = JKL_FORWARD(f);
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    void set_default_on_recycle(auto&& f) requires(has_res)
    {
        std::lock_guard lg{_mut};
        _onRecycle = JKL_FORWARD(f);
    }

    template<bool Lock = true>
    pool_type* get_pool(auto const& k)
    {
        conditional_lock_guard<Lock> lg{_mut};

        if(auto it = _pools.find(k); it != _pools.end())
            return & it->second;
        return nullptr;
    }

    template<bool Lock = true>
    std::pair<pool_type*, bool> add_pool(auto&& k, size_t cap, auto&& create, auto&& onRecycle, asio::io_context& ioc)
    {
        conditional_lock_guard<Lock> lg{_mut};

        if constexpr(single_lock)
        {
            auto[it, ok] = _pools.try_emplace(JKL_FORWARD(k), _mut, cap, JKL_FORWARD(create), JKL_FORWARD(onRecycle), ioc);
            return {& it->second, ok};
        }
        else
        {
            auto[it, ok] = _pools.try_emplace(JKL_FORWARD(k), cap, JKL_FORWARD(create), JKL_FORWARD(onRecycle), ioc);
            return {& it->second, ok};
        }
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    std::pair<pool_type*, bool> add_pool(auto&& k, size_t cap, auto&& create, auto&& onRecycle)
    {
        return add_pool(JKL_FORWARD(k), cap, create, onRecycle, *_ioc);
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    std::pair<pool_type*, bool> add_pool(auto&& k, size_t cap, auto&& create)
    {
        return add_pool(JKL_FORWARD(k), cap, create, _onRecycle, *_ioc);
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    std::pair<pool_type*, bool> add_pool(auto&& k, size_t cap)
    {
        return add_pool(JKL_FORWARD(k), cap, _creator, _onRecycle, *_ioc);
    }

    template<bool Lock = true>
    std::pair<pool_type*, bool> add_pool(auto&& k)
    {
        return add_pool<Lock>(JKL_FORWARD(k), _cap, _creator, _onRecycle, *_ioc);
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    res_holder try_acquire(auto&& k)
    {
        std::unique_lock lk{_mut};

        pool_type* p = get_pool<false>(k);

        if(! p)
        {
            if constexpr(AutoAddPool)
                p = add_pool<false>(JKL_FORWARD(k)).first;
            else
                return {};
        }

        if constexpr(LockPolicy == res_pool_map_single_lock)
        {
            return p->template try_acquire<false>();
        }
        else if constexpr(LockPolicy == res_pool_map_lock_per_pool)
        {
            lk.unlock();
            return p->try_acquire();
        }
        else
        {
            JKL_DEPENDENT_FAIL(decltype(k), "unsupported LockPolicy");
        }

    }


    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    auto acquire(auto&& k, auto... p)
    {
        struct not_found_awaiter
        {
            bool await_ready() const noexcept { return true; }
            bool await_suspend(std::coroutine_handle<>) const noexcept { return false; }
            aresult<res_holder> await_resume() { return gerrc::not_found; }
        };

        std::unique_lock lk{_mut};

        pool_type* pool = get_pool<false>(k);

        using awaiter = variant_awaiter<not_found_awaiter, decltype(pool->acquire(p...))>;

        if(! pool)
        {
            if constexpr(AutoAddPool)
                pool = add_pool<false>(JKL_FORWARD(k)).first;
            else
                return awaiter{};
        }

        if constexpr(LockPolicy == res_pool_map_single_lock)
        {
            return awaiter{pool->acquire(std::move(lk), p...)};
        }
        else if constexpr(LockPolicy == res_pool_map_lock_per_pool)
        {
            lk.unlock();
            return awaiter{pool->acquire(p...)};
        }
        else
        {
            JKL_DEPENDENT_FAIL(decltype(k), "unsupported LockPolicy");
        }
    }
};


} // namespace jkl
