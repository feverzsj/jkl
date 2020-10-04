#pragma once

#include <jkl/config.hpp>
#include <jkl/result.hpp>
#include <jkl/curl/str.hpp>
#include <jkl/curl/opt.hpp>
#include <jkl/curl/error.hpp>
#include <jkl/curl/easy.hpp>
#include <curl/curl.h>
#include <optional>
#include <memory>
#include <deque>


namespace jkl{


class curl_multi
{
    struct deleter{ void operator()(CURLM* h) noexcept { curl_multi_cleanup(h); } };
    std::unique_ptr<CURLM, deleter> _h;

protected:
    curl_multi(CURLM* h) : _h(h) {}

public:
    curl_multi() : _h(curl_multi_init()) {}

    explicit operator bool() const noexcept { return _h.operator bool(); }
    auto handle() const noexcept { return _h.get(); }

    template<class T>
    void setopt(CURLMoption k, T v)
    {
        throw_on_error(curl_multi_setopt(handle(), k, v));
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

    // NOTE: all added curl_easy must exist until being removed or until after destruction of curl_multi
    aresult<> add   (curl_easy& e) noexcept { return curl_multi_add_handle   (handle(), e.handle()); }
    aresult<> remove(curl_easy& e) noexcept { return curl_multi_remove_handle(handle(), e.handle()); }

    template<class Buf>
    aresult<int> wait(Buf& extra_fds, std::chrono::milliseconds const& timeout) noexcept
    {
        int n_fds = 0;
        if(auto e = curl_multi_wait(handle(), buf_data(extra_fds), static_cast<unsigned>(buf_size(extra_fds)),
                                    static_cast<int>(timeout.count()), &n_fds))
            return e;
        return n_fds;
    }

    aresult<int> perform() noexcept
    {
        int runningHandles = 0;
        if(auto e = curl_multi_perform(handle(), &runningHandles))
            return e;
        return runningHandles;
    }

    template<class Buf>
    aresult<int> poll(Buf& extra_fds, std::chrono::milliseconds const& timeout) noexcept
    {
        int n_fds = 0;
        if(auto e = curl_multi_poll(handle(), buf_data(extra_fds), static_cast<unsigned>(buf_size(extra_fds)),
                                    static_cast<int>(timeout.count()), &n_fds))
            return e;
        return n_fds;
    }

    aresult<> wakeup() noexcept
    {
        return curl_multi_wakeup(handle());
    }

    aresult<int> fdset(fd_set* read_fd_set, fd_set* write_fd_set, fd_set* exc_fd_set) noexcept
    {
        int max_fd = 0;
        if(auto e = curl_multi_fdset(handle(), read_fd_set, write_fd_set, exc_fd_set, &max_fd))
            return e;
        return max_fd;
    }

    aresult<std::chrono::milliseconds> timeout() noexcept
    {
        long ms = 0;
        if(auto e = curl_multi_timeout(handle(), &ms))
            return e;
        return std::chrono::milliseconds(ms);
    }

    aresult<int> socket_action(curl_socket_t s, int ev_bitmask) noexcept
    {
        int runningHandles = 0;
        if(auto e = curl_multi_socket_action(handle(), s, ev_bitmask, &runningHandles))
            return e;
        return runningHandles;
    }

    aresult<> assign(curl_socket_t sockfd, void* sockp) noexcept
    {
        return curl_multi_assign(handle(), sockfd, sockp);
    }

    CURLMsg* next_msg(int& left) noexcept
    {
        return curl_multi_info_read(handle(), &left);
    }

    CURLMsg* next_msg() noexcept
    {
        int left = 0;
        return next_msg(left);
    }

    struct done_msg_t
    {
        curl_easy easy;
        CURLcode  err;

        explicit done_msg_t(CURLMsg& m) : easy(m.easy_handle), err(m.data.result) {}
    };

    std::optional<done_msg_t> next_done() noexcept
    {
        while(CURLMsg* m = next_msg())
        {
            if(m->msg == CURLMSG_DONE)
                return std::optional<done_msg_t>(std::in_place, *m);
        }

        return std::nullopt;
    }

};


} // namespace jkl
