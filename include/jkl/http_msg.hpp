#pragma once

#include <jkl/config.hpp>
#include <jkl/util/small_vector.hpp>
#include <jkl/charset/ascii.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>


namespace jkl{


using http_fields           = beast::http::fields;
using http_request_header   = beast::http::request_header<>;
using http_response_header  = beast::http::response_header<>;

template<class Body = beast::http::string_body>
using http_request_t = beast::http::request<Body>;

template<class Body = beast::http::string_body>
using http_response_t = beast::http::response<Body>;

using http_request  = http_request_t<>;
using http_response = http_response_t<>;


constexpr bool is_http_lws(char c) noexcept
{
    return c == ' ' || c == '\t';
}


struct http_field_content_type
{
    string mime;    // lowercase, type "/" subtype
    string charset; // lowercase
    string boundary;

    // parse all Content-Type fields, ignore any error
    //
    // media-type = type "/" subtype *( ";" parameter )
    // parameter  = attribute "=" value
    // value      = token | quoted-string
    //
    // e.g.:
    // Content-Type: text/html; charset=ISO-8859-4
    // Content-Type: multipart/mixed; boundary=gc0p4Jq0M2Yt08jU534c0p
    //
    // actual parameter depends on type "/" subtype
    // see: https://www.iana.org/assignments/media-types/media-types.xhtml#text
    void parse(http_fields const& flds)
    {
        auto[beg, end] = flds.equal_range(beast::http::field::content_type);
        for(; beg != end; ++beg)
            parse(beg->value());
    }

    // chromium/git/net/http/http_util.cc
    void parse(beast::string_view const& str)
    {
        // Trim leading and trailing whitespace from type.  We include '(' in
        // the trailing trim set to catch media-type comments, which are not at all
        // standard, but may occur in rare cases.
        auto mimeBeg = str.find_first_not_of(" \t");
        if(mimeBeg == npos)
            return;

        auto mimeEnd = str.find_first_of(" \t;(", mimeBeg);

        auto mv = str.substr(mimeBeg, mimeEnd == npos ? mimeEnd : mimeEnd - mimeBeg);

        if(auto q = mv.find('/'); q == npos || q == 0 || q + 1 == mv.size() || mv == "*/*") // "*/*" is meaningless
            return;

        ascii_tolower_assign(mime, mv);

        if(mimeEnd == npos)
            return;

        // Iterate over parameters. Can't split the string around semicolons
        // preemptively because quoted strings may include semicolons. Mostly matches
        // logic in https://mimesniff.spec.whatwg.org/. Main differences: Does not
        // validate characters are HTTP token code points / HTTP quoted-string token
        // code points, and ignores spaces after "=" in parameters.

        auto offset = str.find(';', mimeEnd);
        if(offset == npos)
            return;

        for(;;)
        {
            BOOST_ASSERT(';' == str[offset]);
            ++offset;

            offset = str.find_first_not_of(" \t", offset);
            if(offset == npos)
                return;

            auto nameBeg = offset;

            // Extend parameter name until run into a semicolon or equals sign.  Per
            // spec, trailing spaces are not removed.
            offset = str.find_first_of(";=", offset);
            if(offset == npos)
                return;

            if(str[offset] == ';')
                continue; // ignore names without values

            auto name = str.substr(nameBeg, offset - nameBeg);

            // Now parse the value.
            BOOST_ASSERT('=' == str[offset]);
            ++offset;

            // Remove leading spaces. This violates the spec, though it matches
            // pre-existing behavior.
            //
            // TODO: Consider doing this (only?) after parsing quotes, which
            // seems to align more with the spec - not the content-type spec, but the
            // GET spec's way of getting an encoding, and the spec for handling
            // boundary values as well.
            // See https://encoding.spec.whatwg.org/#names-and-labels.
            offset = str.find_first_not_of(" \t", offset);
            if(offset == npos)
                return;

            if(str[offset] == ';')
                continue; // an unquoted string of only whitespace should be skipped.

            string value;

            if(str[offset] != '"')
            {
                auto vbeg = offset;
                offset = str.find(';', offset);

                auto vend = offset;
                if(vend == npos)
                    vend = str.size();

                // trim LWS at end
                while(vbeg < vend && is_http_lws(str[vend - 1]))
                    --vend;

                value = str.substr(vbeg, vend - vbeg);
            }
            else
            {
                // quoted string

                BOOST_ASSERT('"' == str[offset]);
                ++offset;

                auto vbeg = offset;
                offset = str.find('"', offset);

                auto vend = offset;
                if(vend == npos)
                    vend = str.size();

                // trim LWS
                while(vbeg < vend && is_http_lws(str[vbeg]))
                    ++vbeg;
                while(vbeg < vend && is_http_lws(str[vend - 1]))
                    --vend;

                while(vbeg < vend && str[vbeg] != '"')
                {
                    // Skip over backslash and append the next character, when not at
                    // the end of the string. Otherwise, copy the next character (Which may
                    // be a backslash).
                    if(str[vbeg] == '\\' && vbeg + 1 < vend)
                        ++vbeg;

                    value += str[vbeg];
                    ++vbeg;
                }

                offset = str.find(';', offset);
            }

            // TODO: Check that name has only valid characters.
            if(charset.empty() && ascii_iequal(name, "charset"))
            {
                charset = std::move(value);
                ascii_tolower_inplace(charset);
            }
            else if(boundary.empty() && ascii_iequal(name, "boundary"))
            {
                boundary = std::move(value);
            }

            if(offset == npos)
                return;
        }
    }
};


inline http_field_content_type parse_http_content_type(http_fields const& flds)
{
    http_field_content_type t;
    t.parse(flds);
    return t;
}


} // namespace jkl
