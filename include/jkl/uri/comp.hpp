#pragma once

#include <jkl/config.hpp>
#include <jkl/result.hpp>
#include <jkl/util/str.hpp>
#include <jkl/util/tagged.hpp>
#include <jkl/util/numerify.hpp>
#include <jkl/util/stringify.hpp>
#include <jkl/uri/fwd.hpp>
#include <jkl/uri/url_codec.hpp>
#include <jkl/uri/well_known_port.hpp>
#include <boost/asio/ip/address.hpp>


namespace jkl{


template<class T> concept _uri_comp_ = _tagged_from_<T, uri_tag>;
template<class T> concept _uri_path_ = _tagged_as_<T, uri_path_tag>;
template<class T> concept _uri_uri_  = _tagged_as_<T, uri_uri_tag >;


template<class Tag, uri_codec_e DestCodec, uri_codec_e SrcCodec>
constexpr size_t uri_max_codec_cvt_size(_stringifible_ auto const& t)
{
    if constexpr(DestCodec == url_decoded && SrcCodec == url_encoded)
    {
        return max_stringify_size(t);
    }
    else if constexpr(DestCodec == url_encoded && SrcCodec == url_decoded)
    {
        return url_max_encode_size<Tag>(t);
    }
    else if constexpr(DestCodec == SrcCodec || DestCodec == as_is_codec || SrcCodec == as_is_codec)
    {
        return max_stringify_size(t);
    }
    else
    {
        JKL_DEPENDENT_FAIL(decltype(t), "unsupported codec cvt");
    }
}

template<class Tag, uri_codec_e DestCodec, uri_codec_e SrcCodec>
aresult<char*> uri_codec_cvt_append(char* d, _char_str_ auto const& s)
{
    if constexpr(DestCodec == url_decoded && SrcCodec == url_encoded)
    {
        return url_decode_append<Tag>(d, s);
    }
    else if constexpr(DestCodec == url_encoded && SrcCodec == url_decoded)
    {
        return url_encode_append<Tag>(d, s);
    }
    else if constexpr(DestCodec == SrcCodec || DestCodec == as_is_codec || SrcCodec == as_is_codec)
    {
        return append_str(d, s);
    }
    else
    {
        JKL_DEPENDENT_FAIL(decltype(s), "unsupported codec cvt");
    }
}

template<class Tag, uri_codec_e DestCodec, uri_codec_e SrcCodec>
aresult<> uri_codec_cvt_assign(auto& d, _char_str_ auto const& s)
{
    if constexpr(DestCodec == url_decoded && SrcCodec == url_encoded)
    {
        return url_decode_assign<Tag>(d, s);
    }
    else if constexpr(DestCodec == url_encoded && SrcCodec == url_decoded)
    {
        url_encode_assign<Tag>(d, s);
        return no_err;
    }
    else if constexpr(DestCodec == SrcCodec || DestCodec == as_is_codec || SrcCodec == as_is_codec)
    {
        assign_str(d, s);
        return no_err;
    }
    else
    {
        JKL_DEPENDENT_FAIL(decltype(s), "unsupported codec cvt");
    }
}



template<class Tag, uri_codec_e Codec, _str_class_ S>
struct uri_str_comp
{
    using tag = Tag;
    static constexpr uri_codec_e codec = Codec;

    lref_or_val_t<S> _s;

    auto& str() const noexcept { return _s; }
    auto size() const noexcept { return _s.size(); }
    bool empty() const noexcept { return _s.empty(); }

    auto find(auto const& s) const noexcept
    {
        if constexpr(requires{ _s.find(s); })
            return _s.find(s);
        else
            return string_view{_s}.find(s);
    }

    auto rfind(auto const& s) const noexcept
    {
        if constexpr(requires{ _s.rfind(s); })
            return _s.rfind(s);
        else
            return string_view{_s}.rfind(s);
    }

    bool starts_with(auto const& s) const noexcept
    {
        if constexpr(requires{ _s.starts_with(s); })
            return _s.starts_with(s);
        else
            return string_view{_s}.starts_with(s);
    }

    bool ends_with(auto const& s) const noexcept
    {
        if constexpr(requires{ _s.ends_with(s); })
            return _s.ends_with(s);
        else
            return string_view{_s}.ends_with(s);
    }

