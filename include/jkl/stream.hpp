#pragma once

#include <jkl/config.hpp>
#include <jkl/url.hpp>
#include <jkl/tcp.hpp>
#include <jkl/ssl.hpp>
#include <jkl/task.hpp>
#include <jkl/result.hpp>
#include <jkl/ec_awaiter.hpp>
#include <jkl/util/unit.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>


namespace jkl{


namespace http = beast::http;

using beast::simple_rate_policy;
using beast::unlimited_rate_policy;
using beast::flat_buffer;


template<class Stream>
class timed_stream_t
{
    Stream  _s;

public:
    using stream_type       = Stream;   // beast::basic_stream, beast::ssl_stream
    using next_layer_type   = next_layer_t<Stream>;   // beast::basic_stream
    using lowest_layer_type = lowest_layer_t<Stream>; // asio::basic_socket
    using socket_type       = typename next_layer_type::socket_type;    // asio::basic_stream_socket
    using executor_type     = typename lowest_layer_type::executor_type;
    using protocol_type     = typename lowest_layer_type::protocol_type;
    using endpoint_type     = typename lowest_layer_type::endpoint_type;

    template<class... A>
    explicit timed_stream_t(A&&... streamArgs) : _s(std::forward<A>(streamArgs)...) {}

    //timed_stream_t& operator=(socket_type&& s) { socket() = std::move(s); }

    auto      & stream()       noexcept { return _s; }
    auto const& stream() const noexcept { return _s; }
    auto      & astream()       noexcept { return _s; }
    auto const& astream() const noexcept { return _s; }

    auto      & next_layer()       { return get_next_layer(_s); }
    auto const& next_layer() const { return get_next_layer(_s); }
    auto      & lowest_layer()       { return get_lowest_layer(_s); }
    auto const& lowest_layer() const { return get_lowest_layer(_s); }

    auto      & socket()       { return next_layer().socket(); }
    auto const& socket() const { return next_layer().socket(); }

    decltype(auto) socket_native_handle() { return socket().native_handle(); }

    auto      & rate_policy()       { return next_layer().rate_policy(); }
    auto const& rate_policy() const { return next_layer().rate_policy(); }

    auto get_executor() const { return next_layer().get_executor(); }

    void cancel() { next_layer().cancel(); }
    void close () { next_layer().close (); }
    void expires_after(std::chrono::nanoseconds t) { next_layer().expires_after(t); }
    void expires_at(asio::steady_timer::time_point t) { next_layer().expires_at(t); }
    void expires_never() { next_layer().expires_never(); }
    auto release_socket() { return next_layer().release_socket();}

    template<class T>
    aresult<> set_option(T const& t)
    {
        aerror_code e;
        lowest_layer().set_option(t, e);
        return e;
    }

    template<class T>
    aresult<> get_option(T& t)
    {
        aerror_code e;
        lowest_layer().get_option(t, e);
        if(! e)
            return t;
        return e;
    }

    template<class T>
    aresult<T> option()
    {
        T t;
        JKL_TRY(get_option(t));
        return t;
    }

    template<class... P>
    auto connect(endpoint_type const& ep, P... p)
    {
        return make_ec_awaiter<void>(
            [&, ep](auto&& h){ next_layer().async_connect(ep, std::move(h)); },
            p...);
    }

    template<class T, class... P>
    auto connect(T const& t, P... p)
    {
        if constexpr(std::is_same_v<T, endpoint_type>)
            return make_ec_awaiter<void>(
                [&, t](auto&& h){ next_layer().async_connect(t, std::move(h)); },
                p...);
        else if constexpr(asio::is_endpoint_sequence<T>::value)
            return make_ec_awaiter<endpoint_type>(
                [&, t](auto&& h){ next_layer().async_connect(t, std::move(h)); },
                p...);
        else
            return connect_url(t, p...);
    }

    template<class... P>
    aresult_task<> connect_url(aurl u, P... p)
    {
        if(auto e = u.endpoint())
            co_return co_await connect(*e, p...);

        tcp_resolver_t<executor_type> rsv(get_executor());

        JKL_CO_TRY(eps, co_await rsv.resolve(u.hostname(), u.real_port_or_protocol(), p...));

        co_return co_await connect(eps, p...);
    }

    template<class... P>
    auto shutdown(P... p)
    {
        if constexpr(JKL_CEVL(has_async_shutdown(_s, null_op)))
            return make_ec_awaiter<void>(
                [&](auto&& h){ _s.async_shutdown(std::move(h)); },
                p...);
        else
            return ec_direct_return_awaiter(
                [&](auto& e){ _s.shutdown(asio::basic_socket::shutdown_both, e); });
    }

    aresult<> shutdown_next_layer(asio::basic_socket::shutdown_type t)
    {
        aerror_code e;
        next_layer().shutdown(t, e);
        return e;
    }

