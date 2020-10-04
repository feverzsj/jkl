#pragma once

#include <jkl/config.hpp>
#include <jkl/util/str.hpp>
#include <jkl/uri/comp.hpp>
#include <jkl/uri/reader.hpp>
#include <jkl/uri/resolve.hpp>


namespace jkl{


template<bool PcntEncoded>
aresult<char*> normalize_pcnt_encoding_append(char* d, _char_str_ auto const& s)
{
    if constexpr(PcntEncoded)
    {
        char const* it = str_begin(s);
        char const* const end = str_end(s);

        while(it != end)
        {
            *d++ = *it++;

            if(*it == '%')
            {
                char c;
                if(! unhex(it, end, c))
                    return uri_err::invalid_hex;
                *d++ = ascii_toupper(*it++);
                *d++ = ascii_toupper(*it++);
            }
        }

        return d;
    }
    else
    {
        return append_str(d, s);
    }
}

template<bool PcntEncoded>
aresult<char*> normalize_pcnt_encoding_inplace(char* beg, char* const end)
{
    if constexpr(PcntEncoded)
    {
        while(beg != end)
        {
            if(*beg++ == '%')
            {
                char c;
                if(! unhex(beg, end, c))
                    return uri_err::invalid_hex;
                ascii_toupper_inplace(*beg++);
                ascii_toupper_inplace(*beg++);
            }
        }
    }

    return end;
}


// NOTE: to compare equality of 2 uri, they are better both in decoded normalized form.
//
// scheme  : lowercase
// userinfo: uppercase percent-encodings
// host    : uppercase percent-encodings and lowercase others
// path    : uppercase percent-encodings and remove dot segments
// query   : uppercase percent-encodings
// fragment: uppercase percent-encodings
template<uri_codec_e DestCodec, _uri_comp_ Comp>
aresult<char*> uri_normalize_append(char* d, Comp const& comp)
{
    using tag = typename Comp::tag;
    constexpr bool pcntEncoded = ((DestCodec == as_is_codec ? Comp::codec : DestCodec) == url_encoded);

    if constexpr(is_one_of_v<tag, uri_scheme_tag, uri_scheme_colon_tag, uri_port_tag>)
    {
        char* b = d;
        JKL_TRY(d, comp.template write<DestCodec>(d));
        return ascii_tolower_inplace(b, d);
    }
    else if constexpr(is_one_of_v<tag, uri_user_tag, uri_password_tag, uri_userinfo_tag, uri_userinfo_at_tag,
                                       uri_query_tag, uri_qm_query_tag, uri_frag_tag, uri_sharp_frag_tag, uri_qm_query_frag_tag>)
    {
        char* b = d;
        JKL_TRY(d, comp.template write<DestCodec>(d));
        return normalize_pcnt_encoding_inplace<pcntEncoded>(b, d);
    }
    else if constexpr(is_one_of_v<tag, uri_host_tag, uri_host_port_tag>)
    {
        char* b = d;
        JKL_TRY(d, comp.template write<DestCodec>(d));
        ascii_tolower_inplace(b, d);
        return normalize_pcnt_encoding_inplace<pcntEncoded>(b, d);
    }
    else if constexpr(is_one_of_v<tag, uri_path_tag,
                                       uri_dir_path_tag, uri_file_tag, uri_file_name_tag, uri_file_ext_tag, uri_file_dot_ext_tag>)
    {
        char* b = d;
        JKL_TRY(d, uri_remove_dot_segments_append<DestCodec>(d, comp));
        return normalize_pcnt_encoding_inplace<pcntEncoded>(b, d);
    }
    else if constexpr(is_one_of_v<tag, uri_authority_tag>)
    {
        if(auto q = comp.find('@'); q != npos)
        {
            JKL_TRY(d, uri_normalize_append<DestCodec>(d, comp.template subcomp<uri_userinfo_tag>(0, q)));
            *d++ = '@';
            return uri_normalize_append<DestCodec>(d, comp.template subcomp<uri_host_port_tag>(q + 1));
        }
        return uri_normalize_append<DestCodec>(d, comp.template subcomp<uri_host_port_tag>());
    }
    else if constexpr(is_one_of_v<tag, uri_ss_authority_tag>)
    {
        if(comp.size())
        {
            BOOST_ASSERT(comp.starts_with("//"));

            *d++ = '/'; *d++ = '/';
            return uri_normalize_append<DestCodec>(d, comp.template subcomp<uri_authority_tag>(2));
        }
    }
    else if constexpr(is_one_of_v<tag, uri_scheme_authority_tag>)
    {
        if(comp.size())
        {
            auto q = comp.find('/');
            BOOST_ASSERT(q != npos);
            JKL_TRY(d, uri_normalize_append<DestCodec>(d, comp.template subcomp<uri_scheme_colon_tag>(0, q)));
            return uri_normalize_append<DestCodec>(d, comp.template subcomp<uri_ss_authority_tag>(q));
        }
    }
    else if constexpr(is_one_of_v<tag, uri_uri_tag>)
    {
        auto scheme    = uri_scheme_colon <Comp::codec>(string_view{});
        auto userinfo  = uri_userinfo     <Comp::codec>(string_view{});
        auto hostPort  = uri_host_port    <Comp::codec>(string_view{});
        auto path      = uri_path         <Comp::codec>(string_view{});
        auto queryFrag = uri_qm_query_frag<Comp::codec>(string_view{});

        JKL_TRY(read_uri(comp, scheme, userinfo, hostPort, path, queryFrag));

        JKL_TRY(d, uri_normalize_append<DestCodec>(d, scheme));

        if(userinfo.size())
        {
            JKL_TRY(d, uri_normalize_append<DestCodec>(d, userinfo));
            *d++ = '@';
        }

        JKL_TRY(d, uri_normalize_append<DestCodec>(d, hostPort));
        JKL_TRY(d, uri_normalize_append<DestCodec>(d, path));
        return uri_normalize_append<DestCodec>(d, queryFrag);
    }
}


template<uri_codec_e DestCodec>
aresult<> uri_normalize_assign(_resizable_char_buf_ auto& d, _uri_comp_ auto const& comp)
{
    resize_buf(d, comp.template max_write_size<DestCodec>());
    JKL_TRY(char* e, uri_normalize_append<DestCodec>(buf_data(d), comp));
    resize_buf(d, e - buf_data(d));
    return no_err;
}

template<uri_codec_e DestCodec, _resizable_char_buf_ D = string>
aresult<D> uri_normalize_ret(_uri_comp_ auto const& comp)
{
    D d;
    JKL_TRY(uri_normalize_assign<DestCodec>(d, comp));
    return d;
}


} // namespace jkl
