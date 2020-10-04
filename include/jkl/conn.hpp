#pragma once

#include <jkl/config.hpp>
#include <jkl/url.hpp>
#include <jkl/task.hpp>
#include <jkl/traits.hpp>
#include <jkl/result.hpp>
#include <jkl/resolver.hpp>
#include <jkl/ec_awaiter.hpp>


namespace jkl{


template<class AsyncStream>
class conn_t
{
    AsyncStream _s;

public:
    using stream_type       = AsyncStream;
    using executor_type     = typename stream_type::executor_type;
    using next_layer_type   = next_layer_t<stream_type>;
    using lowest_layer_type = lowest_layer_t<stream_type>; // typically asio::basic_socket    
    using protocol_type     = typename lowest_layer_type::protocol_type;
    using endpoint_type     = typename protocol_type::endpoint_type;

    struct rebind_executor { using type = conn_t<rebind_executor_t<stream_type>> };

    template<class... A>
    explicit conn_t(A&&... streamArgs) : _s(std::forward<A>(streamArgs)...) {}

    auto get_executor() const { return _s.get_executor(); }

    auto      & stream()       noexcept { return _s; }
    auto const& stream() const noexcept { return _s; }
    auto      & astream()       noexcept { return _s; }
    auto const& astream() const noexcept { return _s; }

    auto      & next_layer()       { return get_next_layer(_s); }
    auto const& next_layer() const { return get_next_layer(_s); }
    auto      & lowest_layer()       { return get_lowest_layer(_s); }
    auto const& lowest_layer() const { return get_lowest_layer(_s); }

    auto stream_native_handle() { return _s.native_handle(); }
    auto socket_native_handle() { return lowest_layer().native_handle(); }

    template<class... T>
    void set_option(T&&... t)
    {
        lowest_layer().set_option(std::forward<T>(t)...);
    }

    template<class... T>
    void get_option(T&... t) const
    {
        lowest_layer().get_option(t...);
    }

    template<class Opt, class... E>
    Opt option(E&... e) const
    {
        Opt opt;
        get_option(opt, e...);
        return opt;
    }

    template<class... E> void cancel(E&... e) { next_layer().cancel(e...); }
    template<class... E> void close (E&... e) { next_layer().close (e...); }
    template<class... E>
    void shutdown_lowest_layer(asio::basic_socket::shutdown_type t, E&... e)
    {
        lowest_layer().shutdown(t, e...);
    }

    template<class... P>
    auto shutdown(P... p)
    {
        if constexpr(requires{_s.async_shutdown(null_op);})
            return make_ec_awaiter(
                [&](auto&& h){ _s.async_shutdown(std::move(h)); },
                p...);
        else
            return ec_direct_return_awaiter(
                [&](auto& ec){ shutdown_lowest_layer(asio::basic_socket::shutdown_both, ec); });
    }

    template<class T, class... P>
    auto connect(T&& t, P... p)
    {
        if constexpr(std::is_same_v<std::remove_cvref_t<T>, endpoint_type>)
        {
            return make_ec_awaiter_pre_assign(t,
                [&, ep = t](auto&& h){ lowest_layer().async_connect(ep, std::move(h)); },
                p...);
        }
        else if constexpr(asio::is_endpoint_sequence<std::remove_cvref_t<T>>)
        {
            return make_ec_awaiter<endpoint_type>(
                [&, eps = std::forward<T>(t)](auto&& h){ asio::async_connect(lowest_layer(), eps, std::move(h)); },
                p...);
        }
        else
        {
            return connect(aurl(std::forward<T>(t)), p...);
        }
    }

    template<class... P>
    aresult_task<endpoint_type> connect(aurl u, P... p)
    {
        if(auto ep = u.endpoint<protocol_type>())
            co_return co_await connect(*ep, p...);

        resolver_t<protocol_type, executor_type> rsv(get_executor());

        JKL_CO_TRY(eps, co_await rsv.resolve(u.hostname(), u.real_port_or_protocol(), p...));

        co_return co_await connect(eps, p...);
    }

    template<class B, class... P>
    auto read_some(B&& b, P... p)
    {
        return jkl::read_some(_s, std::forward<B>(b), p...);
    }

    template<class Buf, class... P>
    auto read_some_to_tail(Buf& b, size_t maxSize, P... p)
    {
        return jkl::read_some_to_tail(_s, b, maxSize, p...);
    }