    auto subview(size_t const pos = 0, size_t const count = string_view::npos) const
    {
        if constexpr(requires{ _s.subview(pos, count); })
            return _s.subview(pos, count);
        else
            return string_view{_s}.substr(pos, count);
    }

    template<class SubTag = Tag>
    auto subcomp(size_t const pos = 0, size_t const count = string_view::npos) const
    {
        return uri_str_comp<SubTag, Codec, string_view>{subview(pos, count)};
    }

    /// uri comp concept:

    constexpr bool non_empty() const noexcept
    {
        return _s.size() != 0;
    }

    template<uri_codec_e DestCodec>
    size_t max_write_size() const noexcept
    {
        return uri_max_codec_cvt_size<Tag, DestCodec, Codec>(_s);
    }

    template<uri_codec_e DestCodec>
    aresult<char*> write(char* d) const noexcept
    {
        return uri_codec_cvt_append<Tag, DestCodec, Codec>(d, _s);
    }

    void clear() requires(! std::is_const_v<std::remove_reference_t<S>> && (requires{ _s.clear(); } || requires{ _s = {}; }))
    {
        if constexpr(requires{ _s.clear(); })
            _s.clear();
        else if constexpr(requires{ _s = {}; })
            _s = {};
        else
            JKL_DEPENDENT_FAIL(S, "unsupported type");
    }

    template<uri_codec_e SrcCodec>
    aresult<> read(_char_str_ auto&& s) requires(! std::is_const_v<std::remove_reference_t<S>>)
    {
        return uri_codec_cvt_assign<Tag, Codec, SrcCodec>(_s, JKL_FORWARD(s));
    }
};


template<class Tag, uri_codec_e Codec> // codec of 's'
constexpr auto uri_str(_char_str_ auto&& s) noexcept
{
    return uri_str_comp<
        Tag, Codec, decltype(as_str_class(JKL_FORWARD(s)))
    >{as_str_class(JKL_FORWARD(s))};
}


template<uri_codec_e Codec = url_decoded>
constexpr auto uri_scheme(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_scheme_tag, Codec>(JKL_FORWARD(s));
}

template<uri_codec_e Codec = url_decoded>
constexpr auto uri_scheme_colon(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_scheme_colon_tag, Codec>(JKL_FORWARD(s));
}

template<uri_codec_e Codec = url_decoded>
constexpr auto uri_user(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_user_tag, Codec>(JKL_FORWARD(s));
}

template<uri_codec_e Codec = url_decoded>
constexpr auto uri_password(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_password_tag, Codec>(JKL_FORWARD(s));
}

template<uri_codec_e Codec = url_decoded>
constexpr auto uri_userinfo(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_userinfo_tag, Codec>(JKL_FORWARD(s));
}

template<uri_codec_e Codec = url_decoded>
constexpr auto uri_userinfo_at(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_userinfo_at_tag, Codec>(JKL_FORWARD(s));
}

template<uri_codec_e Codec = url_decoded>
constexpr auto uri_host(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_host_tag, Codec>(JKL_FORWARD(s));
}



template<class T>
    requires(is_one_of_v<std::remove_cvref_t<T>, asio::ip::address, asio::ip::address_v4, asio::ip::address_v6>)
struct uri_ipaddr_comp
{
    using tag = uri_host_tag;

    lref_or_val_t<T> addr;
    bool unspecified;

    explicit uri_ipaddr_comp(auto&& a) : addr{JKL_FORWARD(a)}, unspecified{a.is_unspecified()} {}

    constexpr bool non_empty() const noexcept
    {
        return ! unspecified;
    }

    template<uri_codec_e DestCodec>
    size_t max_write_size() const noexcept
    {
        if constexpr(std::is_same_v<std::remove_cvref_t<T>, asio::ip::address>)
            return addr.is_ipv4() ? 15 : 45;
        if constexpr(std::is_same_v<std::remove_cvref_t<T>, asio::ip::address_v4>)
            return 15;
        if constexpr(std::is_same_v<std::remove_cvref_t<T>, asio::ip::address_v6>)
            return 45;
        else
            JKL_DEPENDENT_FAIL(T, "unsupported type");
    }

