#pragma once

#include <jkl/config.hpp>
#include <jkl/util/base64.hpp>
#include <jkl/uri/url_codec.hpp>


namespace jkl{


enum class data_url_err
{
    invliad_mime = 1,
    not_data_url = 2,
    no_data = 3,
    empty_type_without_charset = 4
};

class data_url_err_category : public aerror_category
{
public:
    char const* name() const noexcept override { return "data_url"; }

    std::string message(int condition) const override
    {
        switch(condition)
        {
            case static_cast<int>(data_url_err::invliad_mime) :
                return "invliad MIME";
            case static_cast<int>(data_url_err::not_data_url) :
                return "not a data url";
            case static_cast<int>(data_url_err::no_data) :
                return "data part missing";
            case static_cast<int>(data_url_err::empty_type_without_charset) :
                return "empty type without charset";
        }

        return "undefined";
    }
};

inline constexpr data_url_err_category g_data_url_err_cat;
inline auto const& data_url_err_cat() noexcept { return g_data_url_err_cat; }

inline aerror_code      make_error_code     (data_url_err e) noexcept { return {static_cast<int>(e), data_url_err_cat()}; }
inline aerror_condition make_error_condition(data_url_err e) noexcept { return {static_cast<int>(e), data_url_err_cat()}; }


} // namespace jkl


namespace boost::system{

template<> struct is_error_code_enum<jkl::data_url_err> : public std::true_type {};

} // namespace boost::system


namespace jkl{


// ref: https://tools.ietf.org/html/rfc2397
// 
// dataurl    := "data:" [ mediatype ] [ ";base64" ] "," data
// mediatype  := [ type "/" subtype ] *( ";" parameter )
// data       := *urlchar
// parameter  := attribute "=" value
// 
// If <mediatype> is omitted, it defaults to text/plain;charset=US-ASCII.
// As a shorthand, "text/plain" can be omitted but the charset parameter supplied.

struct data_url_parse_result
{
    string mime;    // lowercase
    string charset; // lowercase
    string data;
    bool   is_base64 = false;
};


template<bool Decode = true>
inline aresult<data_url_parse_result> parse_data_url(string_view u)
{
    if(! u.starts_with("data:"))
        return data_url_err::not_data_url;
    u.remove_prefix(5);

    auto q = u.find(',');
    if(q == npos)
        return data_url_err::no_data;

    data_url_parse_result r;

    auto data = u.substr(q + 1);
    u.remove_suffix(data.size() + 1);

    if(r.is_base64 = u.ends_with(";base64"))
        u.remove_suffix(7);

    if(u.empty())
    {
        r.mime = "text/plain";
        r.charset = "US-ASCII"
    }
    else
    {
        q = u.find(';');
        ascii_tolower_assign(r.mime, u.substr(q));

        if(q != npos)
        {
            if(q = u.find(";charset=", q); q != npos)
                ascii_tolower_assign(r.charset, u.substr(q + 9));
        }

        if(r.mime.empty())
        {
            if(r.charset.empty())
                return data_url_err::empty_type_without_charset;
            r.mime = "text/plain";
        }
        else
        {
            q = r.mime.find('/');
            if(q == npos || q == 0 || q + 1 == r.mime.size())
                return data_url_err::invliad_mime;
        }
    }

    if constexpr(Decode)
    {
        if(r.is_base64)
        {
            JKL_TRY(base64_url::decode_assign(r.data, data));
        }
        else
        {
            JKL_TRY(url_decode_assign<uri_user_tag>(r.data, data));
        }
    }
    else
    {
        r.data = data;
    }

    return r;
}


} // namespace jkl