    template<class... P>
    auto handshake(asio::ssl::stream_base::handshake_type t, P... p)
    {
        return make_ec_awaiter<void>(
            [&, t](auto&& h){ _s.async_handshake(t, std::move(h)); },
            p...);
    }

    template<class Buf, class... P>
    auto buffered_handshake(asio::ssl::stream_base::handshake_type t, Buf& b, P... p)
    {
        return make_ec_awaiter<void>(
            [&, t, b = asio_buf(b)](auto&& h){ _s.async_handshake(t, b, std::move(h)); },
        p...);
    }

    // not using aresult<>, as this should only use SSL_set_verify which always succeeds
    template<class = void> // just to avoid instantiating this method when instantiates the class
    void set_verify_mode(asio::ssl::verify_mode m) 
    {
        _s.set_verify_mode(m);
    }

    template<class = void>
    void set_verify_depth(int d)
    {
        _s.set_verify_depth(d);
    }

    template<class F>
    void set_verify_callback(F&& f)
    {
        _s.set_verify_callback(std::forward<F>(f));
    }

    template<class Str = std::string_view>
    Str sni_hostname() const noexcept
    {
        if(auto* name = SSL_get_servername(_s.native_handle(), TLSEXT_NAMETYPE_host_name))
            return {name};
        return {};

    }

    // for when this is a client;
    // many hosts need this to handshake successfully;
    template<class Str> 
    aresult<> set_sni_hostname(Str const& name) noexcept
    {
        // _s should be ssl stream
        if(SSL_set_tlsext_host_name(_s.native_handle(), as_cstr(name).data()))
            return no_err;
        return aerror_code(static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category());
    }

    // Sets the expected server hostname, which will be checked during the verification process.
    // The hostname must not contain \0 characters.
    template<class Str>
    aresult<> set_server_hostname(Str const& name) noexcept
    {
        auto* param = ::SSL_get0_param(handle);
        
        ::X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);

        if(X509_VERIFY_PARAM_set1_host(param, str_data(name), str_size(name)))
            return no_err;
        return aerror_code(static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category());
    }
};

template<
    class RatePolicy = unlimited_rate_policy,
    class Req        = http::request<http::string_body>,
    class Res        = http::response<http::dynamic_body>,
    class ReadBuf    = flat_buffer,
    class Executor   = asio::executor
>
using http_plain_conn_t = timed_stream_t<beast::basic_stream<asio::ip::tcp, Executor, RatePolicy>, Req, Res, ReadBuf>;

template<
    class RatePolicy = unlimited_rate_policy,
    class Req        = http::request<http::string_body>,
    class Res        = http::response<http::dynamic_body>,
    class ReadBuf    = flat_buffer,
    class Executor   = asio::executor
>
using http_ssl_conn_t = timed_stream_t<beast::ssl_stream<beast::basic_stream<asio::ip::tcp, Executor, RatePolicy>>, Req, Res, ReadBuf>;

using http_plain_conn = http_plain_conn_t<>;
using http_ssl_conn   = http_ssl_conn_t<>;



template<class Protocol, class Executor, class RatePolicy>
class basic_var_stream
{
public:
    using plain_stream_type = beast::basic_stream<Protocol, Executor, RatePolicy>;
    using ssl_stream_type   = beast::ssl_stream<plain_stream_type>;

    using executor_type = Executor;

    template<class... A>
    explicit basic_var_stream(A&&... args) : _v(std::forward<A>(args)...) {}

    auto& next_layer()
    {
        return std::visit([](auto& s) -> auto& { return get_next_layer(s); }, _v);
    }

    auto const& next_layer() const
    {
        return std::visit([](auto& s) -> auto& { return get_next_layer(s); }, _v);
    }

    template<class... A>
    void async_read_some(A&&... args)
    {
        std::visit([&](auto& s){ s.async_read_some(std::forward<A>(args)...); }, _v);
    }

    template<class... A>
    void async_write_some(A&&... args)
    {
        std::visit([&](auto& s){ s.async_write_some(std::forward<A>(args)...); }, _v);
    }

    // ssl specific

    using native_handle_type = typename ssl_stream_type::native_handle_type;

    native_handle_type native_handle() { return std::get<ssl_stream_type>(_v).native_handle(); }

    template<class... A>
    void async_handshake(A&&... args)
    {
        std::get<ssl_stream_type>(_v).async_handshake(std::forward<A>(args)...);
    }

    template<class... A>
    void set_verify_mode(A&&... args)
    {
        std::get<ssl_stream_type>(_v).set_verify_mode(std::forward<A>(args)...);
    }

    template<class... A>
    void set_verify_depth(A&&... args)
    {
        std::get<ssl_stream_type>(_v).set_verify_depth(std::forward<A>(args)...);
    }

