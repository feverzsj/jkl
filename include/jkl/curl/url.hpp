#pragma once

#include <jkl/config.hpp>
#include <jkl/result.hpp>
#include <jkl/util/str.hpp>
#include <jkl/util/stringify.hpp>
#include <jkl/util/handle.hpp>
#include <jkl/util/flat_map_set.hpp>
#include <jkl/curl/str.hpp>
#include <jkl/curl/error.hpp>
#include <curl/curl.h>


namespace jkl{


class curlu
{
    struct deleter{ void operator()(CURLU* h) noexcept { curl_url_cleanup(h); } };
    struct copier { CURLU* operator()(CURLU* h) { return curl_url_dup(h); } };

    copyable_handle<CURLU*, deleter, copier> _h;

protected:
    curlu(CURLU* h) : _h(h) {}

public:
    // flags:
    enum : unsigned
    {
        show_port             = CURLU_DEFAULT_PORT,       // [get] if port isn't set explicitly, show the default port.
        hide_default_port     = CURLU_NO_DEFAULT_PORT,    // [get] if port is set and is same as default, don't show it.
        allow_default_scheme  = CURLU_DEFAULT_SCHEME,     // [get/set] use default scheme if missing.
        allow_empty_authority = CURLU_NO_AUTHORITY,       // [set] allow empty authority(HostName + Port) when the scheme is unknown.
        allow_unknown_scheme  = CURLU_NON_SUPPORT_SCHEME, // [set] allow non-supported scheme.
        no_usr_pw             = CURLU_DISALLOW_USER,      // [set] no user+password allowed.
        path_as_is            = CURLU_PATH_AS_IS,         // [set] leave dot sequences on set.
        decode_on_get         = CURLU_URLDECODE,          // [get] URL decode on get.
        encode_on_set         = CURLU_URLENCODE,          // [set] URL encode on set.
        guess_scheme          = CURLU_GUESS_SCHEME        // [set] legacy curl-style guessing.
    };

    curlu() : _h(curl_url()) {}

    template<_str_ S>
    explicit curlu(S const& s, unsigned flags = encode_on_set)
        : curlu()
    {
        set(CURLUPART_URL, s, flags);
    }

    explicit operator bool() const noexcept { return _h.operator bool(); }
    auto handle() const noexcept { return _h.get(); }

    aresult<> try_set(CURLUPart what, _str_ auto const& part, unsigned flags = encode_on_set)
    {
        return curl_url_set(handle(), what, as_cstr(part).data(), flags);
    }

    curlu& set(CURLUPart what, _str_ auto const& part, unsigned flags = encode_on_set)
    {
        throw_on_error(try_set(what, part, flags));
        return *this;
    }

    aresult<curlstr> try_get(CURLUPart what, unsigned flags = 0) const
    {
        char* part = nullptr;
        if(auto e =curl_url_get(handle(), what, &part, flags))
            return e;
        return curlstr(part);
    }

    curlstr get(CURLUPart what, unsigned flags = 0) const
    {
        return try_get(what, flags).value_or_throw();
    }

    curlstr get_decoded(CURLUPart what, unsigned flags = 0) const
    {
        return get(what, flags | decode_on_get);
    }

    curlstr url     (unsigned flags = 0) const { return get(CURLUPART_URL     , flags); }
    curlstr scheme  (unsigned flags = 0) const { return get(CURLUPART_SCHEME  , flags); }
    curlstr user    (unsigned flags = 0) const { return get(CURLUPART_USER    , flags); }
    curlstr password(unsigned flags = 0) const { return get(CURLUPART_PASSWORD, flags); }
    curlstr options (unsigned flags = 0) const { return get(CURLUPART_OPTIONS , flags); }
    curlstr host    (unsigned flags = 0) const { return get(CURLUPART_HOST    , flags); }
    curlstr port    (unsigned flags = 0) const { return get(CURLUPART_PORT    , flags); }
    curlstr path    (unsigned flags = 0) const { return get(CURLUPART_PATH    , flags); }
    curlstr query   (unsigned flags = 0) const { return get(CURLUPART_QUERY   , flags); }
    curlstr fragment(unsigned flags = 0) const { return get(CURLUPART_FRAGMENT, flags); }
    curlstr zoneid  (unsigned flags = 0) const { return get(CURLUPART_ZONEID  , flags); } // ipv6 zone id

    curlstr decoded_url  (unsigned flags = 0) const { return url  (flags | decode_on_get); }
    curlstr decoded_query(unsigned flags = 0) const { return query(flags | decode_on_get); }

    // set new url;
    // if s is a relative url and a url was already set, redirect the url to s
    curlu& url     (_str_ auto const& s, unsigned flags = 0            ) & { return set(CURLUPART_URL     , s, flags); }
    curlu& scheme  (_str_ auto const& s, unsigned flags = 0            ) & { return set(CURLUPART_SCHEME  , s, flags); }
    curlu& user    (_str_ auto const& s, unsigned flags = encode_on_set) & { return set(CURLUPART_USER    , s, flags); }
    curlu& password(_str_ auto const& s, unsigned flags = encode_on_set) & { return set(CURLUPART_PASSWORD, s, flags); }
    curlu& options (_str_ auto const& s, unsigned flags = encode_on_set) & { return set(CURLUPART_OPTIONS , s, flags); }
    curlu& host    (_str_ auto const& s, unsigned flags = encode_on_set) & { return set(CURLUPART_HOST    , s, flags); }
    curlu& port    (_str_ auto const& s, unsigned flags = 0            ) & { return set(CURLUPART_PORT    , s, flags); }
    curlu& port    (   unsigned short n, unsigned flags = 0            ) & { return port(stringify(n)     ,    flags); }
    curlu& path    (_str_ auto const& s, unsigned flags = encode_on_set) & { return set(CURLUPART_PATH    , s, flags); } // all '/' are preserved, other characters are percent encoded when encode_on_set.
    curlu& fragment(_str_ auto const& s, unsigned flags = encode_on_set) & { return set(CURLUPART_FRAGMENT, s, flags); }
    curlu& zoneid  (_str_ auto const& s, unsigned flags = encode_on_set) & { return set(CURLUPART_ZONEID  , s, flags); } // ipv6 zone id

