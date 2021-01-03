#pragma once

#include <jkl/config.hpp>
#include <jkl/uri/comp.hpp>
#include <jkl/uri/reader.hpp>
#include <tuple>


namespace jkl{


template<_uri_comp_... T>
class uri_writer : public std::tuple<lref_or_val_t<T>...>
{
    using base = std::tuple<lref_or_val_t<T>...>;

    template<class Tag> static consteval bool has() { return has_tag<base, Tag>; };
    template<class... Tags> static consteval bool has_oneof() { return has_oneof_tag<base, Tags...>; };

    template<class Tag> auto&& get()       noexcept requires(has<Tag>()) { return get_tagged<Tag>(static_cast<base      &>(*this)); }
    template<class Tag> auto&& get() const noexcept requires(has<Tag>()) { return get_tagged<Tag>(static_cast<base const&>(*this)); }

    template<class Tag, uri_codec_e DestCodec>
    size_t mxsize() const noexcept requires(has<Tag>())
    {
        return get<Tag>().template max_write_size<DestCodec>();
    }

    template<class Tag, uri_codec_e DestCodec> // codec of output 'd'
    aresult<char*> do_write(char* d) const requires(has<Tag>())
    {
        return get<Tag>().template write<DestCodec>(d);
    }

    template<class Tag>
    constexpr bool non_empty() const noexcept requires(has<Tag>())
    {
        return get<Tag>().non_empty();
    }

public:
    using base::base;

    // TODO: sync with wirte()
    template<uri_codec_e DestCodec>
    size_t max_write_size() const
    {
        size_t n = 0;

        if constexpr(has<uri_scheme_authority_tag>())
        {
            n += mxsize<uri_scheme_authority_tag, DestCodec>();
        }
        else
        {
            if constexpr(has<uri_scheme_colon_tag>())
            {
                n += mxsize<uri_scheme_colon_tag, DestCodec>();
            }
            else if constexpr(has<uri_scheme_tag>())
            {
                if(non_empty<uri_scheme_tag>())
                    n += mxsize<uri_scheme_tag, DestCodec>() + 1;
            }

            if constexpr(has<uri_ss_authority_tag>())
            {
                n += mxsize<uri_ss_authority_tag, DestCodec>();
            }
            else if constexpr(has<uri_authority_tag>())
            {
                n += 2 + mxsize<uri_authority_tag, DestCodec>();
            }
            else
            {
                n += 2;

                if constexpr(has<uri_userinfo_at_tag>())
                {
                    n += mxsize<uri_userinfo_at_tag, DestCodec>();
                }
                else if constexpr(has<uri_userinfo_tag>())
                {
                    if(non_empty<uri_userinfo_tag>())
                        n += mxsize<uri_userinfo_tag, DestCodec>() + 1;
                }
                else if constexpr(has<uri_user_tag>())
                {
                    if(non_empty<uri_user_tag>())
                    {
                        n += mxsize<uri_user_tag, DestCodec>();

                        if constexpr(has<uri_password_tag>())
                        {
                            if(non_empty<uri_password_tag>())
                                n += 1 + mxsize<uri_password_tag, DestCodec>();
                        }
                        n += 1;
                    }
                }

                if constexpr(has<uri_host_port_tag>())
                {
                    n += mxsize<uri_host_port_tag, DestCodec>();
                }
                else
                {
                    if constexpr(has<uri_host_tag>())
                    {
                        n += mxsize<uri_host_tag, DestCodec>();
                    }

                    if constexpr(has<uri_port_tag>())
                    {
                        if(non_empty<uri_port_tag>())
                            n += 1 + mxsize<uri_port_tag, DestCodec>();
                    }
                    else if constexpr(has<uri_auto_port_tag>())
                    {
                        n += mxsize<uri_auto_port_tag, DestCodec>();
                    }
                }
            }
        }

        if constexpr(has<uri_path_tag>())
        {
            n += mxsize<uri_path_tag, DestCodec>();
        }
        else
        {
            if constexpr(has<uri_dir_path_tag>())
            {
                n += mxsize<uri_dir_path_tag, DestCodec>();
            }

            if constexpr(has<uri_file_tag>())
            {
                n += mxsize<uri_file_tag, DestCodec>();
            }
            else
            {
                if constexpr(has<uri_file_name_tag>())
                {
                    n += mxsize<uri_file_name_tag, DestCodec>();
                }

                if constexpr(has<uri_file_dot_ext_tag>())
                {
                    n += mxsize<uri_file_dot_ext_tag, DestCodec>();
                }
                else if constexpr(has<uri_file_ext_tag>())
                {
                    n += 1 + mxsize<uri_file_ext_tag, DestCodec>();
                }
            }
        }

        if constexpr(has<uri_qm_query_frag_tag>())
        {
            n += mxsize<uri_qm_query_frag_tag, DestCodec>();
        }
        else
        {
            if constexpr(has<uri_qm_query_tag>())
            {
                n += mxsize<uri_qm_query_tag, DestCodec>();
            }
            else if constexpr(has<uri_query_tag>())
            {
                if(non_empty<uri_query_tag>())
                    n += 1 + mxsize<uri_query_tag, DestCodec>();
            }

            if constexpr(has<uri_sharp_frag_tag>())
            {
                n += mxsize<uri_sharp_frag_tag, DestCodec>();
            }
            else if constexpr(has<uri_frag_tag>())
            {
                if(non_empty<uri_frag_tag>())
                    n += 1 + mxsize<uri_frag_tag, DestCodec>();
            }
        }

        return n;
    }

