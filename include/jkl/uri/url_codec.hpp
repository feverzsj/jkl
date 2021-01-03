#pragma once

#include <jkl/config.hpp>
#include <jkl/util/str.hpp>
#include <jkl/util/stringify.hpp>
#include <jkl/charset/ascii.hpp>
#include <jkl/uri/fwd.hpp>
#include <jkl/uri/error.hpp>


namespace jkl{


constexpr char* append_hex(char* d, char c) noexcept
{
    constexpr char const* hex = "0123456789ABCDEF";
    auto const u = static_cast<unsigned char>(c);
    d[0] = hex[u >> 4 ];
    d[1] = hex[u & 0xf];
    return d + 2;
}

inline constexpr unsigned char g_lut_hex_to_dec[256] = {
#define __ 0xFF
    __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 00-0F */
    __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 10-1F */
    __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 20-2F */
     0, 1, 2, 3, 4, 5, 6, 7, 8, 9,__,__,__,__,__,__, /* 30-3F */
    __,10,11,12,13,14,15,__,__,__,__,__,__,__,__,__, /* 40-4F */
    __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 50-5F */
    __,10,11,12,13,14,15,__,__,__,__,__,__,__,__,__, /* 60-6F */
    __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 70-7F */
    __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 80-8F */
    __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 90-9F */
    __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* A0-AF */
    __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* B0-BF */
    __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* C0-CF */
    __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* D0-DF */
    __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* E0-EF */
    __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* F0-FF */
#undef __
};

constexpr bool unhex(char const* beg, char const* end, char& c) noexcept
{
    if(end - beg >= static_cast<std::ptrdiff_t>(sizeof(c) * 2))
    {
        unsigned char const hi = g_lut_hex_to_dec[static_cast<unsigned char>(beg[0])];
        unsigned char const lo = g_lut_hex_to_dec[static_cast<unsigned char>(beg[1])];

        if((hi | lo) != 0xFF)
        {
            c = static_cast<char>((hi << 4) | lo);
            return true;
        }
    }
    return false;
}

namespace url_codec{


enum no_esc_cat : unsigned
{
    no_esc_query    = 1,
    no_esc_frag     = 2,
    no_esc_userinfo = 4,
    no_esc_host     = 8,
    no_esc_path     = 16
};

// combines: kSharedCharTypeTable(url_canon_internal.cc), kHostCharLookup(url_canon_host.cc),
//           kPathCharLookup(url_canon_path.cc), kShouldEscapeCharInFragment(url_canon_etc.cc)
// from chromium/url into this table.
inline constexpr unsigned char g_lut_no_esc[256] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                            // 0x00 - 0x0f
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                            // 0x10 - 0x1f
    0,                                                                         // 0x20  ' ' (escape spaces in queries)
    no_esc_query | no_esc_frag | no_esc_userinfo |               no_esc_path,  // 0x21  !
    0,                                                                         // 0x22  "
                   no_esc_frag,                                                // 0x23  #  (invalid in query since it marks the ref)
    no_esc_query | no_esc_frag | no_esc_userinfo |               no_esc_path,  // 0x24  $
    0,                                                                         // 0x25  %
    no_esc_query | no_esc_frag | no_esc_userinfo |               no_esc_path,  // 0x26  &
    0,                                                                         // 0x27  '  (Try to prevent XSS.)
    no_esc_query | no_esc_frag | no_esc_userinfo |               no_esc_path,  // 0x28  (
    no_esc_query | no_esc_frag | no_esc_userinfo |               no_esc_path,  // 0x29  )
    no_esc_query | no_esc_frag | no_esc_userinfo |               no_esc_path,  // 0x2a  *
                   no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x2b  +
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_path,                // 0x2c  ,
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x2d  -
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x2e  .
    no_esc_query | no_esc_frag |                                 no_esc_path,  // 0x2f  /
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x30  0
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x31  1
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x32  2
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x33  3
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x34  4
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x35  5
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x36  6
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x37  7
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x38  8
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x39  9
    no_esc_query | no_esc_frag |                   no_esc_host | no_esc_path,  // 0x3a  :
    no_esc_query | no_esc_frag |                                 no_esc_path,  // 0x3b  ;
    0,                                                                         // 0x3c  <  (Try to prevent certain types of XSS.)
    no_esc_query | no_esc_frag |                                 no_esc_path,  // 0x3d  =
    0,                                                                         // 0x3e  >  (Try to prevent certain types of XSS.)
    no_esc_query | no_esc_frag,                                                // 0x3f  ?
    no_esc_query | no_esc_frag |                                 no_esc_path,  // 0x40  @
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x41  A
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x42  B
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x43  C
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x44  D
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x45  E
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x46  F
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x47  G
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x48  H
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x49  I
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x4a  J
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x4b  K
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x4c  L
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x4d  M
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x4e  N
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x4f  O
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x50  P
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x51  Q
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x52  R
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x53  S
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x54  T
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x55  U
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x56  V
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x57  W
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x58  X
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x59  Y
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x5a  Z
    no_esc_query | no_esc_frag |                   no_esc_host | no_esc_path,  // 0x5b  [
    no_esc_query | no_esc_frag,                                                // 0x5c  '\'
    no_esc_query | no_esc_frag |                   no_esc_host | no_esc_path,  // 0x5d  ]
    no_esc_query | no_esc_frag,                                                // 0x5e  ^
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x5f  _
    no_esc_query,                                                              // 0x60  `
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x61  a
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x62  b
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x63  c
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x64  d
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x65  e
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x66  f
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x67  g
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x68  h
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x69  i
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x6a  j
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x6b  k
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x6c  l
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x6d  m
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x6e  n
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x6f  o
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x70  p
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x71  q
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x72  r
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x73  s
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x74  t
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x75  u
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x76  v
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x77  w
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x78  x
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x79  y
    no_esc_query | no_esc_frag | no_esc_userinfo | no_esc_host | no_esc_path,  // 0x7a  z
    no_esc_query | no_esc_frag,                                                // 0x7b  {
    no_esc_query | no_esc_frag,                                                // 0x7c  |
    no_esc_query | no_esc_frag,                                                // 0x7d  }
    no_esc_query | no_esc_frag | no_esc_userinfo |               no_esc_path,  // 0x7e  ~
    0,                                                                         // 0x7f
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                            // 0x80 - 0x8f
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                            // 0x90 - 0x9f
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                            // 0xa0 - 0xaf
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                            // 0xb0 - 0xbf
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                            // 0xc0 - 0xcf
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                            // 0xd0 - 0xdf
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                            // 0xe0 - 0xef
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                            // 0xf0 - 0xff
};


template<no_esc_cat NoEscCat>
constexpr bool is_no_esc(char c)
{
    return g_lut_no_esc[static_cast<unsigned char>(c)] & NoEscCat;
}

template<no_esc_cat NoEscCat, bool SpaceToPlus = (NoEscCat == no_esc_query)>
char* escape_append(char* d, _char_str_ auto const& s) noexcept
{
    for(char c : as_str_class(s))
    {
        if constexpr(SpaceToPlus)
        {
            if(c == ' ') // space: 0x20 = 32
            {
                *d++ = '+';
                continue;
            }
        }

        if(is_no_esc<NoEscCat>(c))
        {
            *d++ = c;
            continue;
        }

        *d++ = '%';
        d = append_hex(d, c);
    }

    return d;
}

template<bool PlusToSpace = false>
aresult<char*> unescape_append(char* d, _char_str_ auto const& s)
{
    char const* it  = str_begin(s);
    char const* const end = str_end(s);

    while(it < end)
    {
        char c = *it++;

        // spaces may be encoded as plus signs in the query
        if constexpr(PlusToSpace)
        {
            if(c == '+')
            {
                *d++ = ' ';
                continue;
            }
        }

        if(c == '%')
        {
            if(! unhex(it, end, c))
                return uri_err::invalid_hex;
            it += 2;
        }

        *d++ = c;
    }

    return d;
}


template<class Tag>
constexpr size_t max_encode_size(_stringifible_ auto const& t) noexcept
{
    if constexpr(is_one_of_v<Tag, uri_scheme_tag, uri_scheme_colon_tag, uri_port_tag, uri_auto_port_tag>)
    {
        return max_stringify_size(t);
    }
    else if constexpr(is_one_of_v<Tag, uri_user_tag, uri_password_tag, uri_userinfo_tag, uri_userinfo_at_tag,
                                       uri_host_tag, uri_host_port_tag,
                                       uri_authority_tag, uri_ss_authority_tag, uri_scheme_authority_tag,
                                       uri_path_tag,
                                       uri_dir_path_tag, uri_file_tag, uri_file_name_tag, uri_file_ext_tag, uri_file_dot_ext_tag,
                                       uri_query_tag, uri_qm_query_tag, uri_frag_tag, uri_sharp_frag_tag, uri_qm_query_frag_tag,
                                       uri_uri_tag>)
    {
        return max_stringify_size(t) * 3;
    }
    else
        JKL_DEPENDENT_FAIL(Tag, "unsupported part");
}


// just assume each part use valid characters.
template<class Tag>
char* encode_append(char* d, string_view s) noexcept
{
    if constexpr(is_one_of_v<Tag, uri_scheme_tag, uri_scheme_colon_tag, uri_port_tag, uri_auto_port_tag>)
    {
        return append_str(d, s);
        //return ascii_tolower_append(d, s);
    }
    else if constexpr(is_one_of_v<Tag, uri_user_tag, uri_password_tag>)
    {
        return escape_append<no_esc_userinfo>(d, s);
    }
    else if constexpr(is_one_of_v<Tag, uri_host_tag, uri_host_port_tag>)
    {
        return escape_append<no_esc_host>(d, s);
    }
    else if constexpr(is_one_of_v<Tag, uri_path_tag,
                                       uri_dir_path_tag, uri_file_tag, uri_file_name_tag, uri_file_ext_tag, uri_file_dot_ext_tag>)
    {
        return escape_append<no_esc_path>(d, s);
    }
    else if constexpr(is_one_of_v<Tag, uri_query_tag, uri_qm_query_tag>)
    {
        return escape_append<no_esc_query>(d, s);
    }
    else if constexpr(is_one_of_v<Tag, uri_frag_tag, uri_sharp_frag_tag>)
    {
        return escape_append<no_esc_frag>(d, s);
    }
    else if constexpr(is_one_of_v<Tag, uri_userinfo_tag>)
    {
        if(auto q = s.find(':'); q != npos)
        {
            d = encode_append<uri_user_tag>(d, s.substr(0, q));
            *d++ = ':';
            return encode_append<uri_password_tag>(d, s.substr(q + 1));
        }
        return encode_append<uri_user_tag>(d, s);
    }
    else if constexpr(is_one_of_v<Tag, uri_userinfo_at_tag>)
    {
        if(s.size())
        {
            BOOST_ASSERT(s.back() == '@');
            s.remove_suffix(1); // '@'
            return encode_append<uri_userinfo_tag>(d, s);
        }
        return d;
    }
    else if constexpr(is_one_of_v<Tag, uri_authority_tag>)
    {
        if(auto q = s.find('@'); q != npos)
        {
            d = encode_append<uri_userinfo_tag>(d, s.substr(0, q));
            *d++ = '@';
            return encode_append<uri_host_port_tag>(d, s.substr(q + 1));
        }
        return encode_append<uri_host_port_tag>(d, s);
    }
    else if constexpr(is_one_of_v<Tag, uri_ss_authority_tag>)
    {
        if(s.size())
        {
            BOOST_ASSERT(s.starts_with("//"));

            *d++ = '/'; *d++ = '/';
            s.remove_prefix(2);
            return encode_append<uri_authority_tag>(d, s);
        }
        return d;
    }
    else if constexpr(is_one_of_v<Tag, uri_scheme_authority_tag>)
    {
        if(auto q = s.find(':'); q != npos)
        {
            d = encode_append<uri_scheme_colon_tag>(d, s.substr(0, q + 1));
            return encode_append<uri_ss_authority_tag>(d, s.substr(q + 1));
        }
        return encode_append<uri_ss_authority_tag>(d, s);
    }
    else if constexpr(is_one_of_v<Tag, uri_qm_query_frag_tag>)
    {
        if(auto q = s.find('#'); q != npos)
        {
            d = encode_append<uri_qm_query_tag>(d, s.substr(0, q));
            return encode_append<uri_sharp_frag_tag>(d, s.substr(q));
        }
        return encode_append<uri_qm_query_tag>(d, s);
    }
    else
    {
        JKL_DEPENDENT_FAIL(Tag, "unsupported part");
    }
}


template<class Tag>
aresult<char*> decode_append(char* d, string_view const& s) noexcept
{
    if constexpr(is_one_of_v<Tag, uri_scheme_tag, uri_scheme_colon_tag, uri_port_tag, uri_auto_port_tag>)
    {
        return append_str(d, s);
    }
    else if constexpr(is_one_of_v<Tag, uri_user_tag, uri_password_tag, uri_userinfo_tag, uri_userinfo_at_tag,
                                       uri_host_tag, uri_host_port_tag, uri_path_tag,
                                       uri_dir_path_tag, uri_file_tag, uri_file_name_tag, uri_file_ext_tag, uri_file_dot_ext_tag,
                                       uri_frag_tag, uri_sharp_frag_tag,
                                       uri_authority_tag, uri_ss_authority_tag, uri_scheme_authority_tag>)
    {
        return unescape_append(d, s);
    }
    else if constexpr(is_one_of_v<Tag, uri_query_tag, uri_qm_query_tag>)
    {
        return unescape_append<true>(d, s);
    }
    else if constexpr(is_one_of_v<Tag, uri_qm_query_frag_tag>)
    {
        if(auto q = s.find('#'); q != npos)
        {
            JKL_TRY(d, decode_append<uri_qm_query_tag>(d, s.substr(0, q)));
            return decode_append<uri_sharp_frag_tag>(d, s.substr(q));
        }
        return decode_append<uri_qm_query_tag>(d, s);
    }
    else
    {
        JKL_DEPENDENT_FAIL(Tag, "unsupported part");
    }
}


} // namespace url_codec