    template<class B, class... P>
    auto write_some(B&& b, P... p)
    {
        return jkl::write_some(_s, std::forward<B>(b), p...);
    }

    template<class Buf, class... P>
    auto write_some_from(Buf& b, size_t pos, P... p)
    {
        return jkl::write_some_from(_s, b, pos, p...);
    }


    template<class B, class CompCond, class... P>
    auto read(B&& b, CompCond&& c, P... p)
    {
        return jkl::read(_s, std::forward<B>(b), std::forward<CompCond>(c), p...);
    }

    template<class B, class... P>
    auto read_to_fill(B&& b, P... p)
    {
        return jkl::read_to_fill(_s, std::forward<B>(b), p...);
    }

    template<class Buf, class... P>
    auto read_to_tail(Buf& b, size_t size, P... p)
    {
        return jkl::read_to_tail(_s, b, size, p...);
    }

    template<class B, class Match, class... P>
    auto read_until(B&& b, Match&& m, P... p)
    {
        return jkl::read_until(_s, std::forward<B>(b), std::forward<Match>(m), p...);
    }
};


// NOTE: all conn must have same next_layer_type and executor_type
template<class... Conns>
class var_conn_t : std::variant<Conns...>
{
    using base = std::variant<Conns...>;

public:
    struct rebind_executor
    {
        using type = var_conn_t<rebind_executor_t<Conns>...>;
    };

    using base::base;
    using base::operator=;

    template<class F>
    decltype(auto) visit(F&& f) { return std::visit([](auto& c){ return f(c); }, *this); }
    template<class F>
    decltype(auto) visit(F&& f) const { return std::visit([](auto& c){ return f(c); }, *this); }

    auto get_executor() { return visit([](auto& c){ return c.get_executor(); }); }

    auto      & next_layer  ()       { return visit([](auto& c){ return c.next_layer  (); }); }
    auto const& next_layer  () const { return visit([](auto& c){ return c.next_layer  (); }); }
    auto      & lowest_layer()       { return visit([](auto& c){ return c.lowest_layer(); }); }
    auto const& lowest_layer() const { return visit([](auto& c){ return c.lowest_layer(); }); }

    auto stream_native_handle() { return visit([](auto& c){ return c.stream_native_handle(); }); }
    auto socket_native_handle() { return visit([](auto& c){ return c.socket_native_handle(); }); }


    template<class... T>
    void set_option(T&&... t) { return visit([&](auto& c){ return c.set_option(std::forward<T>(t)...); }); }
    
    template<class... T>
    void get_option(T&... t) const { return visit([&](auto& c){ return c.get_option(t...); }); }

    template<class Opt, class... E>
    Opt option(E&... e) const { return visit([&](auto& c){ return c.option<Opt>(e...); }); }


    template<class... E> void cancel(E&... e) { visit([&](auto& c){ c.cancel(e...); }); }
    template<class... E> void close (E&... e) { visit([&](auto& c){ c.close (e...); }); }
    template<class... E>
    void shutdown_lowest_layer(asio::basic_socket::shutdown_type t, E&... e)
    {
        visit([&](auto& c){ c.shutdown_lowest_layer(t, e...); });
    }

    template<class... P>
    auto shutdown(P... p) { return visit([&](auto& c){ return c.shutdown(p...); }); }


    template<class T, class... P>
    auto connect(T&& t, P... p) { return visit([&](auto& c){ return c.connect(std::forward<T>(t), p...); }); }


    template<class B, class... P>
    auto read_some(B&& b, P... p)
    {
        return visit([&](auto& c){ return c.read_some(std::forward<B>(b), p...); });
    }

    template<class Buf, class... P>
    auto read_some_to_tail(Buf& b, size_t maxSize, P... p)
    {
        return visit([&](auto& c){ return c.read_some_to_tail(b, maxSize, p...); }); 
    }

    template<class B, class... P>
    auto write_some(B&& b, P... p)
    {
        return visit([&](auto& c){ return c.write_some(std::forward<B>(b), p...); });
    }

    template<class Buf, class... P>
    auto write_some_from(Buf& b, size_t pos, P... p)
    {
        return visit([&](auto& c){ return c.write_some_from(b, pos, p...); });
    }

    template<class B, class CompCond, class... P>
    auto read(B&& b, CompCond&& c, P... p)
    {
        return visit([&](auto& c){ return c.read(std::forward<B>(b), std::forward<CompCond>(c), p...); });
    }

