#pragma once

#include <jkl/config.hpp>
#include <jkl/util/str.hpp>
#include <new>
#include <memory>
#include <curl/curl.h>


namespace jkl{


// NOTE: curl_slist won't be copied in so must outlive the easy handle
// NOTE: handle is null until you actually append something.
//       so be careful with handle_opt like curlopt::httpheader.
// NOTE: move only
class curlist
{
    struct deleter{ void operator()(curl_slist* h) noexcept { curl_slist_free_all(h); } };
    std::unique_ptr<curl_slist, deleter> _h;

public:
    template<class... T>
    explicit curlist(T&&... t)
    {
        append(std::forward<T>(t)...);
    }

    explicit operator bool() const noexcept { return _h.operator bool(); }
    auto handle() const noexcept { return _h.get(); }

    void reset() { _h.reset(); } // NOTE: handle is now null, next append() will create a valid handle.

    template</*_str_*/class S> 
    void append1(S const& s)
    {
        auto t = curl_slist_append(handle(), as_cstr(s).data());
        if(! t)
            throw std::runtime_error("curlist: failed to append");
        if(! _h)
            _h.reset(t);
    }

    void append1(curlist const& l)
    {
        for(auto const& s : l)
            append1(s);
    }

    void append1(curlist&& l) noexcept
    {
        if(_h)
        {
            auto h = handle();
            while(h->next)
                h = h->next;
            h->next = l._h.release();
        }
        else
        {
            _h = std::move(l._h);
        }
    }

    template<class... T>
    void append(T&&... ts)
    {
        (... , append1(std::forward<T>(ts)));
    }

    auto const& front() const noexcept
    {
        BOOST_ASSERT(_h);

        return _h->data;
    }

    auto const& back() const noexcept
    {
        BOOST_ASSERT(_h);

        auto h = handle();
        while(h->next)
            h = h->next;
        return h->data;
    }

    size_t size() const noexcept
    {
        size_t n = 0;

        for(auto h = handle(); h; h = h->next)
            ++n;

        return n;
    }


    class const_iterator
    {
        friend class curlist;

        curl_slist* _h = nullptr;

        const_iterator() = default;
        const_iterator(curl_slist* h) : _h(h) {}

    public:
        char* const& operator* () const noexcept { BOOST_ASSERT(_h); return _h->data; }
        const_iterator& operator++() noexcept { if(_h) _h = _h->next; return *this; }
        bool operator==(const_iterator const& r) const noexcept { return _h == r._h; }
        bool operator!=(const_iterator const& r) const noexcept { return _h != r._h; }
    };

    const_iterator begin() const noexcept { return {handle()}; }
    const_iterator end  () const noexcept { return {}; }
};


} // namespace jkl
