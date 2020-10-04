#pragma once

#include <jkl/config.hpp>
#include <jkl/util/str.hpp>
#include <jkl/util/unit.hpp>
#include <jkl/curl/url.hpp>
#include <jkl/curl/str.hpp>
#include <jkl/curl/list.hpp>
#include <jkl/curl/share.hpp>
#include <curl/curl.h>
#include <chrono>


namespace jkl::curlopt{


template<auto Opt>
constexpr auto long_opt = [](long v){ return [v](auto& h){ h.setopt(Opt, v); }; };

template<auto Opt>
constexpr auto offt_opt = [](curl_off_t v){ return [v](auto& h){ h.setopt(Opt, v); }; };


// template<auto Opt>
// constexpr auto bool_opt = [](bool v = true){ return [v](auto& h){ h.setopt(Opt, v ? 1L : 0L); }; };
template<auto Opt>
struct bool_opt_t
{
    bool v = true;

    bool_opt_t operator()(bool t) const noexcept { return {t}; }

    template<class H>
    auto operator()(H& h) const requires(requires{ h.setopt(Opt, 1L); })
    {
        h.setopt(Opt, v ? 1L : 0L);
    }
};

template<auto Opt>
constexpr bool_opt_t<Opt> bool_opt;


template<auto Opt>
constexpr auto cstr_opt = overload
{
    [](std::nullptr_t)
    {
        return [](auto& h){ h.setopt(Opt, static_cast<char*>(nullptr)); };
    },
    [](auto const& v)
    {        // vv  this opt should be used in place, don't copy/move them around.
        return [&v](auto& h)
        {
            if(! str_size(v))
                h.setopt(Opt, static_cast<char*>(nullptr));
            else
                h.setopt(Opt, const_cast<char*>(as_cstr(v).data()));
        };
    }
};

template<auto Opt, class T>
constexpr auto ptr_opt = [](T v)
{
    static_assert(std::is_pointer_v<T> || std::is_null_pointer_v<T>);
    return [v](auto& h){ h.setopt(Opt, v); };
};

template<auto Opt> constexpr auto data_opt = ptr_opt<Opt, void*>;

template<auto Opt, class T>
constexpr auto func_opt = [](T v)
{
    static_assert((std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>) || std::is_null_pointer_v<T>);
    return [v](auto& h){ h.setopt(Opt, v); };
};

template<auto Opt, auto DataOpt, class F>
constexpr auto cb_opt = [](F f, void* d)
{
    static_assert((std::is_pointer_v<F> && std::is_function_v<std::remove_pointer_t<F>>) || std::is_null_pointer_v<F>);
    return [f, d](auto& h){ h.setopt(Opt, f); h.setopt(DataOpt, d); };
};

template<auto Opt, class Handle>
constexpr auto handle_opt = [](Handle& l)
{
    return [&l](auto& h){ BOOST_ASSERT(l.handle()); h.setopt(Opt, l.handle()); };
};

template<auto Opt, class U, class Expected = long>
constexpr auto unit_opt = [](U const& u)
{
    return [u](auto& h){ h.setopt(Opt, static_cast<Expected>(u.count())); };
};

template<auto Opt, class U>
constexpr auto lunit_opt = unit_opt<Opt, U, curl_off_t>;


// BEHAVIOR OPTIONS

constexpr auto verbose            = bool_opt<CURLOPT_VERBOSE      >;
constexpr auto header_to_write_cb = bool_opt<CURLOPT_HEADER       >;
constexpr auto noprogress         = bool_opt<CURLOPT_NOPROGRESS   >;
constexpr auto nosignal           = bool_opt<CURLOPT_NOSIGNAL     >;
constexpr auto wildcardmatch      = bool_opt<CURLOPT_WILDCARDMATCH>;


// CALLBACK OPTIONS

constexpr auto writefunc                = func_opt<CURLOPT_WRITEFUNCTION             , curl_write_callback         >;
constexpr auto writedata                = data_opt<CURLOPT_WRITEDATA                                               >;
constexpr auto readfunc                 = func_opt<CURLOPT_READFUNCTION              , curl_read_callback          >;
constexpr auto readdata                 = data_opt<CURLOPT_READDATA                                                >;
constexpr auto ioctlfunc                = func_opt<CURLOPT_IOCTLFUNCTION             , curl_ioctl_callback         >;
constexpr auto ioctldata                = data_opt<CURLOPT_IOCTLDATA                                               >;
constexpr auto seekfunc                 = func_opt<CURLOPT_SEEKFUNCTION              , curl_seek_callback          >;
constexpr auto seekdata                 = data_opt<CURLOPT_SEEKDATA                                                >;
constexpr auto sockoptfunc              = func_opt<CURLOPT_SOCKOPTFUNCTION           , curl_sockopt_callback       >;
constexpr auto sockoptdata              = data_opt<CURLOPT_SOCKOPTDATA                                             >;
constexpr auto opensocketfunc           = func_opt<CURLOPT_OPENSOCKETFUNCTION        , curl_opensocket_callback    >;
constexpr auto opensocketdata           = data_opt<CURLOPT_OPENSOCKETDATA                                          >;
constexpr auto closesocketfunc          = func_opt<CURLOPT_CLOSESOCKETFUNCTION       , curl_closesocket_callback   >;
constexpr auto closesocketdata          = data_opt<CURLOPT_CLOSESOCKETDATA                                         >;
constexpr auto progressfunc             = func_opt<CURLOPT_PROGRESSFUNCTION          , curl_progress_callback      >;
constexpr auto progressdata             = data_opt<CURLOPT_PROGRESSDATA                                            >;
constexpr auto xferinfofunc             = func_opt<CURLOPT_XFERINFOFUNCTION          , curl_xferinfo_callback      >;
constexpr auto xferinfodata             = data_opt<CURLOPT_XFERINFODATA                                            >;
constexpr auto headerfunc               = func_opt<CURLOPT_HEADERFUNCTION            , curl_write_callback         >;
constexpr auto headerdata               = data_opt<CURLOPT_HEADERDATA                                              >;
constexpr auto debugfunc                = func_opt<CURLOPT_DEBUGFUNCTION             , curl_debug_callback         >;
constexpr auto debugdata                = data_opt<CURLOPT_DEBUGDATA                                               >;
constexpr auto ssl_ctx_func             = func_opt<CURLOPT_SSL_CTX_FUNCTION          , curl_ssl_ctx_callback       >;
constexpr auto ssl_ctx_data             = data_opt<CURLOPT_SSL_CTX_DATA                                            >;
constexpr auto conv_to_network_func     = func_opt<CURLOPT_CONV_TO_NETWORK_FUNCTION  , curl_conv_callback          >;
constexpr auto conv_from_network_func   = func_opt<CURLOPT_CONV_FROM_NETWORK_FUNCTION, curl_conv_callback          >;
constexpr auto conv_from_utf8_func      = func_opt<CURLOPT_CONV_FROM_UTF8_FUNCTION   , curl_conv_callback          >;
constexpr auto interleavefunc           = func_opt<CURLOPT_INTERLEAVEFUNCTION        , curl_write_callback         >;
constexpr auto interleavedata           = data_opt<CURLOPT_INTERLEAVEDATA                                          >;
constexpr auto chunk_bgn_func           = func_opt<CURLOPT_CHUNK_BGN_FUNCTION        , curl_chunk_bgn_callback     >;
constexpr auto chunk_end_func           = func_opt<CURLOPT_CHUNK_END_FUNCTION        , curl_chunk_end_callback     >;
constexpr auto chunk_data               = data_opt<CURLOPT_CHUNK_DATA                                              >;
constexpr auto suppress_connect_headers = bool_opt<CURLOPT_SUPPRESS_CONNECT_HEADERS                                >;
constexpr auto fnmatch_func             = func_opt<CURLOPT_FNMATCH_FUNCTION          , curl_fnmatch_callback       >;
constexpr auto fnmatch_data             = data_opt<CURLOPT_FNMATCH_DATA                                            >;
constexpr auto resolver_start_func      = func_opt<CURLOPT_RESOLVER_START_FUNCTION   , curl_resolver_start_callback>;
constexpr auto resolver_start_data      = data_opt<CURLOPT_RESOLVER_START_DATA                                     >;

constexpr auto write_cb          = cb_opt<CURLOPT_WRITEFUNCTION          , CURLOPT_WRITEDATA          , curl_write_callback         >;
constexpr auto read_cb           = cb_opt<CURLOPT_READFUNCTION           , CURLOPT_READDATA           , curl_read_callback          >;
constexpr auto ioctl_cb          = cb_opt<CURLOPT_IOCTLFUNCTION          , CURLOPT_IOCTLDATA          , curl_ioctl_callback         >;
constexpr auto seek_cb           = cb_opt<CURLOPT_SEEKFUNCTION           , CURLOPT_SEEKDATA           , curl_seek_callback          >;
constexpr auto sockopt_cb        = cb_opt<CURLOPT_SOCKOPTFUNCTION        , CURLOPT_SOCKOPTDATA        , curl_sockopt_callback       >;
constexpr auto opensocket_cb     = cb_opt<CURLOPT_OPENSOCKETFUNCTION     , CURLOPT_OPENSOCKETDATA     , curl_opensocket_callback    >;
constexpr auto closesocket_cb    = cb_opt<CURLOPT_CLOSESOCKETFUNCTION    , CURLOPT_CLOSESOCKETDATA    , curl_closesocket_callback   >;
constexpr auto progress_cb       = cb_opt<CURLOPT_PROGRESSFUNCTION       , CURLOPT_PROGRESSDATA       , curl_progress_callback      >;
constexpr auto xferinfo_cb       = cb_opt<CURLOPT_XFERINFOFUNCTION       , CURLOPT_XFERINFODATA       , curl_xferinfo_callback      >;
constexpr auto header_cb         = cb_opt<CURLOPT_HEADERFUNCTION         , CURLOPT_HEADERDATA         , curl_write_callback         >;
constexpr auto debug_cb          = cb_opt<CURLOPT_DEBUGFUNCTION          , CURLOPT_DEBUGDATA          , curl_debug_callback         >;
constexpr auto ssl_ctx_cb        = cb_opt<CURLOPT_SSL_CTX_FUNCTION       , CURLOPT_SSL_CTX_DATA       , curl_ssl_ctx_callback       >;
constexpr auto interleave_cb     = cb_opt<CURLOPT_INTERLEAVEFUNCTION     , CURLOPT_INTERLEAVEDATA     , curl_write_callback         >;
constexpr auto fnmatch_cb        = cb_opt<CURLOPT_FNMATCH_FUNCTION       , CURLOPT_FNMATCH_DATA       , curl_fnmatch_callback       >;
constexpr auto resolver_start_cb = cb_opt<CURLOPT_RESOLVER_START_FUNCTION, CURLOPT_RESOLVER_START_DATA, curl_resolver_start_callback>;


// ERROR OPTIONS

constexpr auto errbuf(_buf_ auto& b)
{
    return [&b](auto& h){
        BOOST_ASSERT(buf_size(b) >= CURL_ERROR_SIZE);
        h.setopt(CURLOPT_ERRORBUFFER, const_cast<char*>(buf_data(b)));
    };
}

constexpr auto std_err               = ptr_opt <CURLOPT_STDERR, FILE*>;
constexpr auto failonerror           = bool_opt<CURLOPT_FAILONERROR>;
constexpr auto keep_sending_on_error = bool_opt<CURLOPT_KEEP_SENDING_ON_ERROR>;


// NETWORK OPTIONS

constexpr auto encoded_url = cstr_opt<CURLOPT_URL>; // assume the string is already url encoded

constexpr auto url(_str_ auto const& u)
{
    return [&u](auto& h)
    {
        h.setopt(CURLOPT_URL, const_cast<char*>(encode_url(u).c_str()));
    };
}

constexpr auto url(curlu& u)
{
    return [&u](auto& h)
    {
        h.setopt(CURLOPT_URL, const_cast<char*>(u.url().c_str()));
    };
}

constexpr auto url_ref(curlu& u)
{
    return [&u](auto& h)
    {
        h.setopt(CURLOPT_CURLU, u.handle());
    };
}

constexpr auto path_as_is           = bool_opt<CURLOPT_PATH_AS_IS>;
constexpr auto protocols            = long_opt<CURLOPT_PROTOCOLS>;
constexpr auto redir_protocols      = long_opt<CURLOPT_REDIR_PROTOCOLS>;
constexpr auto default_protocol     = cstr_opt<CURLOPT_DEFAULT_PROTOCOL>;
constexpr auto proxy                = cstr_opt<CURLOPT_PROXY>;
constexpr auto pre_proxy            = cstr_opt<CURLOPT_PRE_PROXY>;
constexpr auto proxyport            = long_opt<CURLOPT_PROXYPORT>;
constexpr auto proxytype            = long_opt<CURLOPT_PROXYTYPE>;
constexpr auto noproxy              = cstr_opt<CURLOPT_NOPROXY>;
constexpr auto httpproxytunnel      = bool_opt<CURLOPT_HTTPPROXYTUNNEL>;
constexpr auto socks5_auth          = long_opt<CURLOPT_SOCKS5_AUTH>;
constexpr auto socks5_gssapi_nec    = bool_opt<CURLOPT_SOCKS5_GSSAPI_NEC>;
constexpr auto proxy_service_name   = cstr_opt<CURLOPT_PROXY_SERVICE_NAME>;
constexpr auto haproxyprotocol      = long_opt<CURLOPT_HAPROXYPROTOCOL>;
constexpr auto service_name         = cstr_opt<CURLOPT_SERVICE_NAME>;
constexpr auto interface            = cstr_opt<CURLOPT_INTERFACE>;
constexpr auto localport            = long_opt<CURLOPT_LOCALPORT>;
constexpr auto localportrange       = long_opt<CURLOPT_LOCALPORTRANGE>;
constexpr auto dns_cache_timeout    = long_opt<CURLOPT_DNS_CACHE_TIMEOUT>;
constexpr auto doh_url              = cstr_opt<CURLOPT_DOH_URL>;
constexpr auto buffersize           = long_opt<CURLOPT_BUFFERSIZE>;
constexpr auto port                 = long_opt<CURLOPT_PORT>;
constexpr auto tcp_fastopen         = bool_opt<CURLOPT_TCP_FASTOPEN>;
constexpr auto tcp_nodelay          = bool_opt<CURLOPT_TCP_NODELAY>;
constexpr auto address_scope        = long_opt<CURLOPT_ADDRESS_SCOPE>;
constexpr auto tcp_keepalive        = bool_opt<CURLOPT_TCP_KEEPALIVE>;
constexpr auto tcp_keepidle         = unit_opt<CURLOPT_TCP_KEEPIDLE , std::chrono::seconds>;
constexpr auto tcp_keepintvl        = unit_opt<CURLOPT_TCP_KEEPINTVL, std::chrono::seconds>;
constexpr auto unix_socket_path     = cstr_opt<CURLOPT_UNIX_SOCKET_PATH>;
constexpr auto abstract_unix_socket = cstr_opt<CURLOPT_ABSTRACT_UNIX_SOCKET>;


// NAMES and PASSWORDS OPTIONS (Authentication)

constexpr auto netrc                    = bool_opt<CURLOPT_NETRC                   >;
constexpr auto netrc_file               = cstr_opt<CURLOPT_NETRC_FILE              >;
constexpr auto userpwd                  = cstr_opt<CURLOPT_USERPWD                 >;
constexpr auto proxyuserpwd             = cstr_opt<CURLOPT_PROXYUSERPWD            >;
constexpr auto username                 = cstr_opt<CURLOPT_USERNAME                >;
constexpr auto password                 = cstr_opt<CURLOPT_PASSWORD                >;
constexpr auto login_options            = cstr_opt<CURLOPT_LOGIN_OPTIONS           >;
constexpr auto proxyusername            = cstr_opt<CURLOPT_PROXYUSERNAME           >;
constexpr auto proxypassword            = cstr_opt<CURLOPT_PROXYPASSWORD           >;
constexpr auto httpauth                 = long_opt<CURLOPT_HTTPAUTH                >;
constexpr auto tlsauth_username         = cstr_opt<CURLOPT_TLSAUTH_USERNAME        >;
constexpr auto proxy_tlsauth_username   = cstr_opt<CURLOPT_PROXY_TLSAUTH_USERNAME  >;
constexpr auto tlsauth_password         = cstr_opt<CURLOPT_TLSAUTH_PASSWORD        >;
constexpr auto proxy_tlsauth_password   = cstr_opt<CURLOPT_PROXY_TLSAUTH_PASSWORD  >;
constexpr auto tlsauth_type             = cstr_opt<CURLOPT_TLSAUTH_TYPE            >;
constexpr auto proxy_tlsauth_type       = cstr_opt<CURLOPT_PROXY_TLSAUTH_TYPE      >;
constexpr auto proxyauth                = long_opt<CURLOPT_PROXYAUTH               >;
constexpr auto sasl_authzid             = cstr_opt<CURLOPT_SASL_AUTHZID            >;
constexpr auto sasl_ir                  = bool_opt<CURLOPT_SASL_IR                 >;
constexpr auto xoauth2_bearer           = cstr_opt<CURLOPT_XOAUTH2_BEARER          >;
constexpr auto disallow_username_in_url = bool_opt<CURLOPT_DISALLOW_USERNAME_IN_URL>;


// HTTP OPTIONS

constexpr auto autoreferer       = bool_opt<CURLOPT_AUTOREFERER      >;
constexpr auto accept_encoding   = cstr_opt<CURLOPT_ACCEPT_ENCODING  >;
constexpr auto transfer_encoding = bool_opt<CURLOPT_TRANSFER_ENCODING>;
constexpr auto followlocation    = bool_opt<CURLOPT_FOLLOWLOCATION   >;
constexpr auto unrestricted_auth = bool_opt<CURLOPT_UNRESTRICTED_AUTH>;
constexpr auto maxredirs         = long_opt<CURLOPT_MAXREDIRS        >;
constexpr auto postredir         = long_opt<CURLOPT_POSTREDIR        >;

constexpr auto customrequest     = cstr_opt<CURLOPT_CUSTOMREQUEST    >; // change the http verb, but keep the underlying behavior(CURLOPT_HTTPGET, CURLOPT_POST)
constexpr auto httpget           = bool_opt<CURLOPT_HTTPGET          >; // GET (the default option), no request body
constexpr auto nobody            = bool_opt<CURLOPT_NOBODY           >; // HEAD request, no request body
constexpr auto post              = bool_opt<CURLOPT_POST             >; // POST, can have request body, set "Expect: 100-continue" when body size > 1kb for HTTP 1.1 and set "Content-Type: application/x-www-form-urlencoded" by default

template<bool Copy = false>
constexpr auto postfields(_byte_buf_ auto const& b)
{
    return [&b](auto& h){
        h.setopt(CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(buf_size(b)));
        h.setopt(Copy ? CURLOPT_COPYPOSTFIELDS : CURLOPT_POSTFIELDS, const_cast<char*>(buf_data(b)));
    };
}

constexpr auto copypostfields(_byte_buf_ auto const& b)
{
    return postfields<true>(b);
}

constexpr auto referer                = cstr_opt  <CURLOPT_REFERER                                           >;
constexpr auto useragent              = cstr_opt  <CURLOPT_USERAGENT                                         >;
constexpr auto httpheader             = handle_opt<CURLOPT_HTTPHEADER            , curlist                   >;
constexpr auto headeropt              = long_opt  <CURLOPT_HEADEROPT                                         >;
constexpr auto proxyheader            = handle_opt<CURLOPT_PROXYHEADER           , curlist                   >;
constexpr auto http200aliases         = handle_opt<CURLOPT_HTTP200ALIASES        , curlist                   >;
constexpr auto cookie                 = cstr_opt  <CURLOPT_COOKIE                                            >;
constexpr auto cookiefile             = cstr_opt  <CURLOPT_COOKIEFILE                                        >;
constexpr auto cookiejar              = cstr_opt  <CURLOPT_COOKIEJAR                                         >;
constexpr auto cookiesession          = bool_opt  <CURLOPT_COOKIESESSION                                     >;
constexpr auto cookielist             = cstr_opt  <CURLOPT_COOKIELIST                                        >;
constexpr auto altsvc                 = cstr_opt  <CURLOPT_ALTSVC                                            >;
constexpr auto altsvc_ctrl            = long_opt  <CURLOPT_ALTSVC_CTRL                                       >;
constexpr auto request_target         = cstr_opt  <CURLOPT_REQUEST_TARGET                                    >;
constexpr auto http_version           = long_opt  <CURLOPT_HTTP_VERSION                                      >;
constexpr auto http09_allowed         = bool_opt  <CURLOPT_HTTP09_ALLOWED                                    >;
constexpr auto ignore_content_length  = bool_opt  <CURLOPT_IGNORE_CONTENT_LENGTH                             >;
constexpr auto http_content_decoding  = bool_opt  <CURLOPT_HTTP_CONTENT_DECODING                             >;
constexpr auto http_transfer_decoding = bool_opt  <CURLOPT_HTTP_TRANSFER_DECODING                            >;
constexpr auto expect_100_timeout     = unit_opt  <CURLOPT_EXPECT_100_TIMEOUT_MS  , std::chrono::milliseconds>;
constexpr auto trailerfunction        = func_opt  <CURLOPT_TRAILERFUNCTION        , curl_trailer_callback    >;
constexpr auto trailerdata            = data_opt  <CURLOPT_TRAILERDATA                                       >;
constexpr auto pipewait               = bool_opt  <CURLOPT_PIPEWAIT                                          >;
constexpr auto stream_depends         = ptr_opt   <CURLOPT_STREAM_DEPENDS         , CURL*                    >;
constexpr auto stream_depends_e       = ptr_opt   <CURLOPT_STREAM_DEPENDS_E       , CURL*                    >;
constexpr auto stream_weight          = long_opt  <CURLOPT_STREAM_WEIGHT                                     >;


// SMTP OPTIONS

constexpr auto mail_from             = cstr_opt  <CURLOPT_MAIL_FROM                     >;
constexpr auto mail_rcpt             = handle_opt<CURLOPT_MAIL_RCPT            , curlist>;
constexpr auto mail_auth             = cstr_opt  <CURLOPT_MAIL_AUTH                     >;
// constexpr auto mail_rcpt_alllowfails = bool_opt  <CURLOPT_MAIL_RCPT_ALLLOWFAILS         >;


// TFTP OPTIONS

constexpr auto tftp_blksize    = long_opt<CURLOPT_TFTP_BLKSIZE   >;
constexpr auto tftp_no_options = bool_opt<CURLOPT_TFTP_NO_OPTIONS>;


// FTP OPTIONS

constexpr auto ftpport                 = cstr_opt  <CURLOPT_FTPPORT                                      >;
constexpr auto quote                   = handle_opt<CURLOPT_QUOTE                  , curlist             >;
constexpr auto postquote               = handle_opt<CURLOPT_POSTQUOTE              , curlist             >;
constexpr auto prequote                = handle_opt<CURLOPT_PREQUOTE               , curlist             >;
constexpr auto append                  = bool_opt  <CURLOPT_APPEND                                       >;
constexpr auto ftp_use_eprt            = bool_opt  <CURLOPT_FTP_USE_EPRT                                 >;
constexpr auto ftp_use_epsv            = bool_opt  <CURLOPT_FTP_USE_EPSV                                 >;
constexpr auto ftp_use_pret            = bool_opt  <CURLOPT_FTP_USE_PRET                                 >;
constexpr auto ftp_create_missing_dirs = long_opt  <CURLOPT_FTP_CREATE_MISSING_DIRS                      >;
constexpr auto ftp_response_timeout    = unit_opt  <CURLOPT_FTP_RESPONSE_TIMEOUT   , std::chrono::seconds>;
constexpr auto ftp_alternative_to_user = cstr_opt  <CURLOPT_FTP_ALTERNATIVE_TO_USER                      >;
constexpr auto ftp_skip_pasv_ip        = bool_opt  <CURLOPT_FTP_SKIP_PASV_IP                             >;
constexpr auto ftpsslauth              = long_opt  <CURLOPT_FTPSSLAUTH                                   >;
constexpr auto ftp_ssl_ccc             = long_opt  <CURLOPT_FTP_SSL_CCC                                  >;
constexpr auto ftp_account             = cstr_opt  <CURLOPT_FTP_ACCOUNT                                  >;
constexpr auto ftp_filemethod          = long_opt  <CURLOPT_FTP_FILEMETHOD                               >;


// RTSP OPTIONS

constexpr auto rtsp_request     = long_opt<CURLOPT_RTSP_REQUEST    >;
constexpr auto rtsp_session_id  = cstr_opt<CURLOPT_RTSP_SESSION_ID >;
constexpr auto rtsp_stream_uri  = cstr_opt<CURLOPT_RTSP_STREAM_URI >;
constexpr auto rtsp_transport   = cstr_opt<CURLOPT_RTSP_TRANSPORT  >;
constexpr auto rtsp_client_cseq = long_opt<CURLOPT_RTSP_CLIENT_CSEQ>;
constexpr auto rtsp_server_cseq = long_opt<CURLOPT_RTSP_SERVER_CSEQ>;


// PROTOCOL OPTIONS

constexpr auto transfertext        = bool_opt<CURLOPT_TRANSFERTEXT                    >;
constexpr auto proxy_transfer_mode = bool_opt<CURLOPT_PROXY_TRANSFER_MODE             >;
constexpr auto crlf                = bool_opt<CURLOPT_CRLF                            >;
constexpr auto range               = cstr_opt<CURLOPT_RANGE                           >;
constexpr auto resume_from         = offt_opt<CURLOPT_RESUME_FROM_LARGE               >;
constexpr auto filetime            = bool_opt<CURLOPT_FILETIME                        >;
constexpr auto dirlistonly         = bool_opt<CURLOPT_DIRLISTONLY                     >;
constexpr auto infilesize          = offt_opt<CURLOPT_INFILESIZE_LARGE                >;
constexpr auto upload              = bool_opt<CURLOPT_UPLOAD                          >;
constexpr auto upload_buffersize   = long_opt<CURLOPT_UPLOAD_BUFFERSIZE               >;
constexpr auto mimepost            = ptr_opt <CURLOPT_MIMEPOST            , curl_mime*>;
constexpr auto maxfilesize         = offt_opt<CURLOPT_MAXFILESIZE_LARGE               >;
constexpr auto timecondition       = long_opt<CURLOPT_TIMECONDITION                   >;
constexpr auto timevalue           = long_opt<CURLOPT_TIMEVALUE                       >;
constexpr auto timevalue_large     = offt_opt<CURLOPT_TIMEVALUE_LARGE                 >;


// CONNECTION OPTIONS

constexpr auto timeout                = unit_opt  <CURLOPT_TIMEOUT_MS               , std::chrono::milliseconds>;
constexpr auto low_speed_limit        = unit_opt  <CURLOPT_LOW_SPEED_LIMIT          , _B_<long>                >;
constexpr auto low_speed_time         = unit_opt  <CURLOPT_LOW_SPEED_TIME           , std::chrono::seconds     >;
constexpr auto max_send_speed         = lunit_opt <CURLOPT_MAX_SEND_SPEED_LARGE     , _B_<curl_off_t>          >;
constexpr auto max_recv_speed         = lunit_opt <CURLOPT_MAX_RECV_SPEED_LARGE     , _B_<curl_off_t>          >;
constexpr auto maxconnects            = long_opt  <CURLOPT_MAXCONNECTS                                         >;
constexpr auto fresh_connect          = bool_opt  <CURLOPT_FRESH_CONNECT                                       >;
constexpr auto forbid_reuse           = bool_opt  <CURLOPT_FORBID_REUSE                                        >;
constexpr auto maxage_conn            = unit_opt  <CURLOPT_MAXAGE_CONN              , std::chrono::seconds     >;
constexpr auto connecttimeout         = unit_opt  <CURLOPT_CONNECTTIMEOUT_MS        , std::chrono::milliseconds>;
constexpr auto ipresolve              = long_opt  <CURLOPT_IPRESOLVE                                           >;
constexpr auto connect_only           = bool_opt  <CURLOPT_CONNECT_ONLY                                        >;
constexpr auto use_ssl                = long_opt  <CURLOPT_USE_SSL                                             >;
constexpr auto resolve                = handle_opt<CURLOPT_RESOLVE                  , curlist                  >;
constexpr auto dns_interface          = cstr_opt  <CURLOPT_DNS_INTERFACE                                       >;
constexpr auto dns_local_ip4          = cstr_opt  <CURLOPT_DNS_LOCAL_IP4                                       >;
constexpr auto dns_local_ip6          = cstr_opt  <CURLOPT_DNS_LOCAL_IP6                                       >;
constexpr auto dns_servers            = cstr_opt  <CURLOPT_DNS_SERVERS                                         >;
constexpr auto dns_shuffle_addresses  = bool_opt  <CURLOPT_DNS_SHUFFLE_ADDRESSES                               >;
constexpr auto accepttimeout          = unit_opt  <CURLOPT_ACCEPTTIMEOUT_MS         , std::chrono::milliseconds>;
constexpr auto happy_eyeballs_timeout = unit_opt  <CURLOPT_HAPPY_EYEBALLS_TIMEOUT_MS, std::chrono::milliseconds>;


// SSL and SECURITY OPTIONS

constexpr auto sslcert               = cstr_opt <CURLOPT_SSLCERT              >;
// constexpr auto sslcert_blob          = cstr_opt <CURLOPT_SSLCERT_BLOB         >;
constexpr auto proxy_sslcert         = cstr_opt <CURLOPT_PROXY_SSLCERT        >;
// constexpr auto proxy_sslcert_blob    = cstr_opt <CURLOPT_PROXY_SSLCERT_BLOB   >;
constexpr auto sslcerttype           = cstr_opt <CURLOPT_SSLCERTTYPE          >;
constexpr auto proxy_sslcerttype     = cstr_opt <CURLOPT_PROXY_SSLCERTTYPE    >;
constexpr auto sslkey                = cstr_opt <CURLOPT_SSLKEY               >;
// constexpr auto sslkey_blob           = cstr_opt <CURLOPT_SSLKEY_BLOB          >;
constexpr auto proxy_sslkey          = cstr_opt <CURLOPT_PROXY_SSLKEY         >;
// constexpr auto proxy_sslkey_blob     = cstr_opt <CURLOPT_PROXY_SSLKEY_BLOB    >;
constexpr auto sslkeytype            = cstr_opt <CURLOPT_SSLKEYTYPE           >;
constexpr auto proxy_sslkeytype      = cstr_opt <CURLOPT_PROXY_SSLKEYTYPE     >;
constexpr auto keypasswd             = cstr_opt <CURLOPT_KEYPASSWD            >;
constexpr auto proxy_keypasswd       = cstr_opt <CURLOPT_PROXY_KEYPASSWD      >;
constexpr auto ssl_enable_alpn       = bool_opt <CURLOPT_SSL_ENABLE_ALPN      >;
constexpr auto ssl_enable_npn        = bool_opt <CURLOPT_SSL_ENABLE_NPN       >;
constexpr auto sslengine             = cstr_opt <CURLOPT_SSLENGINE            >;
constexpr auto sslengine_default     = bool_opt <CURLOPT_SSLENGINE_DEFAULT    >;
constexpr auto ssl_falsestart        = bool_opt <CURLOPT_SSL_FALSESTART       >;
constexpr auto sslversion            = long_opt <CURLOPT_SSLVERSION           >;
constexpr auto proxy_sslversion      = long_opt <CURLOPT_PROXY_SSLVERSION     >;
constexpr auto ssl_verifyhost        = long_opt <CURLOPT_SSL_VERIFYHOST       >;
constexpr auto proxy_ssl_verifyhost  = long_opt <CURLOPT_PROXY_SSL_VERIFYHOST >;
constexpr auto ssl_verifypeer        = bool_opt <CURLOPT_SSL_VERIFYPEER       >;
constexpr auto proxy_ssl_verifypeer  = bool_opt <CURLOPT_PROXY_SSL_VERIFYPEER >;
constexpr auto ssl_verifystatus      = bool_opt <CURLOPT_SSL_VERIFYSTATUS     >;
constexpr auto cainfo                = cstr_opt <CURLOPT_CAINFO               >;
constexpr auto proxy_cainfo          = cstr_opt <CURLOPT_PROXY_CAINFO         >;
constexpr auto issuercert            = cstr_opt <CURLOPT_ISSUERCERT           >;
// constexpr auto issuercert_blob       = cstr_opt <CURLOPT_ISSUERCERT_BLOB      >;
// constexpr auto proxy_issuercert      = cstr_opt <CURLOPT_PROXY_ISSUERCERT     >;
// constexpr auto proxy_issuercert_blob = cstr_opt <CURLOPT_PROXY_ISSUERCERT_BLOB>;
constexpr auto capath                = cstr_opt <CURLOPT_CAPATH               >;
constexpr auto proxy_capath          = cstr_opt <CURLOPT_PROXY_CAPATH         >;
constexpr auto crlfile               = cstr_opt <CURLOPT_CRLFILE              >;
constexpr auto proxy_crlfile         = cstr_opt <CURLOPT_PROXY_CRLFILE        >;
constexpr auto certinfo              = bool_opt <CURLOPT_CERTINFO             >;
constexpr auto pinnedpublickey       = cstr_opt <CURLOPT_PINNEDPUBLICKEY      >;
constexpr auto proxy_pinnedpublickey = cstr_opt <CURLOPT_PROXY_PINNEDPUBLICKEY>;
constexpr auto random_file           = cstr_opt <CURLOPT_RANDOM_FILE          >;
constexpr auto egdsocket             = cstr_opt <CURLOPT_EGDSOCKET            >;
constexpr auto ssl_cipher_list       = cstr_opt <CURLOPT_SSL_CIPHER_LIST      >;
constexpr auto proxy_ssl_cipher_list = cstr_opt <CURLOPT_PROXY_SSL_CIPHER_LIST>;
constexpr auto tls13_ciphers         = cstr_opt <CURLOPT_TLS13_CIPHERS        >;
constexpr auto proxy_tls13_ciphers   = cstr_opt <CURLOPT_PROXY_TLS13_CIPHERS  >;
constexpr auto ssl_sessionid_cache   = bool_opt <CURLOPT_SSL_SESSIONID_CACHE  >;
constexpr auto ssl_options           = long_opt <CURLOPT_SSL_OPTIONS          >;
constexpr auto proxy_ssl_options     = long_opt <CURLOPT_PROXY_SSL_OPTIONS    >;
constexpr auto krblevel              = cstr_opt <CURLOPT_KRBLEVEL             >;
constexpr auto gssapi_delegation     = long_opt <CURLOPT_GSSAPI_DELEGATION    >;


// SSH OPTIONS

constexpr auto ssh_auth_types          = long_opt<CURLOPT_SSH_AUTH_TYPES                              >;
constexpr auto ssh_compression         = bool_opt<CURLOPT_SSH_COMPRESSION                             >;
constexpr auto ssh_host_public_key_md5 = cstr_opt<CURLOPT_SSH_HOST_PUBLIC_KEY_MD5                     >;
constexpr auto ssh_public_keyfile      = cstr_opt<CURLOPT_SSH_PUBLIC_KEYFILE                          >;
constexpr auto ssh_private_keyfile     = cstr_opt<CURLOPT_SSH_PRIVATE_KEYFILE                         >;
constexpr auto ssh_knownhosts          = cstr_opt<CURLOPT_SSH_KNOWNHOSTS                              >;
constexpr auto ssh_keyfunction         = func_opt<CURLOPT_SSH_KEYFUNCTION        , curl_sshkeycallback>;
constexpr auto ssh_keydata             = data_opt<CURLOPT_SSH_KEYDATA                                 >;


// OTHER OPTIONS

constexpr auto priv_data           = data_opt  <CURLOPT_PRIVATE                        >;
constexpr auto share               = handle_opt<CURLOPT_SHARE              , curl_share>;
constexpr auto new_file_perms      = long_opt  <CURLOPT_NEW_FILE_PERMS                 >;
constexpr auto new_directory_perms = long_opt  <CURLOPT_NEW_DIRECTORY_PERMS            >;


// TELNET OPTIONS

constexpr auto telnetoptions = handle_opt<CURLOPT_TELNETOPTIONS, curlist>;


} // namespace jkl::curlopt



