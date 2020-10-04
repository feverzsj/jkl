#pragma once

#include <jkl/config.hpp>
#include <jkl/tcp.hpp>
#include <jkl/error.hpp>
#include <jkl/result.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/certify/https_verification.hpp>


namespace jkl{


class ssl_context : public asio::ssl::context
{
    using base = asio::ssl::context;

public:
    using base::base;
    using base::operator=;

    // Enables the use of the OS supplied certificate validation mechanism during a TLS handshake.
    // Certificate verification is first performed using CA certificates imported into OpenSSL.
    // If that fails because of missing CA certificates, the library will utilize platform specific
    // APIs to try and complete the process.
    // If verification, using the platform-specific API, fails, the handshake operation will fail
    // with an error indicating that the certificate chain was self-signed. A more detailed error
    // can be retrieved after the handshake operation completes, using SSL_get_verify_result.
    // The library uses SSL_CTX_set_cert_verify_callback in order to override the default certificate
    // verification procedure. If a regular verification callback is set using set_verify_callback(),
    // it will be invoked during the first phase of certificate verification, in the same way a
    // default-configured asio::ssl::context would.
    void use_native_cert_verify()
    {
        boost::certify::enable_native_https_server_verification(*this);
    }
};


template<class AsyncSslStream>
class ssl_conn_t : public conn_t<AsyncSslStream>
{
    using base = conn_t<AsyncSslStream>;

public:
    using base::base;
    using base::operator=;

    template<class... P>
    auto handshake(asio::ssl::stream_base::handshake_type t, P... p)
    {
        return make_ec_awaiter(
            [&, t](auto&& h){ base::stream().async_handshake(t, std::move(h)); },
            p...);
    }

    template<class B, class... P>
    auto buffered_handshake(asio::ssl::stream_base::handshake_type t, B& buf, P... p)
    {
        return make_ec_awaiter(
            [&, t, b = asio_buf(b)](auto&& h){ base::stream().async_handshake(t, b, std::move(h)); },
        p...);
    }

    template<class... E>
    void set_verify_mode(asio::ssl::verify_mode m, E&... e) 
    {
        base::stream().set_verify_mode(m, e...);
    }

    template<class... E>
    void set_verify_depth(int d, E&... e)
    {
        base::stream().set_verify_depth(d);
    }

    template<class F, class... E>
    void set_verify_callback(F&& f, E&... e)
    {
        base::stream().set_verify_callback(std::forward<F>(f), e);
    }

    template<class Str = std::string_view>
    Str sni_hostname() const noexcept
    {
        if(auto* name = SSL_get_servername(base::stream_native_handle(), TLSEXT_NAMETYPE_host_name))
            return name;
        return {};
    }

    // for when this is a client;
    // many hosts need this to handshake successfully;
    template<class Str> 
    void set_sni_hostname(Str const& name)
    {
        if(0 == SSL_set_tlsext_host_name(base::stream_native_handle(), as_cstr(name).data()))
            throw asystem_error(aerror_code(static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()));
    }

    // Sets the expected server hostname, which will be checked during the verification process.
    // The hostname must not contain \0 characters.
    template<class Str>
    void set_server_hostname(Str const& name)
    {
        auto* param = ::SSL_get0_param(base::stream_native_handle());
        
        ::X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);

        if(0 == X509_VERIFY_PARAM_set1_host(param, str_data(name), str_size(name)))
            throw asystem_error(aerror_code(static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()));
    }
};


using ssl_tcp_conn = ssl_conn_t<asio::ssl::stream<asio::ip::tcp::socket>>;


} // namespace jkl