    template<class... A>
    void set_verify_callback(A&&... args)
    {
        std::get<ssl_stream_type>(_v).set_verify_callback(std::forward<A>(args)...);
    }

private:    
    std::variant<plain_stream_type, ssl_stream_type> _v;
};


template<
    class RatePolicy = unlimited_rate_policy,
    class Req        = http::request<http::string_body>,
    class Res        = http::response<http::dynamic_body>,
    class ReadBuf    = flat_buffer,
    class Executor   = asio::executor
>
using http_var_conn_t = timed_stream_t<basic_var_stream<asio::ip::tcp, Executor, RatePolicy>, Req, Res, ReadBuf>;


// typical usage:
//    http_conn_factory factory;
//    //factory.set_max_read_buf(MiB(6));
//    factory.ssl_ctx().set_verify_mode(asio::ssl::verify_peer);
//    factory.ssl_ctx().use_native_cert_verify();
//    auto conn = co_await factory.connect(url, ...);
//    ... prepare request ...
//    JKL_CO_TRY(co_await conn.write_req(...)); // or handle error manually here.
//    JKL_CO_TRY(co_await conn.read_res(...));
//    ... process response ...
class http_conn_factory
{
    std::optional<ssl_context> _ownSslCtx;

    asio::io_context* _ioc = nullptr;
    ssl_context*      _slc = nullptr;

    _B_<size_t> _maxRdBuf = SIZE_MAX;
    bool        _noDelayWhenHandshake = false;

public:
    explicit http_conn_factory(asio::io_context& ioc = default_ioc())
        : _ioc(&ioc)
    {
        use_own_ssl_ctx();
    }

    explicit http_conn_factory(ssl_context& slc, asio::io_context& ioc = default_ioc())
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

    template<
        class RatePolicy = unlimited_rate_policy,
        class Req        = http::request<http::string_body>,
        class Res        = http::response<http::dynamic_body>,
        class ReadBuf    = flat_buffer,
        class Executor   = asio::executor,
        class... P>
    aresult_task<http_var_conn_t<RatePolicy, Req, Res, ReadBuf, Executor>>
    connect_ex(aurl u, Executor const& ex, P... p)
    {
        using conn_t = http_var_conn_t<RatePolicy, Req, Res, ReadBuf, Executor>;
        using plain_stream_t = typename conn_t::stream_type::plain_stream_type;
        using ssl_stream_t = typename conn_t::stream_type::ssl_stream_type;

        if(u.scheme() == "http")
        {
            conn_t conn(std::in_place_type_t<plain_stream_t>, ex, ssl_ctx());
            
            if(_maxRdBuf != _B_<size_t>(SIZE_MAX))
                conn.set_max_read_buf(_maxRdBuf);
            
            JKL_CO_TRY(co_await conn.connect(u, p...));
            co_return conn;
        }

        if(u.scheme() == "https")
        {
            conn_t conn(std::in_place_type_t<ssl_stream_t>, ex, ssl_ctx());
            
            if(_maxRdBuf != _B_<size_t>(SIZE_MAX))
                conn.set_max_read_buf(_maxRdBuf);

            if(u.hostname().size())
                JKL_CO_TRY(conn.set_sni_hostname(u.hostname()));

            if(_verifyMode != -1)
                JKL_CO_TRY(conn.set_verify_mode(_verifyMode));
            if(_verifyDepth != -1)
                JKL_CO_TRY(conn.set_verify_depth(_verifyDepth));
            if(_verifyCallback)
                JKL_CO_TRY(conn.set_verify_callback(_verifyCallback));

            JKL_CO_TRY(co_await conn.connect(u, p...));
            
            if(_noDelayWhenHandshake)
            {
                JKL_CO_TRY(noDelay, conn.option<asio::ip::tcp::no_delay>());
                if(! noDelay)
                    JKL_CO_TRY(conn.set_option(asio::ip::tcp::no_delay(true)));

                auto r1 = co_await conn.handshake(asio::ssl::stream_base::client, u, p...);

                if(! noDelay)
                {
                    auto r2 = (conn.set_option(asio::ip::tcp::no_delay(false)));
                    JKL_CO_TRY(r1);
                    JKL_CO_TRY(r2);
                }
                else
                {
                    JKL_CO_TRY(r1);
                }
            }
            else
            {
                JKL_CO_TRY(co_await conn.handshake(asio::ssl::stream_base::client, u, p...));
            }

            co_return conn;
        }

        co_return aerrc::protocol_not_supported;
    }

    template<
        class RatePolicy = unlimited_rate_policy,
        class Req        = http::request<http::string_body>,
        class Res        = http::response<http::dynamic_body>,
        class ReadBuf    = flat_buffer,
        class Executor   = asio::executor,
        class... P>
    auto connect(aurl u, P... p)
    {
        return connect_ex(std::move(u), io_ctx().get_executor(), p...);
    }
};


} // namespace jkl