#pragma once

#include <jkl/config.hpp>
#include <jkl/uri/comp.hpp>
#include <tuple>
#include <optional>


namespace jkl{


// you can give and parse some or all uri components.
template<_uri_comp_... T>
class uri_reader : public std::tuple<lref_or_val_t<T>...>
{
    using base = std::tuple<lref_or_val_t<T>...>;

    char const* _end;
    char const* _it ;

    template<class Tag> static consteval bool has() { return has_tag<base, Tag>; };
    template<class... Tags> static consteval bool has_oneof() { return has_oneof_tag<base, Tags...>; };

    template<class Tag> auto&& get()       noexcept requires(has<Tag>()) { return get_tagged<Tag>(static_cast<base      &>(*this)); }
    template<class Tag> auto&& get() const noexcept requires(has<Tag>()) { return get_tagged<Tag>(static_cast<base const&>(*this)); }

    template<class Tag>
    void clear()
    {
        if constexpr(has<Tag>())
            return get<Tag>().clear();
    }

    void clear()
    {
    #ifdef BOOST_CLANG
        #warning "uri_read::clear() is disabled, since it crashes clang-concepts"
    #else
        clear<uri_scheme_tag          >();
        clear<uri_user_tag            >();
        clear<uri_password_tag        >();
        clear<uri_host_tag            >();
        clear<uri_port_tag            >();
        clear<uri_auto_port_tag       >();
        clear<uri_path_tag            >();
        clear<uri_query_tag           >();
        clear<uri_frag_tag            >();

        clear<uri_scheme_colon_tag    >();
        clear<uri_userinfo_tag        >();
        clear<uri_userinfo_at_tag     >();
        clear<uri_host_port_tag       >();
        clear<uri_authority_tag       >();
        clear<uri_ss_authority_tag    >();
        clear<uri_scheme_authority_tag>();
        clear<uri_qm_query_tag        >();
        clear<uri_sharp_frag_tag      >();
        clear<uri_qm_query_frag_tag   >();

        clear<uri_dir_path_tag        >();
        clear<uri_file_tag            >();
        clear<uri_file_name_tag       >();
        clear<uri_file_ext_tag        >();
        clear<uri_file_dot_ext_tag    >();
    #endif
    }

    template<class Tag, uri_codec_e SrcCodec>
    aresult<> do_read(_char_str_ auto const& s) requires(has<Tag>())
    {
        return get<Tag>().template read<SrcCodec>(s);
    }