    template<uri_codec_e DestCodec>
    aresult<char*> write(char* d) const noexcept
    {
        if constexpr(std::is_same_v<T, asio::ip::address>)
        {
            if(addr.is_ipv6())
                return append_str(d, '[', addr.to_string(), ']');
            return append_str(d, addr.to_string());
        }
        if constexpr(std::is_same_v<T, asio::ip::address_v4>)
        {
            return append_str(d, addr.to_string());
        }
        if constexpr(std::is_same_v<T, asio::ip::address_v6>)
        {
            return append_str(d, '[', addr.to_string(), ']');
        }
        else
        {
            JKL_DEPENDENT_FAIL(T, "unsupported type");
        }
    }

    void clear() requires(! std::is_const_v<std::remove_reference_t<T>> && requires{ addr = {}; })
    {
        addr = {};
        unspecified = true;
    }

    template<uri_codec_e SrcCodec>
    aresult<> read(_char_str_ auto const& s) requires(! std::is_const_v<std::remove_reference_t<T>>)
    {
        aerror_code ec;

        if constexpr(std::is_same_v<std::remove_cvref_t<T>, asio::ip::address>)
            addr = asio::ip::make_address(s, ec);
        if constexpr(std::is_same_v<std::remove_cvref_t<T>, asio::ip::address_v4>)
            addr = asio::ip::make_address_v4(s, ec);
        if constexpr(std::is_same_v<std::remove_cvref_t<T>, asio::ip::address_v6>)
            addr = asio::ip::make_address_v4(s, ec);
        else
            JKL_DEPENDENT_FAIL(T, "unsupported type");

        unspecified = ec;

        return ec;
    }
};


template<class T>
constexpr auto uri_host(T&& addr) noexcept
{
    return uri_ipaddr_comp<T>{std::forward<T>(addr)};
}


template<uri_codec_e Codec = as_is_codec>
constexpr auto uri_port(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_port_tag, Codec>(JKL_FORWARD(s));
}


template<_arithmetic_ T>
struct uri_port_comp
{
    using tag = uri_port_tag;

    lref_or_val_t<T> n;

    constexpr bool non_empty() const noexcept
    {
        return n != 0;
    }

    template<uri_codec_e DestCodec>
    size_t max_write_size() const noexcept
    {
        return max_stringify_size(n);
    }

    template<uri_codec_e DestCodec>
    aresult<char*> write(char* d)
    {
        return append_str(d, stringify(n));
    }

    void clear() requires(! std::is_const_v<std::remove_reference_t<T>>)
    {
        n = 0;
    }

    template<uri_codec_e SrcCodec>
    aresult<> read(_char_str_ auto const& s) requires(! std::is_const_v<std::remove_reference_t<T>>)
    {
        if(str_size(s) == 0)
            n = 0;
        else if(! numerify(s, n))
            return uri_err::invalid_port;
        return no_err;
    }
};


template<_arithmetic_ T>
struct uri_auto_port_comp
{
    using tag = uri_auto_port_tag;

    lref_or_val_t<T> n;

    template<uri_codec_e DestCodec>
    size_t max_write_size() const noexcept
    {
        return 1 + max_stringify_size(n);
    }

    void clear() requires(! std::is_const_v<std::remove_reference_t<T>>)
    {
        n = 0;
    }