namespace jkl::curlmopt{


constexpr auto chunk_length_penalty_size   = curlopt::long_opt<CURLMOPT_CHUNK_LENGTH_PENALTY_SIZE                             >;
constexpr auto content_length_penalty_size = curlopt::long_opt<CURLMOPT_CONTENT_LENGTH_PENALTY_SIZE                           >;
constexpr auto max_host_connections        = curlopt::long_opt<CURLMOPT_MAX_HOST_CONNECTIONS                                  >;
constexpr auto max_pipeline_length         = curlopt::long_opt<CURLMOPT_MAX_PIPELINE_LENGTH                                   >;
constexpr auto max_total_connections       = curlopt::long_opt<CURLMOPT_MAX_TOTAL_CONNECTIONS                                 >;
constexpr auto maxconnects                 = curlopt::long_opt<CURLMOPT_MAXCONNECTS                                           >;
constexpr auto pipelining                  = curlopt::long_opt<CURLMOPT_PIPELINING                                            >;
constexpr auto pushfunction                = curlopt::func_opt<CURLMOPT_PUSHFUNCTION               , curl_push_callback       >;
constexpr auto pushdata                    = curlopt::data_opt<CURLMOPT_PUSHDATA                                              >;
constexpr auto socketfunction              = curlopt::func_opt<CURLMOPT_SOCKETFUNCTION             , curl_socket_callback     >;
constexpr auto socketdata                  = curlopt::data_opt<CURLMOPT_SOCKETDATA                                            >;
constexpr auto timerfunction               = curlopt::func_opt<CURLMOPT_TIMERFUNCTION              , curl_multi_timer_callback>;
constexpr auto timerdata                   = curlopt::data_opt<CURLMOPT_TIMERDATA                                             >;
constexpr auto max_concurrent_streams      = curlopt::long_opt<CURLMOPT_MAX_CONCURRENT_STREAMS                                >;


constexpr auto push_cb   = curlopt::cb_opt<CURLMOPT_PUSHFUNCTION  , CURLMOPT_PUSHDATA  , curl_push_callback       >;
constexpr auto socket_cb = curlopt::cb_opt<CURLMOPT_SOCKETFUNCTION, CURLMOPT_SOCKETDATA, curl_socket_callback     >;
constexpr auto timer_cb  = curlopt::cb_opt<CURLMOPT_TIMERFUNCTION , CURLMOPT_TIMERDATA , curl_multi_timer_callback>;


} // namespace jkl::curlmopt


namespace jkl::curlshopt{


constexpr auto lockfunc   = curlopt::func_opt<CURLSHOPT_LOCKFUNC  , curl_lock_function  >;
constexpr auto unlockfunc = curlopt::func_opt<CURLSHOPT_UNLOCKFUNC, curl_unlock_function>;
constexpr auto userdata   = curlopt::data_opt<CURLSHOPT_USERDATA                        >;
constexpr auto share      = curlopt::long_opt<CURLSHOPT_SHARE                           >;
constexpr auto unshare    = curlopt::long_opt<CURLSHOPT_UNSHARE                         >;


} // namespace jkl::curlshopt