template<class Tag>
constexpr size_t url_max_encode_size(_stringifible_ auto const& t) noexcept
{
    return url_codec::max_encode_size<Tag>(t);
}


template<class Tag>
char* url_encode_append(char* d, string_view const& s) noexcept
{
    return url_codec::encode_append<Tag>(d, s);
}

template<class Tag>
void url_encode_append(_resizable_char_buf_ auto& encoded, string_view const& s)
{
    char* const d = buy_buf(encoded, url_max_encode_size<Tag>(s)); // NOTE: don't replace d with this, the evaluation order is unspecified
    resize_buf(encoded, url_encode_append<Tag>(d, s) - buf_data(encoded));
}

template<class Tag>
void url_encode_assign(_resizable_char_buf_ auto& encoded, string_view const& s)
{
    resize_buf(encoded, url_max_encode_size<Tag>(s));
    char* const beg = buf_data(encoded);
    resize_buf(encoded, url_encode_append<Tag>(beg, s) - beg);
}

template<class Tag, _resizable_char_buf_ B = string>
B url_encode_ret(string_view const& s)
{
    B encoded;
    url_encode_assign<Tag>(encoded, s);
    return encoded;
}


template<class Tag>
aresult<char*> url_decode_append(char* d, string_view const& s) noexcept
{
    return url_codec::decode_append<Tag>(d, s);
}

template<class Tag>
aresult<> url_decode_append(_resizable_char_buf_ auto& decoded, string_view const& s)
{
    JKL_TRY(char* const end, url_decode_append<Tag>(buy_buf(decoded, str_size(s)), s));
    resize_buf(decoded, end - buf_data(decoded));
    return no_err;
}

template<class Tag>
aresult<> url_decode_assign(_resizable_char_buf_ auto& decoded, string_view const& s)
{
    resize_buf(decoded, str_size(s));
    char* const beg = buf_data(decoded);
    //JKL_TRY(char* const end, url_decode_append<Tag>(beg, s));
    auto r = url_decode_append<Tag>(beg, s);
    if(! r)
        return r.error();
    char* end = r.value();
    resize_buf(decoded, end - beg);
    return no_err;
}

template<class Tag, _resizable_char_buf_ B = string>
aresult<B> url_decode_ret(string_view const& s)
{
    B decoded;
    url_decode_assign<Tag>(decoded, s);
    return decoded;
}


} // namespace jkl
