#pragma once

#include <jkl/config.hpp>
#include <jkl/params.hpp>
#include <jkl/util/str.hpp>
#include <jkl/uri/comp.hpp>
#include <jkl/uri/reader.hpp>


namespace jkl{


// Remove Dot Segments in path
// ref: https://tools.ietf.org/html/rfc3986#section-5.2.4

                                  // vvvvvvvvv: in and out may ref same buf
template<uri_codec_e DestCodec, bool SameInOut = false, _uri_path_ P>
aresult<char*> uri_remove_dot_segments_append(char* const out, P const& path)
{
    static_assert((SameInOut && (DestCodec == P::codec || DestCodec == as_is_codec || P::codec == as_is_codec)) || !SameInOut);
    BOOST_ASSERT(SameInOut == (str_data(path.str()) == out));

    if(path.empty())
        return out;

    string_view in{path.str()};
    bool isRel = (in[0] != '/');

    // make sure 'in' and 'out' are same.
    // while scanning, segments are always copied to left, so no overwrite can happen.
    if constexpr(! SameInOut)
    {
        JKL_TRY(char* e, path.template write<DestCodec>(out));
        in = {out, static_cast<size_t>(e - out)};
    }

    char* op = out;

    do
    {
        // A. If the input buffer begins with a prefix of "../" or "./",
        //    then remove that prefix from the input buffer;
        if(in.starts_with("./"))
        {
            in.remove_prefix(2);
        }
        else if(in.starts_with("../"))
        {
            in.remove_prefix(3);
        }

        // B. if the input buffer begins with a prefix of "/./" or "/.",
        //    where "."  is a complete path segment, then replace that prefix with "/" in
        //    the input buffer;
        else if(in.starts_with("/./"))
        {
            in.remove_prefix(2);
        }
        else if(in == "/.")
        {
            const_cast<char&>(in[1]) = '/';
            in.remove_prefix(1);
        }

        // C. if the input buffer begins with a prefix of "/../" or "/..",
        //    where ".." is a complete path segment, then replace that prefix with "/"
        //    in the input buffer and remove the last segment and its preceding "/" (if
        //    any) from the output buffer;
        else if(in.starts_with("/../"))
        {
            in.remove_prefix(3);

            // remove the last segment from the output buffer
            while(op > out)
            {
                --op;
                if(*op == '/')
                    break;
            }
        }
        else if(in == "/..")
        {
            const_cast<char&>(in[2]) = '/';
            in.remove_prefix(2);

            // remove the last segment from the output buffer
            while(op > out)
            {
                --op;
                if(*op == '/')
                    break;
            }
        }

        // D. if the input buffer consists only of "." or "..",
        //    then remove that from the input buffer;
        else if(in == "." || in == "..")
        {
            op = out;
            break;
        }
        else
        {
            // E. move the first path segment in the input buffer to the end of
            //    the output buffer, including the initial "/" character (if any) and
            //    any subsequent characters up to, but not including, the next "/"
            //    character or the end of the input buffer.

            // skip first '/', if the original path is relative.
            size_t pos = ((isRel && op == out && in[0] == '/') ? 1 : 0);

            if(pos || (op != in.data()))
            {
                auto ncopied = in.copy(op, in.find('/', 1), pos);
                op += ncopied;
                in.remove_prefix(ncopied + pos);
            }
            else
            {
                auto q = in.find('/', 1);
                if(q == npos)
                    q = in.size();
                op += q;
                in.remove_prefix(q);
            }
        }
    }
    while(in.size());

    return op;
}

template<uri_codec_e DestCodec, bool SameInOut = false>
aresult<> uri_remove_dot_segments_assign(_resizable_char_buf_ auto& out, _uri_path_ auto const& path)
{
    if constexpr(! SameInOut)
        resize_buf(out, path.template max_write_size<DestCodec>());
    JKL_TRY(char* e, uri_remove_dot_segments_append<DestCodec, SameInOut>(buf_data(out), path));
    resize_buf(out, e - buf_data(out));
    return no_err;
}

template<uri_codec_e DestCodec, _resizable_char_buf_ B = string>
aresult<B> uri_remove_dot_segments_ret(_uri_path_ auto const& path)
{
    B out;
    JKL_TRY(uri_remove_dot_segments_assign<DestCodec>(out, path));
    return out;
}

// return where it ends
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
inline aresult<char*> uri_remove_dot_segments_inplace(_uri_path_ auto const& path)
{
    return uri_remove_dot_segments_append<as_is_codec, true>(path.str().data(), path);
}

inline aresult<char*> uri_remove_dot_segments_inplace(char* beg, char* end)
{
    return uri_remove_dot_segments_append<as_is_codec, true>(beg, uri_path(string_view{beg, end}));
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
aresult<> uri_remove_dot_segments_inplace(_resizable_char_buf_ auto& inOut)
{
    return uri_remove_dot_segments_assign<as_is_codec, true>(inOut, uri_path(inOut));
}


// resolve 'ref' path, using 'base' as base path
// NOTE: 'ref' must NOT be empty
// absolute path: path starts with '/', the base is authority root.
// relative path: path does NOT start with '/', relative to a base path.
//
// https://tools.ietf.org/html/rfc3986#section-5.2.2
// https://tools.ietf.org/html/rfc3986#section-5.2.3
template<uri_codec_e DestCodec, _uri_path_ B, _uri_path_ R>
aresult<char*> uri_resolve_path_append(char* tar, B const& base, R const& ref, bool treatEmptyBaseAsRoot = false)
{
    BOOST_ASSERT(ref.size());

    auto beg = tar;

    if(ref.str()[0] != '/')
    {
        if(auto q = base.rfind('/'); q != npos)
        {
            JKL_TRY(tar, base.subcomp(0, q).template write<DestCodec>(tar));
            *tar++ = '/';
        }
        else
        {
            if(treatEmptyBaseAsRoot && base.empty())
                *tar++ = '/';
        }
    }

    JKL_TRY(tar, ref.template write<DestCodec>(tar));

    return uri_remove_dot_segments_inplace(beg, tar);
}

template<uri_codec_e DestCodec>
aresult<> uri_resolve_path_assign(_resizable_char_buf_ auto& tar, _uri_path_ auto const& base, _uri_path_ auto const& ref,
                                  bool treatEmptyBaseAsRoot = false)
{
    resize_buf(tar, base.template max_write_size<DestCodec>() + 1 + ref.template max_write_size<DestCodec>());
    JKL_TRY(char* e, uri_resolve_path_append<DestCodec>(buf_data(tar), base, ref, treatEmptyBaseAsRoot));
    resize_buf(tar, e - buf_data(tar));
    return no_err;
}

template<uri_codec_e DestCodec, _resizable_char_buf_ B = string>
aresult<B> uri_resolve_path_ret(_uri_path_ auto const& base, _uri_path_ auto const& ref, bool treatEmptyBaseAsRoot = false)
{
    B tar;
    JKL_TRY(uri_resolve_path_assign<DestCodec>(tar, base, ref, treatEmptyBaseAsRoot));
    return tar;
}



// 'tar' must be at least same size as base.max_write_size<DestCodec>() + 1 + ref.max_write_size<DestCodec>()
// 'base' must be an uri with scheme, and can NOT contain dot segments
// 'ref' can be absolute or relative, and may contain dot segments
//
// ref https://tools.ietf.org/html/rfc3986#section-5.2.2
template<uri_codec_e DestCodec, _uri_uri_ B, _uri_uri_ R>
aresult<char*> uri_resolve_append(char* tar, B const& base, R const& ref, auto... p)
{
    auto params = make_params(p..., p_keep_frag);
    constexpr bool skipFrag = params(t_skip_frag);

    auto rscheme     = uri_scheme          <R::codec>(string_view{});
    auto rauth       = uri_ss_authority    <R::codec>(string_view{});
    auto rSchemeAuth = uri_scheme_authority<R::codec>(string_view{});
    auto rpath       = uri_path            <R::codec>(string_view{});
    auto rquery      = uri_qm_query        <R::codec>(string_view{});
    auto rfrag       = uri_sharp_frag      <R::codec>(string_view{});

    JKL_TRYV(read_uri<false>(ref, rscheme, rauth, rSchemeAuth, rpath, rquery, rfrag));

    if(rscheme.size())
    {
        JKL_TRY(tar, rSchemeAuth.template write<DestCodec>(tar));
        JKL_TRY(tar, uri_remove_dot_segments_append<DestCodec>(tar, rpath));
        JKL_TRY(tar, rquery.template write<DestCodec>(tar));

        if constexpr(skipFrag)
            return tar;
        else
            return rfrag.template write<DestCodec>(tar);
    }

    auto bscheme = uri_scheme_colon<B::codec>(string_view{});
    auto bauth   = uri_ss_authority<B::codec>(string_view{});
    auto bpath   = uri_path        <B::codec>(string_view{});
    auto bquery  = uri_qm_query    <B::codec>(string_view{});

    JKL_TRYV(read_uri<false>(base, bscheme, bauth, bpath, bquery));

    BOOST_ASSERT(bscheme.size());

    JKL_TRY(tar, bscheme.template write<DestCodec>(tar));

    if(rauth.size())
    {
        JKL_TRY(tar, rauth.template write<DestCodec>(tar));
        JKL_TRY(tar, uri_remove_dot_segments_append<DestCodec>(tar, rpath));
        JKL_TRY(tar, rquery.template write<DestCodec>(tar));
    }
    else
    {
        JKL_TRY(tar, bauth.template write<DestCodec>(tar));

        if(rpath.empty())
        {
            JKL_TRY(tar, bpath.template write<DestCodec>(tar));

            if(rquery.size())
            {
                JKL_TRY(tar, rquery.template write<DestCodec>(tar));
            }
            else
            {
                JKL_TRY(tar, bquery.template write<DestCodec>(tar));
            }
        }
        else
        {
            JKL_TRY(tar, uri_resolve_path_append<DestCodec>(tar, bpath, rpath, bauth.non_empty()));
            JKL_TRY(tar, rquery.template write<DestCodec>(tar));
        }
    }

    if constexpr(skipFrag)
        return tar;
    else
        return rfrag.template write<DestCodec>(tar);
}

template<uri_codec_e DestCodec>
aresult<> uri_resolve_assign(_resizable_char_buf_ auto& tar, _uri_uri_ auto const& base, _uri_uri_ auto const& ref, auto... p)
{
    resize_buf(tar, base.template max_write_size<DestCodec>() + 1 + ref.template max_write_size<DestCodec>());
    JKL_TRY(char* e, uri_resolve_append<DestCodec>(buf_data(tar), base, ref, p...));
    resize_buf(tar, e - buf_data(tar));
    return no_err;
}

template<uri_codec_e DestCodec, _resizable_char_buf_ T = string>
aresult<T> uri_resolve_ret(_uri_uri_ auto const& base, _uri_uri_ auto const& ref, auto... p)
{
    T tar;
    JKL_TRY(uri_resolve_assign<DestCodec>(tar, base, ref, p...));
    return tar;
}


} // namespace jkl
