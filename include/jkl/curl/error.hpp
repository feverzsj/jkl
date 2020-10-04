#pragma once

#include <jkl/config.hpp>
#include <jkl/error.hpp>
#include <curl/curl.h>


namespace jkl{


// CURLcode

class curl_err_category : public aerror_category
{
public:
	char const* name() const noexcept override { return "curl_easy"; }
	std::string message(int rc) const override { return curl_easy_strerror(static_cast<CURLcode>(rc)); }
};

inline constexpr curl_err_category g_curl_error_cat;
inline auto const& curl_err_cat() noexcept { return g_curl_error_cat; }


// CURLMcode

class curlm_err_category : public aerror_category
{
public:
    char const* name() const noexcept override { return "curl_multi"; }
    std::string message(int rc) const override { return curl_multi_strerror(static_cast<CURLMcode>(rc)); }
};

inline constexpr curlm_err_category g_curlm_error_cat;
inline auto const& curlm_err_cat() noexcept { return g_curlm_error_cat; }


// CURLSHcode

class curlsh_err_category : public aerror_category
{
public:
    char const* name() const noexcept override { return "curl_share"; }

    std::string message(int rc) const override
    {
        switch(static_cast<CURLSHcode>(rc))
        {
            case CURLSHE_BAD_OPTION  : return "bad option";
            case CURLSHE_IN_USE      : return "in use";
            case CURLSHE_INVALID     : return "invalid";
            case CURLSHE_NOMEM       : return "out of memory";
            case CURLSHE_NOT_BUILT_IN: return "feature not present in lib";
            default                  : return "undefined";
        }
    }
};

inline constexpr curlsh_err_category g_curlsh_error_cat;
inline auto const& curlsh_err_cat() noexcept { return g_curlsh_error_cat; }


// CURLUcode

class curlu_err_category : public aerror_category
{
public:
    char const* name() const noexcept override { return "curl_url"; }

    std::string message(int rc) const override
    {
        switch(static_cast<CURLUcode>(rc))
        {
            case CURLUE_BAD_HANDLE        : return "bad handle";
            case CURLUE_BAD_PARTPOINTER   : return "bad partpointer";
            case CURLUE_MALFORMED_INPUT   : return "malformed input";
            case CURLUE_BAD_PORT_NUMBER   : return "bad port number";
            case CURLUE_UNSUPPORTED_SCHEME: return "unsupported scheme";
            case CURLUE_URLDECODE         : return "url decode error";
            case CURLUE_OUT_OF_MEMORY     : return "out of memory";
            case CURLUE_USER_NOT_ALLOWED  : return "user not allowed";
            case CURLUE_UNKNOWN_PART      : return "unknown part";
            case CURLUE_NO_SCHEME         : return "no scheme";
            case CURLUE_NO_USER           : return "no user";
            case CURLUE_NO_PASSWORD       : return "no password";
            case CURLUE_NO_OPTIONS        : return "no options";
            case CURLUE_NO_HOST           : return "no host";
            case CURLUE_NO_PORT           : return "no port";
            case CURLUE_NO_QUERY          : return "no query";
            case CURLUE_NO_FRAGMENT       : return "no fragment";
            default                       : return "undefined";
        }
    }
};

inline constexpr curlu_err_category g_curlu_error_cat;
inline auto const& curlu_err_cat() noexcept { return g_curlu_error_cat; }


} // namespace jkl


namespace boost::system{

template<> struct is_error_code_enum<CURLcode> : public true_type {};
template<> struct is_error_code_enum<CURLMcode> : public true_type {};
template<> struct is_error_code_enum<CURLSHcode> : public true_type {};
template<> struct is_error_code_enum<CURLUcode> : public true_type {};

} // namespace boost::system


// must define in global, as the enums are defined in global, otherwise ADL won't find them.

inline jkl::aerror_code      make_error_code(CURLcode e) noexcept { return {static_cast<int>(e), jkl::curl_err_cat()}; }
inline jkl::aerror_condition make_error_condition(CURLcode e) noexcept { return {static_cast<int>(e), jkl::curl_err_cat()}; }

inline jkl::aerror_code      make_error_code(CURLMcode e) noexcept { return {static_cast<int>(e), jkl::curlm_err_cat()}; }
inline jkl::aerror_condition make_error_condition(CURLMcode e) noexcept { return {static_cast<int>(e), jkl::curlm_err_cat()}; }

inline jkl::aerror_code      make_error_code(CURLSHcode e) noexcept { return {static_cast<int>(e), jkl::curlsh_err_cat()}; }
inline jkl::aerror_condition make_error_condition(CURLSHcode e) noexcept { return {static_cast<int>(e), jkl::curlsh_err_cat()}; }

inline jkl::aerror_code      make_error_code(CURLUcode e) noexcept { return {static_cast<int>(e), jkl::curlu_err_cat()}; }
inline jkl::aerror_condition make_error_condition(CURLUcode e) noexcept { return {static_cast<int>(e), jkl::curlu_err_cat()}; }