    curlu&& url     (_str_ auto const& s, unsigned flags = 0            ) && { return std::move(url     (s, flags)); }
    curlu&& scheme  (_str_ auto const& s, unsigned flags = 0            ) && { return std::move(scheme  (s, flags)); }
    curlu&& user    (_str_ auto const& s, unsigned flags = encode_on_set) && { return std::move(user    (s, flags)); }
    curlu&& password(_str_ auto const& s, unsigned flags = encode_on_set) && { return std::move(password(s, flags)); }
    curlu&& options (_str_ auto const& s, unsigned flags = encode_on_set) && { return std::move(options (s, flags)); }
    curlu&& host    (_str_ auto const& s, unsigned flags = encode_on_set) && { return std::move(host    (s, flags)); }
    curlu&& port    (_str_ auto const& s, unsigned flags = 0            ) && { return std::move(port    (s, flags)); }
    curlu&& port    (   unsigned short n, unsigned flags = 0            ) && { return std::move(port    (n, flags)); }
    curlu&& path    (_str_ auto const& s, unsigned flags = encode_on_set) && { return std::move(path    (s, flags)); }
    curlu&& fragment(_str_ auto const& s, unsigned flags = encode_on_set) && { return std::move(fragment(s, flags)); }
    curlu&& zoneid  (_str_ auto const& s, unsigned flags = encode_on_set) && { return std::move(zoneid  (s, flags)); }

    // if flags & encode_on_set, 's' must has the form of "name=value", since only the first '=' is preserved, other characters are percent encoded
    curlu& set_query_str(_str_ auto const& s, unsigned flags = encode_on_set) & { return set(CURLUPART_QUERY, s, flags); }
    curlu& append_query_str(_str_ auto const& s, unsigned flags = encode_on_set) & { return set_query_str(s, flags | CURLU_APPENDQUERY); }
    curlu& query_str(_str_ auto const&... s) & { return (... , append_query_str(s)); }
    curlu& query(_str_ auto const& k, _stringifible_ auto const& v) & { return append_query_str(cat_str(k, '=', stringify(v))); }
    curlu& query(auto const&... kvs) & { return (... , query(kvs)); }

    curlu&& set_query_str(_str_ auto const& s, unsigned flags = encode_on_set) && { return std::move(set_query_str(s, flags)); }
    curlu&& append_query_str(_str_ auto const& s, unsigned flags = encode_on_set) && { return std::move(append_query_str(s, flags)); }
    curlu&& query_str(_str_ auto const&... s) && { return std::move(query_str(s...)); }
    curlu&& query(auto const&... kvs) && { return std::move(query(kvs...)); }
};



curlstr encode_url(_str_ auto const& u){ return curlu{u}.url(); }
curlstr decode_url(_str_ auto const& u){ return curlu{u, 0}.decoded_url(); }


// URL escape/unescape the whole string, so don't pass in whole url

curlstr curl_escape(_str_ auto const& s)
{
    return curlstr(curl_easy_escape(nullptr, str_data(s), static_cast<int>(str_size(s))));
}

curlstr curl_unescape(_str_ auto const& s)
{
    return curlstr(curl_easy_unescape(nullptr, str_data(s), static_cast<int>(str_size(s))));
}



template<size_t SmallSize>
class url_query_t
{
    string _q;
    small_flat_multimap<std::string_view, std::string_view, SmallSize> _kvs;

public:
    url_query_t() = default;
    explicit url_query_t(_str_ auto const& q) { set(q); }

    url_query_t(url_query_t const&) = delete;
    url_query_t& operator=(url_query_t const&) = delete;

    void rebuild_query()
    {
        _q.clear();
        for(auto& kv : _kvs)
            append_str(_q, k.first, '=', k.second);
    }

    auto const& query() { if(_q.empty()) rebuild_query(); return _q; }

    void set(_str_ auto const& q)
    {
        _q.assign(str_data(q), str_size(q));
        _kvs.clear();

        char const* pos = _q.data();
        char const* const end = _q.data() + _q.size();

        while(pos != end)
        {
            char const* begin = pos;
            char const* delimiter = 0;

            while(pos != end)
            {
                // scan for the component parts of this pair
                if(! delimiter && *pos == '=')
                    delimiter = pos;
                if(*pos == '&')
                    break;
                ++pos;
            }

            if(! delimiter)
                delimiter = pos;

            // pos is the end of this pair (the end of the string or the pair delimiter)
            // delimiter points to the value delimiter or to the end of this pair

             _kvs.emplace(std::string_view{begin, delimiter - begin},
                (pos > delimiter + 1) ? std::string_view{delimiter + 1, pos - delimiter - 1} : std::string_view{});

            if(pos != end)
                ++pos;
        }
    }

};



} // namespace jkl