    char* append_colon_port_to(char* d, _char_str_ auto const& uri) // uri until 'd'
    {
        if(n && n != prefix_to_well_known_port(uri))
            return append_str(d, ':', stringify(n));
        return d;
    }
                                                               //vvv: uri until 's'
    aresult<> assign_port(_char_str_ auto const& s, _char_str_ auto const& uri) requires(! std::is_const_v<std::remove_reference_t<T>>)
    {
        if(str_size(s) == 0)
        {
            n = prefix_to_well_known_port(uri);
        }
        else
        {
            if(! numerify(s, n))
                return uri_err::invalid_port;
        }
        return no_err;
    }
};

constexpr auto uri_port(_arithmetic_ auto&& n) noexcept
{
    return uri_port_comp<decltype(n)>{JKL_FORWARD(n)};
}

constexpr auto uri_auto_port(_arithmetic_ auto&& n) noexcept
{
    return uri_auto_port_comp<decltype(n)>{JKL_FORWARD(n)};
}


template<uri_codec_e Codec = url_decoded>
constexpr auto uri_host_port(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_host_port_tag, Codec>(JKL_FORWARD(s));
}

template<uri_codec_e Codec = url_decoded>
constexpr auto uri_authority(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_authority_tag, Codec>(JKL_FORWARD(s));
}

template<uri_codec_e Codec = url_decoded>
constexpr auto uri_ss_authority(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_ss_authority_tag, Codec>(JKL_FORWARD(s));
}

template<uri_codec_e Codec = url_decoded>
constexpr auto uri_scheme_authority(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_scheme_authority_tag, Codec>(JKL_FORWARD(s));
}


template<uri_codec_e Codec = url_decoded>
constexpr auto uri_path(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_path_tag, Codec>(JKL_FORWARD(s));
}

template<uri_codec_e Codec = url_decoded>
constexpr auto uri_dir_path(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_dir_path_tag, Codec>(JKL_FORWARD(s));
}

template<uri_codec_e Codec = url_decoded>
constexpr auto uri_file(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_file_tag, Codec>(JKL_FORWARD(s));
}

template<uri_codec_e Codec = url_decoded>
constexpr auto uri_file_name(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_file_name_tag, Codec>(JKL_FORWARD(s));
}

template<uri_codec_e Codec = url_decoded>
constexpr auto uri_file_ext(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_file_ext_tag, Codec>(JKL_FORWARD(s));
}

template<uri_codec_e Codec = url_decoded>
constexpr auto uri_file_dot_ext(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_file_dot_ext_tag, Codec>(JKL_FORWARD(s));
}


template<uri_codec_e Codec = url_decoded>
constexpr auto uri_query(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_query_tag, Codec>(JKL_FORWARD(s));
}


template<uri_codec_e DestCodec, uri_codec_e Codec>
aresult<> _append_query_pair(char*& d, _char_str_ auto const& k, _stringifible_ auto const& v) noexcept
{
    JKL_TRY(d, uri_codec_cvt_append<uri_query_tag, DestCodec, Codec>(d, k));
    *d++ = '=';
    JKL_TRY(d, uri_codec_cvt_append<uri_query_tag, DestCodec, Codec>(d, stringify(v)));
    *d++ = '&';
    return no_err;
}


template<uri_codec_e Codec, class... T>
    requires(sizeof...(T) > 1 && sizeof...(T) % 2 == 0)
struct uri_query_kvs_comp : std::tuple<lref_or_val_t<T>...>
{
    using tag = uri_query_tag;

    using base = std::tuple<lref_or_val_t<T>...>;
    using base::base;

    constexpr bool non_empty() const noexcept
    {
        return true;
    }

    template<uri_codec_e DestCodec>
    size_t max_write_size() const noexcept
    {
        return std::apply(
            [](auto const&... kvs)
            {
                if constexpr(DestCodec == url_encoded && Codec == url_decoded)
                    return max_stringify_size(kvs...) * 3 + sizeof...(kvs);
                else
                    return max_stringify_size(kvs...) + sizeof...(kvs);
            },
            static_cast<base const&>(*this)
        );
    }

    template<uri_codec_e DestCodec>
    aresult<char*> write(char* d) const noexcept
    {
        auto&& r = foreach_pair_of_tuple(
            [&d](_char_str_ auto const& k, _stringifible_ auto const& v)
            {
                return _append_query_pair<DestCodec, Codec>(d, k, v);
            },
            static_cast<base const&>(*this)
        );

        JKL_TRY(r);
        return --d; // remove last '&'
    }

    static aresult<> foreach_pair(auto&& f, auto&& a, auto&& b, auto const&... pas)
    {
        auto&& r = JKL_FORWARD(f)(JKL_FORWARD(a), JKL_FORWARD(b));
        if(r)
            return foreach_pair(JKL_FORWARD(f), std::forward<decltype(pas)>(pas)...);
        return r;
    }

    static aresult<> foreach_pair_of_tuple(auto&& f, auto&& t)
    {
        return std::apply(
            [&f](auto&&... pas)
            {
                return foreach_pair(JKL_FORWARD(f), std::forward<decltype(pas)>(pas)...);
            },
            JKL_FORWARD(t));
    }
};


template<uri_codec_e Codec>
constexpr auto uri_query(auto&&... kvs) noexcept requires(sizeof...(kvs) > 1 && sizeof...(kvs) % 2 == 0)
{
    return uri_query_kvs_comp<Codec, decltype(kvs)...>{JKL_FORWARD(kvs)...};
}


template<uri_codec_e Codec, class T>
struct uri_query_range_or_cont_comp
{
    using tag = uri_query_tag;

    static constexpr uri_codec_e codec = Codec;

    lref_or_val_t<T> rc;

