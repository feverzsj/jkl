#pragma once

#include <jkl/config.hpp>
#include <jkl/result.hpp>
#include <jkl/curl/error.hpp>
#include <curl/curl.h>


namespace jkl{


class curl_share
{
    struct deleter{ void operator()(CURLSH* h) noexcept { curl_share_cleanup(h); } };
    std::unique_ptr<CURLSH, deleter> _h;

public:
    curl_share() : _h(curl_share_init()) {}

    auto handle() const noexcept { return _h.get(); }
    explicit operator bool() const noexcept { return _h.operator bool(); }

    template<class T>
    void setopt(CURLSHoption k, T v)
    {
        throw_on_error(curl_share_setopt(handle(), k, v));
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
};


} // namespace jkl