    //                            v: presents when authority presents
    // URI = scheme:[//authority](/)path[?query][#fragment]
    //                 authority = [userinfo@]host[:port]
    //                              userinfo = user[:password]
    //
    // FileURI = 'file://[host]/path'

    // TODO: sync with max_write_size()
    // NOTE: just assume everything necessary presented
    //       if overlapped parts present, prefer the wider parts.
    template<uri_codec_e DestCodec>
    aresult<char*> append_to(char* d) const
    {
        [[maybe_unused]] char const* const beg = d;

        if constexpr(has<uri_scheme_authority_tag>())
        {
            JKL_TRY(d, do_write<uri_scheme_authority_tag, DestCodec>(d));
        }
        else
        {
            if constexpr(has<uri_scheme_colon_tag>())
            {
                JKL_TRY(d, do_write<uri_scheme_colon_tag, DestCodec>(d));
            }
            else if constexpr(has<uri_scheme_tag>())
            {
                if(non_empty<uri_scheme_tag>())
                {
                    JKL_TRY(d, do_write<uri_scheme_tag, DestCodec>(d));
                    *d++ = ':';
                }
            }

            if constexpr(has<uri_ss_authority_tag>())
            {
                JKL_TRY(d, do_write<uri_ss_authority_tag, DestCodec>(d));
            }
            else if constexpr(has<uri_authority_tag>())
            {
                *d++ = '/'; *d++ = '/'; // NOTE: since uri_authority_tag presents, we'll write "//" anyway.
                JKL_TRY(d, do_write<uri_authority_tag, DestCodec>(d));
            }
            else
            {
                *d++ = '/'; *d++ = '/'; // NOTE: since uri_[userinfo/user/...authority children]_tag presents, we'll write "//" anyway.

                if constexpr(has<uri_userinfo_at_tag>())
                {
                    JKL_TRY(d, do_write<uri_userinfo_at_tag, DestCodec>(d));
                }
                else if constexpr(has<uri_userinfo_tag>())
                {
                    if(non_empty<uri_userinfo_tag>())
                    {
                        JKL_TRY(d, do_write<uri_userinfo_tag, DestCodec>(d));
                        *d++ = '@';
                    }
                }
                else if constexpr(has<uri_user_tag>())
                {
                    if(non_empty<uri_user_tag>())
                    {
                        JKL_TRY(d, do_write<uri_user_tag, DestCodec>(d));

                        if constexpr(has<uri_password_tag>())
                        {
                            if(non_empty<uri_password_tag>())
                            {
                                *d++ = ':';
                                JKL_TRY(d, do_write<uri_password_tag, DestCodec>(d));
                            }
                        }
                        *d++ = '@';
                    }
                }

                if constexpr(has<uri_host_port_tag>())
                {
                    JKL_TRY(d, do_write<uri_host_port_tag, DestCodec>(d));
                }
                else
                {
                    if constexpr(has<uri_host_tag>())
                    {
                        JKL_TRY(d, do_write<uri_host_tag, DestCodec>(d));
                    }

                    if constexpr(has<uri_port_tag>())
                    {
                        if(non_empty<uri_port_tag>())
                        {
                            *d++ = ':';
                            JKL_TRY(d, do_write<uri_port_tag, DestCodec>(d));
                        }
                    }
                    else if constexpr(has<uri_auto_port_tag>())
                    {
                        JKL_TRY(d, get<uri_auto_port_tag>().append_colon_port_to(d, string_view{beg, d}));
                    }
                }
            }
        }

        if constexpr(has<uri_path_tag>())
        {
            JKL_TRY(d, do_write<uri_path_tag, DestCodec>(d));
        }
        else
        {
            if constexpr(has<uri_dir_path_tag>())
            {
                char* b = d;
                JKL_TRY(d, do_write<uri_dir_path_tag, DestCodec>(d));

                if(b == d || *(d-1) != '/')
                    *d++ = '/';
            }

            if constexpr(has<uri_file_tag>())
            {
                JKL_TRY(d, do_write<uri_file_tag, DestCodec>(d));
            }
            else
            {
                if constexpr(has<uri_file_name_tag>())
                {
                    JKL_TRY(d, do_write<uri_file_name_tag, DestCodec>(d));
                }

                if constexpr(has<uri_file_dot_ext_tag>())
                {
                    JKL_TRY(d, do_write<uri_file_dot_ext_tag, DestCodec>(d));
                }
                else if constexpr(has<uri_file_ext_tag>())
                {
                    *d++ = '.';
                    JKL_TRY(d, do_write<uri_file_ext_tag, DestCodec>(d));
                }
            }
        }

        if constexpr(has<uri_qm_query_frag_tag>())
        {
            JKL_TRY(d, do_write<uri_qm_query_frag_tag, DestCodec>(d));
        }
        else
        {
            if constexpr(has<uri_qm_query_tag>())
            {
                JKL_TRY(d, do_write<uri_qm_query_tag, DestCodec>(d));
            }
            else if constexpr(has<uri_query_tag>())
            {
                if(non_empty<uri_query_tag>())
                {
                    *d++ = '?';
                    JKL_TRY(d, do_write<uri_query_tag, DestCodec>(d));
                }
            }

            if constexpr(has<uri_sharp_frag_tag>())
            {
                JKL_TRY(d, do_write<uri_sharp_frag_tag, DestCodec>(d));
            }
            else if constexpr(has<uri_frag_tag>())
            {
                if(non_empty<uri_frag_tag>())
                {
                    *d++ = '#';
                    JKL_TRY(d, do_write<uri_frag_tag, DestCodec>(d));
                }
            }
        }

        return d;
    }

