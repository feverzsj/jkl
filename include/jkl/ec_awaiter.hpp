#pragma once

#include <jkl/config.hpp>
#include <jkl/error.hpp>
#include <jkl/params.hpp>
#include <jkl/result.hpp>
#include <jkl/std_coro.hpp>
#include <jkl/std_stop_token.hpp>
#include <jkl/util/type_traits.hpp>
#include <boost/asio/steady_timer.hpp>
#include <atomic>
#include <memory>
#include <optional>
#include <type_traits>


namespace jkl{


template<class T, class AsyncObj, class InitOp, class CompOp, bool EnableStop, class Dur, bool PreAssign = false>
struct ec_awaiter
{
    static constexpr bool has_expiryDur = ! std::is_same_v<Dur, null_op_t>;

    AsyncObj&   _ao;
    aerror_code _ec;
    std::coroutine_handle<> _coro;

    [[no_unique_address]] InitOp _initOp;
    [[no_unique_address]] CompOp _compOp; // only called if no error
    [[no_unique_address]] not_null_if_t<  has_expiryDur    , asio::steady_timer                                      > _timer;
    [[no_unique_address]] not_null_if_t<! std::is_void_v<T>, std::conditional_t<PreAssign, T, std::optional<T>>      > _t;
    // NOTE: stop_callback assures when the assigned callback is called, it will not be destructed until the callback returns.
    //       so all the members defined before stop_callback will still be alive inside the assigned callback.
    [[no_unique_address]] not_null_if_t<  EnableStop       , std::optional<std::stop_callback<std::function<void()>>>> _stopCb;


    ec_awaiter(AsyncObj& ao, auto&& i, auto&& c, Dur const& expiryDur)
        : _ao{ao}, _initOp{JKL_FORWARD(i)}, _compOp{JKL_FORWARD(c)}, _timer{get_executor(ao), expiryDur}
    {}

    ec_awaiter(auto&& t, AsyncObj& ao, auto&& i, auto&& c, Dur const& expiryDur) requires(PreAssign)
        : _ao{ao}, _initOp{JKL_FORWARD(i)}, _compOp{JKL_FORWARD(c)}, _timer{get_executor(ao), expiryDur},
          _t{JKL_FORWARD(t)}
    {}

    template<class U>
    void assign_t(U&& u)
    {
        if constexpr(! PreAssign && ! std::is_void_v<T>)
            _t.emplace(std::forward<U>(u));
    }

    void assign_t() noexcept {}


    bool await_ready() const noexcept { return false; }

    template<class Promise>
    bool await_suspend(std::coroutine_handle<Promise> c)
    {
        if constexpr(EnableStop)
        {
            if(c.promise().stop_requested())
            {
                _ec = asio::error::operation_aborted;
                return false;
            }
        }

        // if constexpr(has_expiryDur)
        // {
        //     if(_timer.expiry() == asio::steady_timer::clock_type::now())
        //     {
        //         _ec = gerrc::timeout;
        //         return false;
        //     }
        // }

        _coro = c;

        [[maybe_unused]] std::shared_ptr<std::atomic_flag> anyReached;

        if constexpr(has_expiryDur)
            anyReached = std::make_shared<std::atomic_flag>();

        _initOp(
            [this, anyReached](auto const& ec, auto&&... r) mutable
            {
                if(ec == asio::error::operation_aborted)
                    return;

                if constexpr(has_expiryDur)
                {
                    if(anyReached->test_and_set(std::memory_order_relaxed))
                        return;
                    _timer.cancel();
                }

                if constexpr(EnableStop)
                    _stopCb.reset();

                _ec = ec;
                
                if(! ec)
                {
                    _compOp(std::forward<decltype(r)>(r)...);
                    assign_t(std::forward<decltype(r)>(r)...);
                }

                _coro.resume();
            }
        );

        if constexpr(has_expiryDur)
        {
            _timer.async_wait(
                [this, anyReached]([[maybe_unused]] auto&& ec)
                {
                    if(ec || anyReached->test_and_set(std::memory_order_relaxed))
                        return;

                    if constexpr(EnableStop)
                        _stopCb.reset();

                    _ao.cancel();
                    _ec = gerrc::timeout;
                    _coro.resume();
                }
            );
        }

        if constexpr(EnableStop)
        {
            BOOST_ASSERT(! _stopCb);
            _stopCb.emplace(c.promise().get_stop_token(),
                [this, anyReached]()
                {
                    if constexpr(has_expiryDur)
                    {
                        if(anyReached->test_and_set(std::memory_order_relaxed))
                            return;
                        _timer.cancel();
                    }

                    _ao.cancel();
                    _ec = asio::error::operation_aborted;
                    _coro.resume();
                }
            );
        }

        return true;
    }

    aresult<T> await_resume()
    {
        if constexpr(std::is_void_v<T>)
        {
            return _ec;
        }
        else
        {
            if(_ec)
                return _ec;

            if constexpr(PreAssign)
                return std::move(_t);
            else
                return std::move(*_t);
        }
    }
};


template<class T, class AsyncObj, class InitOp, class CompOp, class... P>
auto make_ec_awaiter_c(AsyncObj& ao, InitOp&& initOp, CompOp&& compOp, P... p)
{
    auto params = make_params(p..., p_disable_stop, p_expires_never); // put default options last

    return ec_awaiter<
        T, AsyncObj, std::remove_cvref_t<InitOp>, std::remove_cvref_t<CompOp>,
        params(t_stop_enabled), decltype(params(t_expiry_dur))
    >(ao, std::forward<InitOp>(initOp), std::forward<CompOp>(compOp), params(t_expiry_dur));
}

template<class T, class AsyncObj, class InitOp, class... P>
auto make_ec_awaiter(AsyncObj& ao, InitOp&& initOp, P... p)
{
    return make_ec_awaiter_c<T>(ao, std::forward<InitOp>(initOp), null_op, p...);
}

template<class T, class AsyncObj, class InitOp, class... P>
auto make_ec_awaiter_pre_assign(T&& t, AsyncObj& ao, InitOp&& initOp, P... p)
{
    auto params = make_params(p..., p_disable_stop, p_expires_never); // put default options last

    return ec_awaiter<
        std::remove_cvref_t<T>, AsyncObj, std::remove_cvref_t<InitOp>, null_op_t,
        params(t_stop_enabled), decltype(params(t_expiry_dur)),
        true
    >(std::forward<T>(t), ao, std::forward<InitOp>(initOp), null_op, params(t_expiry_dur));
}


template<class F>
struct ec_direct_return_awaiter
{
    F f;

    ec_direct_return_awaiter(auto&& u): f{JKL_FORWARD(u)} {}

    bool await_ready() const noexcept { return true; }
    void await_suspend(auto) const noexcept {}
    
    aresult<std::invoke_result_t<F, aerror_code&>> await_resume()
    {
        aerror_code e;

        if constexpr(std::is_void_v<std::invoke_result_t<F, aerror_code&>>)
        {
            f(e);
            if(e)
                return e;
        }
        else
        {
            auto t = f(e);

            if(e)
                return e;

            return t;
        }
    }
};

template<class F>
ec_direct_return_awaiter(F) -> ec_direct_return_awaiter<F>;


} // namespace jkl