    constexpr bool non_empty() const noexcept
    {
        return std::size(rc) != 0;
    }

    template<uri_codec_e DestCodec>
    size_t max_write_size() const noexcept
    {
        size_t n = 0;

        for(auto& kv : rc)
            n += max_stringify_size(kv.first) + max_stringify_size(kv.second);

        if constexpr(DestCodec == url_encoded && Codec == url_decoded)
            return n * 3 + std::size(rc) * 2;
        else
            return n + std::size(rc) * 2;
    }

    template<uri_codec_e DestCodec>
    aresult<char*> write(char* d) const noexcept
    {
        for(auto&[k, v] : rc)
        {
            JKL_TRYV(_append_query_pair<DestCodec, Codec>(d, k, v));
        }

        return --d; // remove last '&'
    }

    void clear() requires(! std::is_const_v<std::remove_reference_t<T>> && requires{ rc.clear(); })
    {
        if constexpr(requires{ rc.clear(); })
            rc.clear();
        else
            JKL_DEPENDENT_FAIL(T, "unsupported codec cvt");
    }

    template<uri_codec_e SrcCodec>
    aresult<> read(string_view s) requires(! std::is_const_v<std::remove_reference_t<T>>)
    {
        while(s.size())
        {
            auto i = s.find_first_of("=&");

            string_view k = s.substr(0, i);
            string_view v;

            if(i != npos && s[i] == '=')
            {
                ++i;
                auto j = s.find(i, '&');
                v = s.substr(i, j);
                i = j;
            }

            JKL_TRY(emplace<SrcCodec>(k, v));
            s.remove_prefix(i + 1);
        }

        return no_err;
    }

    template<uri_codec_e SrcCodec>
    aresult<> emplace(_char_str_ auto const& k, _char_str_ auto const& v) requires(! std::is_const_v<std::remove_reference_t<T>>)
    {
        if constexpr(Codec == SrcCodec || Codec == as_is_codec || SrcCodec == as_is_codec)
        {
            direct_emplace(k, v);
            return no_err;
        }
        else
        {
            using cont_t = std::remove_cvref_t<decltype(rc)>;
            using key_t  = typename cont_t::value_type::first_type;
            using val_t  = typename cont_t::value_type::second_type;

            key_t dk;
            JKL_TRYV(uri_codec_cvt_assign<uri_query_tag, Codec, SrcCodec>(dk, k));
            val_t dv;
            JKL_TRYV(uri_codec_cvt_assign<uri_query_tag, Codec, SrcCodec>(dv, v));

            direct_emplace(std::move(dk), std::move(dv));
            return no_err;
        }
    }

    void direct_emplace(auto&& k, auto&& v) requires(! std::is_const_v<std::remove_reference_t<T>>)
    {
        if constexpr(requires{ rc.emplace(JKL_FORWARD(k), JKL_FORWARD(v)); })
            rc.emplace(JKL_FORWARD(k), JKL_FORWARD(v));
        else if constexpr(requires{ rc.emplace_back(JKL_FORWARD(k), JKL_FORWARD(v)); })
            rc.emplace_back(JKL_FORWARD(k), JKL_FORWARD(v));
        else
            JKL_DEPENDENT_FAIL(decltype(k), "unsupported type");
    }
};

template<uri_codec_e Codec = url_decoded>
constexpr auto uri_query_range_or_cont(auto&& rc) noexcept
{
    return uri_query_range_or_cont_comp<Codec, decltype(rc)>{JKL_FORWARD(rc)};
}


template<uri_codec_e Codec = url_decoded>
constexpr auto uri_qm_query(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_qm_query_tag, Codec>(JKL_FORWARD(s));
}


template<uri_codec_e Codec = url_decoded>
constexpr auto uri_frag(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_frag_tag, Codec>(JKL_FORWARD(s));
}


template<uri_codec_e Codec = url_decoded>
constexpr auto uri_sharp_frag(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_sharp_frag_tag, Codec>(JKL_FORWARD(s));
}


template<uri_codec_e Codec = url_decoded>
constexpr auto uri_qm_query_frag(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_qm_query_frag_tag, Codec>(JKL_FORWARD(s));
}


template<uri_codec_e Codec = url_decoded>
constexpr auto uri_uri(_char_str_ auto&& s) noexcept
{
    return uri_str<uri_uri_tag, Codec>(JKL_FORWARD(s));
}


} // namespace jkl
