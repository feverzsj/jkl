#pragma once

#include <jkl/config.hpp>
#include <jkl/result.hpp>
#include <jkl/util/handle.hpp>
#include <jkl/curl/str.hpp>
#include <jkl/curl/info.hpp>
#include <jkl/curl/error.hpp>
#include <curl/curl.h>


namespace jkl{


class curl_global
{
public:
    curl_global(long f = CURL_GLOBAL_WIN32|CURL_GLOBAL_SSL)
    {
        curl_global_init(f);
    }

    ~curl_global()
    {
        curl_global_cleanup();
    }
};

inline curl_global g_curl_global; // user should not define curl_global any more


class curl_easy
{
    struct deleter{ void operator()(CURL* h) noexcept { curl_easy_cleanup(h); } };
    struct copier { CURL* operator()(CURL* h) { return curl_easy_duphandle(h); } };

    copyable_handle<CURL*, deleter, copier> _h;

public:
    curl_easy() : _h(curl_easy_init()) {}
    explicit curl_easy(CURL* h) : _h(h) {}

    auto handle() const noexcept { return _h.get(); }
    auto release() noexcept { return _h.release(); }

    explicit operator bool() const noexcept { return _h.operator bool(); }
    bool operator==(curl_easy const& r) const noexcept { return handle() == r.handle(); }
    bool operator!=(curl_easy const& r) const noexcept { return handle() != r.handle(); }
    bool operator< (curl_easy const& r) const noexcept { return handle() <  r.handle(); }
    bool operator> (curl_easy const& r) const noexcept { return handle() >  r.handle(); }
    bool operator<=(curl_easy const& r) const noexcept { return handle() <= r.handle(); }
    bool operator>=(curl_easy const& r) const noexcept { return handle() >= r.handle(); }

    template<class T>
    void setopt(CURLoption k, T v)
    {
        throw_on_error(curl_easy_setopt(handle(), k, v));
    }

    template<class T>
    void setopt(T&& opt)
    {
        std::forward<T>(opt)(*this);
    }

    template<class... T>
    void setopts(T&&... opts)
    {
        (..., setopt(std::forward<T>(opts)));
    }

    void reset_opts() { curl_easy_reset(handle()); }

    template<class T>
    void getinfo(CURLINFO k, T& v)
    {
        throw_on_error(curl_easy_getinfo(handle(), k, &v));
    }

    template<class Info>
    auto info(Info const&)
    {
        typename Info::value_type v;
        getinfo(Info::key, v);
        return v;
    }

    template<class T>
    T priv_data()
    {
        return info(curlinfo::priv_data<T>);
    }

    // Currently the only protocol with a connection upkeep mechanism is HTTP/2:
    // when the connection upkeep interval is exceeded and curl_easy_upkeep is called,
    // an HTTP/2 PING frame is sent on the connection. 
    void upkeep() { throw_on_error(curl_easy_upkeep(handle())); }

    aresult<> perform() noexcept
    {
        return curl_easy_perform(handle());
    }

    // you must set both state in once.
    aresult<> pause(int mask)
    {
        return curl_easy_pause(handle(), mask);
    }

    // the member urlencode/urlencode use registered conv callback if exists,
    // the free curlencode/curlencode use iconv

    // return url encoded s
    template<class Str>
    curlstr urlencode(Str const& s)
    {
        return {curl_easy_escape(handle(), str_data(s), static_cast<int>(str_size(s)))};
    }

    template<class Str>
    curlstr urldecode(Str const& s)
    {
        return {curl_easy_unescape(handle(), str_data(s), static_cast<int>(str_size(s)))};
    }
};


} // namespace jkl


namespace std {

template<> struct hash<jkl::curl_easy>
{
    auto operator()(jkl::curl_easy const& e) const noexcept
    {
        return std::hash<decltype(e.handle())>{}(e.handle());
    }
};

} // namespace std