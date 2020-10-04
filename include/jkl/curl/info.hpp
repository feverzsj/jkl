#pragma once

#include <jkl/config.hpp>
#include <jkl/util/str.hpp>
#include <jkl/curl/list.hpp>
#include <curl/curl.h>


namespace jkl::curlinfo{


template<CURLINFO K, class V>    
struct info_t
{
    static constexpr CURLINFO key = K;
    using value_type = V;
};

template<CURLINFO K, class V>    
inline constexpr auto info_c = info_t<K, V>();

template<class T>
inline constexpr auto priv_data               = info_c<CURLINFO_PRIVATE                  , T                   >;
inline constexpr auto effective_url           = info_c<CURLINFO_EFFECTIVE_URL            , char const*         >;
inline constexpr auto response_code           = info_c<CURLINFO_RESPONSE_CODE            , long                >;
inline constexpr auto size_upload             = info_c<CURLINFO_SIZE_UPLOAD_T            , curl_off_t          >;
inline constexpr auto size_download           = info_c<CURLINFO_SIZE_DOWNLOAD_T          , curl_off_t          >;
inline constexpr auto speed_download          = info_c<CURLINFO_SPEED_DOWNLOAD_T         , curl_off_t          >;
inline constexpr auto speed_upload            = info_c<CURLINFO_SPEED_UPLOAD_T           , curl_off_t          >;
inline constexpr auto header_size             = info_c<CURLINFO_HEADER_SIZE              , long                >;
inline constexpr auto request_size            = info_c<CURLINFO_REQUEST_SIZE             , long                >;
inline constexpr auto ssl_verifyresult        = info_c<CURLINFO_SSL_VERIFYRESULT         , long                >;
inline constexpr auto filetime                = info_c<CURLINFO_FILETIME_T               , curl_off_t          >;
inline constexpr auto content_length_download = info_c<CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, curl_off_t          >;
inline constexpr auto content_length_upload   = info_c<CURLINFO_CONTENT_LENGTH_UPLOAD_T  , curl_off_t          >;
inline constexpr auto content_type            = info_c<CURLINFO_CONTENT_TYPE             , char const*         >;
inline constexpr auto redirect_count          = info_c<CURLINFO_REDIRECT_COUNT           , long                >;
inline constexpr auto http_connectcode        = info_c<CURLINFO_HTTP_CONNECTCODE         , long                >;
inline constexpr auto httpauth_avail          = info_c<CURLINFO_HTTPAUTH_AVAIL           , long                >;
inline constexpr auto proxyauth_avail         = info_c<CURLINFO_PROXYAUTH_AVAIL          , long                >;
inline constexpr auto os_errno                = info_c<CURLINFO_OS_ERRNO                 , long                >;
inline constexpr auto num_connects            = info_c<CURLINFO_NUM_CONNECTS             , long                >;
inline constexpr auto ssl_engines             = info_c<CURLINFO_SSL_ENGINES              , curl_slist*         >;
inline constexpr auto cookielist              = info_c<CURLINFO_COOKIELIST               , curl_slist*         >;
inline constexpr auto lastsocket              = info_c<CURLINFO_LASTSOCKET               , long                >;
inline constexpr auto ftp_entry_path          = info_c<CURLINFO_FTP_ENTRY_PATH           , char const*         >;
inline constexpr auto redirect_url            = info_c<CURLINFO_REDIRECT_URL             , char const*         >;
inline constexpr auto primary_ip              = info_c<CURLINFO_PRIMARY_IP               , char const*         >;
inline constexpr auto certinfo                = info_c<CURLINFO_CERTINFO                 , curl_certinfo       >;
inline constexpr auto condition_unmet         = info_c<CURLINFO_CONDITION_UNMET          , long                >;
inline constexpr auto rtsp_session_id         = info_c<CURLINFO_RTSP_SESSION_ID          , char const*         >;
inline constexpr auto rtsp_client_cseq        = info_c<CURLINFO_RTSP_CLIENT_CSEQ         , long                >;
inline constexpr auto rtsp_server_cseq        = info_c<CURLINFO_RTSP_SERVER_CSEQ         , long                >;
inline constexpr auto rtsp_cseq_recv          = info_c<CURLINFO_RTSP_CSEQ_RECV           , long                >;
inline constexpr auto primary_port            = info_c<CURLINFO_PRIMARY_PORT             , long                >;
inline constexpr auto local_ip                = info_c<CURLINFO_LOCAL_IP                 , char const*         >;
inline constexpr auto local_port              = info_c<CURLINFO_LOCAL_PORT               , long                >;
inline constexpr auto activesocket            = info_c<CURLINFO_ACTIVESOCKET             , curl_socket_t       >;
inline constexpr auto tls_ssl_ptr             = info_c<CURLINFO_TLS_SSL_PTR              , curl_tlssessioninfo*>;
inline constexpr auto http_version            = info_c<CURLINFO_HTTP_VERSION             , long                >;
inline constexpr auto proxy_ssl_verifyresult  = info_c<CURLINFO_PROXY_SSL_VERIFYRESULT   , long                >;
inline constexpr auto protocol                = info_c<CURLINFO_PROTOCOL                 , long                >;
inline constexpr auto scheme                  = info_c<CURLINFO_SCHEME                   , char const*         >;
inline constexpr auto total_time              = info_c<CURLINFO_TOTAL_TIME_T             , curl_off_t          >;
inline constexpr auto namelookup_time         = info_c<CURLINFO_NAMELOOKUP_TIME_T        , curl_off_t          >;
inline constexpr auto connect_time            = info_c<CURLINFO_CONNECT_TIME_T           , curl_off_t          >;
inline constexpr auto pretransfer_time        = info_c<CURLINFO_PRETRANSFER_TIME_T       , curl_off_t          >;
inline constexpr auto starttransfer_time      = info_c<CURLINFO_STARTTRANSFER_TIME_T     , curl_off_t          >;
inline constexpr auto redirect_time           = info_c<CURLINFO_REDIRECT_TIME_T          , curl_off_t          >;
inline constexpr auto appconnect_time         = info_c<CURLINFO_APPCONNECT_TIME_T        , curl_off_t          >;
inline constexpr auto retry_after             = info_c<CURLINFO_RETRY_AFTER              , curl_off_t          >;


} // namespace jkl::curlinfo