    template<size_t Off = 0>
    char const* find_first_of(auto... c) const noexcept
    {
        BOOST_ASSERT(_it + Off <= _end);

        char const* p = _it + Off;
        while(p < _end && (... && (*p != c)))
            ++p;
        return p;
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    bool peek_seq(auto... c) const noexcept
    {
        if(_end - _it >= static_cast<ptrdiff_t>(sizeof...(c)))
        {
            auto p = _it;
            return (... && (*p++ == c));
        }
        return false;
    }

    std::optional<string_view> match_scheme_colon() noexcept
    {
        auto e = find_first_of(':', '/', '?', '#');
        if(e != _end && *e == ':')
        {
            ++e;
            return string_view{std::exchange(_it, e), e};
        }
        return std::nullopt;
    }

    std::optional<string_view> match_ss_authority() noexcept
    {
        if(peek_seq('/', '/'))
        {
            auto e = find_first_of<2>('/', '?', '#');
            return string_view{std::exchange(_it, e), e};
        }
        return std::nullopt;
    }

    std::optional<string_view> match_path() noexcept
    {
        if(_it < _end) // don't use peek_seq('/')
        {
            auto e = find_first_of('?', '#');
            return string_view{std::exchange(_it, e), e};
        }
        return std::nullopt;
    }

    std::optional<string_view> match_qm_query() noexcept
    {
        if(peek_seq('?'))
        {
            auto e = find_first_of<1>('#');
            return string_view{std::exchange(_it, e), e};
        }
        return std::nullopt;
    }

    std::optional<string_view> match_sharp_frag() noexcept
    {
        if(peek_seq('#'))
        {
            return string_view{std::exchange(_it, _end), _end};
        }
        return std::nullopt;
    }

public:
    using base::base;

    // NOTE: that's not a strict parser, it just assumes the input uses correct characters for each part,
    //       then the parser will use gen-delims(":/?#[]@") to find each part.
    //
    //                              path-abempty = *("/"segment)
    // URI = [scheme:][//authority][path-abempty][?query][#fragment]
    //                   authority = [userinfo@]host[:port]
    //                                userinfo = user[:password]
    //
    // FileURI = 'file://[host]/path'
    //
    // basically every part can still be empty IF (the specified gen-delims) PRESENTED,
    // except scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
    //
    // ref: https://tools.ietf.org/html/rfc3986#appendix-A
    template<uri_codec_e SrcCodec, bool Clear = true, bool Trim = true>
    aresult<> read(_char_str_ auto const& u)
    {
        if constexpr(Clear)
            clear();

        _it  = str_begin(u);
        _end = str_end(u);

        // trim spaces
        if constexpr(Trim)
            ascii_trim_inplace(_it, _end);

        [[maybe_unused]] auto const trimedBeg = _it;

        // scheme
        if(auto matched = match_scheme_colon())
        {
            if(matched->size() == 1)
                return uri_err::no_scheme_before_colon;

            if constexpr(has<uri_scheme_colon_tag>())
            {
                JKL_TRYV(do_read<uri_scheme_colon_tag, SrcCodec>(*matched));
            }

            if constexpr(has<uri_scheme_tag>())
            {
                matched->remove_suffix(1); // ':'
                JKL_TRYV(do_read<uri_scheme_tag, SrcCodec>(*matched));
            }
        }

        // authority
        if(auto matched = match_ss_authority())
        {
            if constexpr(has<uri_ss_authority_tag>())
            {
                JKL_TRYV(do_read<uri_ss_authority_tag, SrcCodec>(*matched));
            }

            matched->remove_prefix(2); // "//"

            // userinfo
            if(auto j = matched->find('@'); j != npos)
            {
                if constexpr(has_oneof<uri_user_tag, uri_password_tag, uri_userinfo_tag, uri_userinfo_at_tag>())
                {
                    auto userinfo = matched->substr(0, j);

                    if constexpr(has<uri_userinfo_tag>())
                    {
                        JKL_TRYV(do_read<uri_userinfo_tag, SrcCodec>(userinfo));
                    }

                    if constexpr(has<uri_userinfo_at_tag>())
                    {
                        JKL_TRYV(do_read<uri_userinfo_at_tag, SrcCodec>(matched->substr(0, j + 1)));
                    }

                    if constexpr(has<uri_user_tag>() || has<uri_password_tag>())
                    {
                        auto q = userinfo.find(':');

                        if constexpr(has<uri_user_tag>())
                        {
                            JKL_TRYV(do_read<uri_user_tag, SrcCodec>(userinfo.substr(0, q)));
                        }

                        if constexpr(has<uri_password_tag>())
                        {
                            if(q != npos)
                            {
                                JKL_TRYV(do_read<uri_password_tag, SrcCodec>(userinfo.substr(q + 1)));
                            }
                        }
                    }
                }

                matched->remove_prefix(j + 1);
            }

            // host[:port]
            if(matched->size())
            {
                if constexpr(has<uri_host_port_tag>())
                {
                    JKL_TRYV(do_read<uri_host_port_tag, SrcCodec>(*matched));
                }

                // host
                if(matched->front() == '[') // IPv6 address
                {
                    auto j = matched->rfind(']');

                    if(j == npos)
                        return uri_err::unterminated_ipv6;

                    if constexpr(has<uri_host_tag>())
                    {
                        JKL_TRYV(do_read<uri_host_tag, SrcCodec>(matched->substr(0, j + 1)));
                    }

                    matched->remove_prefix(j + 1);
                }
                else
                {
                    auto j = matched->rfind(':');

                    if constexpr(has<uri_host_tag>())
                    {
                        JKL_TRYV(do_read<uri_host_tag, SrcCodec>(matched->substr(0, j)));
                    }

                    if(j == npos)
                        *matched = {};
                    else
                        matched->remove_prefix(j);
                }

                // port
                if constexpr(has_oneof<uri_port_tag, uri_auto_port_tag>())
                {
                    if(matched->starts_with(':'))
                    {
                        matched->remove_prefix(1);
                        if constexpr(has<uri_port_tag>())
                        {
                            JKL_TRYV(do_read<uri_port_tag, SrcCodec>(*matched));
                        }
                    }

                    if constexpr(has<uri_auto_port_tag>())
                    {
                        JKL_TRYV(get<uri_auto_port_tag>().assign_port(*matched, string_view{trimedBeg, _it}));
                    }
                }
            }
        }

        if constexpr(has<uri_scheme_authority_tag>())
        {
            JKL_TRYV(do_read<uri_scheme_authority_tag, SrcCodec>(string_view{trimedBeg, _it}));
        }

        // path
        if(auto matched = match_path())
        {
            if constexpr(has<uri_path_tag>())
            {
                JKL_TRYV(do_read<uri_path_tag, SrcCodec>(*matched));
            }

            if constexpr(has_oneof<uri_dir_path_tag, uri_file_tag, uri_file_name_tag, uri_file_ext_tag, uri_file_dot_ext_tag>())
            {
                auto q = matched->rfind('/');

                if(q != npos)
                {
                    ++q;

                    if constexpr(has<uri_dir_path_tag>())
                    {

                        JKL_TRYV(do_read<uri_dir_path_tag, SrcCodec>(matched->substr(0, q)));
                    }
                }
                else
                {
                    q = 0;
                }

                if(q < matched->size())
                {
                    auto file = matched->substr(q);

                    if constexpr(has<uri_file_tag>())
                    {
                        JKL_TRYV(do_read<uri_file_tag, SrcCodec>(file));
                    }

                    if constexpr(has_oneof<uri_file_name_tag, uri_file_ext_tag, uri_file_dot_ext_tag>())
                    {
                        q = file.rfind('.');

                        if constexpr(has<uri_file_name_tag>())
                        {
                            JKL_TRYV(do_read<uri_file_name_tag, SrcCodec>(file.substr(0, q)));
                        }

                        if(q != npos)
                        {
                            if constexpr(has<uri_file_ext_tag>())
                            {
                                JKL_TRYV(do_read<uri_file_ext_tag, SrcCodec>(file.substr(q + 1)));
                            }

                            if constexpr(has<uri_file_dot_ext_tag>())
                            {
                                JKL_TRYV(do_read<uri_file_dot_ext_tag, SrcCodec>(file.substr(q)));
                            }
                        }
                    }
                }
            }
        }

        if(_it == _end)
            return no_err;

        // query and fragment
        [[maybe_unused]] auto qBeg = _it;

        if(auto matched = match_qm_query())
        {
            if constexpr(has<uri_qm_query_tag>())
            {
                JKL_TRYV(do_read<uri_qm_query_tag, SrcCodec>(*matched));
            }

            if constexpr(has<uri_query_tag>())
            {
                matched->remove_prefix(1); // '?'
                JKL_TRYV(do_read<uri_query_tag, SrcCodec>(*matched));
            }
        }

        if(auto matched = match_sharp_frag())
        {
            if constexpr(has<uri_sharp_frag_tag>())
            {
                JKL_TRYV(do_read<uri_sharp_frag_tag, SrcCodec>(*matched));
            }

            if constexpr(has<uri_frag_tag>())
            {
                matched->remove_prefix(1); // '#'
                JKL_TRYV(do_read<uri_frag_tag, SrcCodec>(*matched));
            }
        }

        if constexpr(has<uri_qm_query_frag_tag>())
        {
            JKL_TRYV(do_read<uri_qm_query_frag_tag, SrcCodec>(string_view{qBeg, _it}));
        }

        return no_err;
    }

    template<bool Clear = true, bool Trim = true, _uri_uri_ U>
    aresult<> read(U const& u)
    {
        return read<U::codec, Clear, Trim>(u.str());
    }
};


template<class... T>
uri_reader(T&&...) -> uri_reader<T&&...>;


template<uri_codec_e SrcCodec, bool Clear = true, bool Trim = true>
aresult<> read_uri(_char_str_ auto const& u, _uri_comp_ auto&&... comps)
{
    return uri_reader{JKL_FORWARD(comps)...}.template read<SrcCodec, Clear, Trim>(u);
}

template<bool Clear = true, bool Trim = true>
aresult<> read_uri(_uri_uri_ auto const& u, _uri_comp_ auto&&... comps)
{
    return uri_reader{JKL_FORWARD(comps)...}.template read<Clear, Trim>(u);
}


} // namespace jkl