    template<uri_codec_e DestCodec>
    aresult<> append_to(_resizable_char_buf_ auto& d) const
    {
        JKL_TRY(char* const end, append_to<DestCodec>(buy_buf(d, max_write_size())));
        resize_buf(d, end - buf_data(d));
        return no_err;
    }

    template<uri_codec_e DestCodec>
    aresult<> assign_to(_resizable_char_buf_ auto& d) const
    {
        resize_buf(d, max_write_size<DestCodec>());
        JKL_TRY(char* const end, append_to<DestCodec>(buf_data(d)));
        resize_buf(d, end - buf_data(d));
        return no_err;
    }

    template<_resizable_char_buf_ B = string>
    B encoded() const
    {
        B d;
        assign_to<url_encoded>(d).throw_on_error();
        return d;
    }

    template<_resizable_char_buf_ B = string>
    B decoded() const
    {
        B d;
        assign_to<url_decoded>(d).throw_on_error();
        return d;
    }
};


template<class... T>
uri_writer(T&&...) -> uri_writer<T&&...>;


template<uri_codec_e DestCodec>
auto append_uri(auto&& d, _uri_comp_ auto const&... comps)
{
    return uri_writer{comps...}.append_to<DestCodec>(JKL_FORWARD(d));
}

template<uri_codec_e DestCodec>
aresult<> assign_uri(_resizable_char_buf_ auto& d, _uri_comp_ auto const&... comps)
{
    return uri_writer{comps...}.assign_to<DestCodec>(d);
}

// for convert uri_uri codec.

template<uri_codec_e DestCodec, _uri_uri_ U>
aresult<> append_uri(_resizable_char_buf_ auto& d,  U const& u)
{
    auto schemeColon = uri_scheme_colon <U::codec>(string_view{});
    auto ssAuth      = uri_ss_authority <U::codec>(string_view{});
    auto path        = uri_path         <U::codec>(string_view{});
    auto queryFrag   = uri_qm_query_frag<U::codec>(string_view{});

    JKL_TRY(read_uri(u, schemeColon, ssAuth, path, queryFrag));

    return append_uri<DestCodec>(d, schemeColon, ssAuth, path, queryFrag);
}

template<uri_codec_e DestCodec, _uri_uri_ U>
aresult<> assign_uri(_resizable_char_buf_ auto& d,  U const& u)
{
    auto schemeColon = uri_scheme_colon <U::codec>(string_view{});
    auto ssAuth      = uri_ss_authority <U::codec>(string_view{});
    auto path        = uri_path         <U::codec>(string_view{});
    auto queryFrag   = uri_qm_query_frag<U::codec>(string_view{});

    JKL_TRY(read_uri(u, schemeColon, ssAuth, path, queryFrag));

    return assign_uri<DestCodec>(d, schemeColon, ssAuth, path, queryFrag);
}


template<uri_codec_e DestCodec, _resizable_char_buf_ B = string>
aresult<B> make_uri(_uri_comp_ auto const&... comps)
{
    B d;
    JKL_TRY(assign_uri<DestCodec>(d, comps...));
    return d;
}


} // namespace jkl