    template<class B, class... P>
    auto read_to_fill(B&& b, P... p)
    {
        return visit([&](auto& c){ return c.read_to_fill(std::forward<B>(b), p...); });
    }

    template<class Buf, class... P>
    auto read_to_tail(Buf& b, size_t size, P... p)
    {
        return visit([&](auto& c){ return c.read_to_tail(b, size, p...); });
    }

    template<class B, class Match, class... P>
    auto read_until(B&& b, Match&& m, P... p)
    {
        return visit([&](auto& c){ return c.read_until(std::forward<B>(b), std::forward<Match>(m), p...); });
    }
};



// typical usage:
//    conn_factory factory;
//    //factory.set_max_read_buf(MiB(6));
//    factory.ssl_ctx().set_verify_mode(asio::ssl::verify_peer);
//    factory.ssl_ctx().use_native_cert_verify();
//    auto conn = co_await factory.connect(url, ...);
//    ... prepare request ...
//    JKL_CO_TRY(co_await conn.write_req(...)); // or handle error manually here.
//    JKL_CO_TRY(co_await conn.read_res(...));
//    ... process response ...
template<class Conn, class SslConn>
class conn_factory
{
    std::optional<ssl_context> _ownSslCtx;

    asio::io_context* _ioc = nullptr;
    ssl_context*      _slc = nullptr;

    _B_<size_t> _maxRdBuf = SIZE_MAX;
    bool        _noDelayWhenHandshake = false;

public:
    using conn_type = rebind_executor_t<var_conn_t<Conn, SslConn>, asio::io_context::executor_type>;

    explicit conn_factory(asio::io_context& ioc = default_ioc())
        : _ioc(&ioc)
    {
        use_own_ssl_ctx();
    }

    explicit conn_factory(ssl_context& slc, asio::io_context& ioc = default_ioc())
        : _ioc(&ioc), _slc(&slc)
    {}

    auto& io_ctx() noexcept { return *_ioc; }
    auto& ssl_ctx() noexcept { return *_slc; }

    void set_io_ctx(asio::io_context& c) noexcept { _ioc = &c; }
    void set_ssl_ctx(ssl_context& c) noexcept { _slc = &c; }
    void use_own_ssl_ctx()
    {
        if(! _ownSslCtx)
            _ownSslCtx.emplace(asio::ssl::context::tlsv12_client);
        _slc = & _ownSslCtx.value();
    }

    void set_max_read_buf(_B_<size_t> n) noexcept { _maxRdBuf = n; }

    // https://www.extrahop.com/company/blog/2016/tcp-nodelay-nagle-quickack-best-practices
    void set_no_delay_when_handshake(bool on = true) noexcept { _noDelayWhenHandshake = on; }

    template< class Ex, class... P>
    aresult_task<rebind_executor_t<conn_type, Ex>> connect_ex(aurl u, Ex const& ex, P... p)
    {
        if(u.scheme() == "http")
        {
            rebind_executor_t<Conn> conn(ex);
            
            if(_maxRdBuf != _B_<size_t>(SIZE_MAX))
                conn.set_max_read_buf(_maxRdBuf);
            
            JKL_CO_TRY(co_await conn.connect(u, p...));
            co_return conn;
        }

        if(u.scheme() == "https")
        {
            rebind_executor_t<SslConn> conn(ex, ssl_ctx());
            
            if(_maxRdBuf != _B_<size_t>(SIZE_MAX))
                conn.set_max_read_buf(_maxRdBuf);

            if(u.hostname().size())
                conn.set_sni_hostname(u.hostname());

            JKL_CO_TRY(co_await conn.connect(u, p...));
            
            if(_noDelayWhenHandshake)
            {
                auto noDelay = conn.option<asio::ip::tcp::no_delay>();
                if(! noDelay)
                    conn.set_option(asio::ip::tcp::no_delay(true));

                auto r = co_await conn.handshake(asio::ssl::stream_base::client, u, p...);

                if(! noDelay)
                    conn.set_option(asio::ip::tcp::no_delay(false));

                JKL_CO_TRY(r);
            }
            else
            {
                JKL_CO_TRY(co_await conn.handshake(asio::ssl::stream_base::client, u, p...));
            }

            co_return conn;
        }

        co_return aerrc::protocol_not_supported;
    }

    template<class T, class... P>
    auto connect(T&& t, P... p)
    {
        return connect_ex(std::forward<T>(t), io_ctx().get_executor(), p...);
    }
};


} // namespace jkl