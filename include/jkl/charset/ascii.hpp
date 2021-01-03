#pragma once

#include <jkl/config.hpp>
#include <jkl/util/str.hpp>


namespace jkl{


inline constexpr unsigned char g_lut_ascii_classification[256] = {
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,  // 0x00
    0x40, 0x68, 0x48, 0x48, 0x48, 0x48, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,  // 0x10
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x28, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,  // 0x20
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84,  // 0x30
    0x84, 0x84, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x05,  // 0x40
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,  // 0x50
    0x05, 0x05, 0x05, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x05,  // 0x60
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,  // 0x70
    0x05, 0x05, 0x05, 0x10, 0x10, 0x10, 0x10, 0x40,
};

inline constexpr char g_lut_tolower[256] = {
    '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',
    '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e', '\x0f',
    '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
    '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f',
    '\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27',
    '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f',
    '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37',
    '\x38', '\x39', '\x3a', '\x3b', '\x3c', '\x3d', '\x3e', '\x3f',
    '\x40',    'a',    'b',    'c',    'd',    'e',    'f',    'g',
    'h',    'i',    'j',    'k',    'l',    'm',    'n',    'o',
    'p',    'q',    'r',    's',    't',    'u',    'v',    'w',
    'x',    'y',    'z', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f',
    '\x60', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67',
    '\x68', '\x69', '\x6a', '\x6b', '\x6c', '\x6d', '\x6e', '\x6f',
    '\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77',
    '\x78', '\x79', '\x7a', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f',
    '\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87',
    '\x88', '\x89', '\x8a', '\x8b', '\x8c', '\x8d', '\x8e', '\x8f',
    '\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97',
    '\x98', '\x99', '\x9a', '\x9b', '\x9c', '\x9d', '\x9e', '\x9f',
    '\xa0', '\xa1', '\xa2', '\xa3', '\xa4', '\xa5', '\xa6', '\xa7',
    '\xa8', '\xa9', '\xaa', '\xab', '\xac', '\xad', '\xae', '\xaf',
    '\xb0', '\xb1', '\xb2', '\xb3', '\xb4', '\xb5', '\xb6', '\xb7',
    '\xb8', '\xb9', '\xba', '\xbb', '\xbc', '\xbd', '\xbe', '\xbf',
    '\xc0', '\xc1', '\xc2', '\xc3', '\xc4', '\xc5', '\xc6', '\xc7',
    '\xc8', '\xc9', '\xca', '\xcb', '\xcc', '\xcd', '\xce', '\xcf',
    '\xd0', '\xd1', '\xd2', '\xd3', '\xd4', '\xd5', '\xd6', '\xd7',
    '\xd8', '\xd9', '\xda', '\xdb', '\xdc', '\xdd', '\xde', '\xdf',
    '\xe0', '\xe1', '\xe2', '\xe3', '\xe4', '\xe5', '\xe6', '\xe7',
    '\xe8', '\xe9', '\xea', '\xeb', '\xec', '\xed', '\xee', '\xef',
    '\xf0', '\xf1', '\xf2', '\xf3', '\xf4', '\xf5', '\xf6', '\xf7',
    '\xf8', '\xf9', '\xfa', '\xfb', '\xfc', '\xfd', '\xfe', '\xff',
};

inline constexpr char g_lut_toupper[256] = {
    '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',
    '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e', '\x0f',
    '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
    '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f',
    '\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27',
    '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f',
    '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37',
    '\x38', '\x39', '\x3a', '\x3b', '\x3c', '\x3d', '\x3e', '\x3f',
    '\x40', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47',
    '\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d', '\x4e', '\x4f',
    '\x50', '\x51', '\x52', '\x53', '\x54', '\x55', '\x56', '\x57',
    '\x58', '\x59', '\x5a', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f',
    '\x60',    'A',    'B',    'C',    'D',    'E',    'F',    'G',
    'H',    'I',    'J',    'K',    'L',    'M',    'N',    'O',
    'P',    'Q',    'R',    'S',    'T',    'U',    'V',    'W',
    'X',    'Y',    'Z', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f',
    '\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87',
    '\x88', '\x89', '\x8a', '\x8b', '\x8c', '\x8d', '\x8e', '\x8f',
    '\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97',
    '\x98', '\x99', '\x9a', '\x9b', '\x9c', '\x9d', '\x9e', '\x9f',
    '\xa0', '\xa1', '\xa2', '\xa3', '\xa4', '\xa5', '\xa6', '\xa7',
    '\xa8', '\xa9', '\xaa', '\xab', '\xac', '\xad', '\xae', '\xaf',
    '\xb0', '\xb1', '\xb2', '\xb3', '\xb4', '\xb5', '\xb6', '\xb7',
    '\xb8', '\xb9', '\xba', '\xbb', '\xbc', '\xbd', '\xbe', '\xbf',
    '\xc0', '\xc1', '\xc2', '\xc3', '\xc4', '\xc5', '\xc6', '\xc7',
    '\xc8', '\xc9', '\xca', '\xcb', '\xcc', '\xcd', '\xce', '\xcf',
    '\xd0', '\xd1', '\xd2', '\xd3', '\xd4', '\xd5', '\xd6', '\xd7',
    '\xd8', '\xd9', '\xda', '\xdb', '\xdc', '\xdd', '\xde', '\xdf',
    '\xe0', '\xe1', '\xe2', '\xe3', '\xe4', '\xe5', '\xe6', '\xe7',
    '\xe8', '\xe9', '\xea', '\xeb', '\xec', '\xed', '\xee', '\xef',
    '\xf0', '\xf1', '\xf2', '\xf3', '\xf4', '\xf5', '\xf6', '\xf7',
    '\xf8', '\xf9', '\xfa', '\xfb', '\xfc', '\xfd', '\xfe', '\xff',
};



// [A-Za-z]
constexpr bool ascii_isalpha(char c) noexcept
{
    return (g_lut_ascii_classification[static_cast<unsigned char>(c)] & 0x01) != 0;
    // return (c|32)-'a' < 26u; // works for both signed and unsigned char
}

// [A-Za-z0-9]
constexpr bool ascii_isalnum(char c) noexcept
{
    return (g_lut_ascii_classification[static_cast<unsigned char>(c)] & 0x04) != 0;
}

// [ \t\r\n\v\f]
constexpr bool ascii_isspace(char c) noexcept
{
    return (g_lut_ascii_classification[static_cast<unsigned char>(c)] & 0x08) != 0;
}

// [!"#$%&'()*+,./:;<=>?@\^_`{|}~-]
constexpr bool ascii_ispunct(char c) noexcept
{
    return (g_lut_ascii_classification[static_cast<unsigned char>(c)] & 0x10) != 0;
}

// [ \t]
constexpr bool ascii_isblank(char c) noexcept
{
    return (g_lut_ascii_classification[static_cast<unsigned char>(c)] & 0x20) != 0;
}

// [\x00-\x1F\x7F]
constexpr bool ascii_iscntrl(char c) noexcept
{
    return (g_lut_ascii_classification[static_cast<unsigned char>(c)] & 0x40) != 0;
}

// [A-Fa-f0-9]
constexpr bool ascii_isxdigit(char c) noexcept
{
    return (g_lut_ascii_classification[static_cast<unsigned char>(c)] & 0x80) != 0;
}

// [0-9]
constexpr bool ascii_isdigit(char c) noexcept
{
    auto uc = static_cast<unsigned char>(c);
    return  '0' <= uc && uc <= '9';
}

// [\x20-\x7E]
constexpr bool ascii_isprint(char c) noexcept
{
    auto uc = static_cast<unsigned char>(c);
    return 0x20 <= uc && uc <= 0x7E;
}

// [\x21-\x7E]
constexpr bool ascii_isgraph(char c) noexcept
{
    auto uc = static_cast<unsigned char>(c);
    return 0x21 <= uc && uc <= 0x7E;
}

// [A-Z]
constexpr bool ascii_isupper(char c)
{
    auto uc = static_cast<unsigned char>(c);
    return  'A' <= uc && uc <= 'Z';
}

// [a-z]
constexpr bool ascii_islower(char c) noexcept
{
    auto uc = static_cast<unsigned char>(c);
    return  'a' <= uc && uc <= 'z';
}

constexpr bool ascii_isascii(char c) noexcept
{
    return static_cast<unsigned char>(c) < 128;
}


constexpr char ascii_tolower(char c) noexcept
{
    return g_lut_tolower[static_cast<unsigned char>(c)];
}

constexpr char ascii_toupper(char c) noexcept
{
    return g_lut_toupper[static_cast<unsigned char>(c)];
}


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr char* ascii_tolower_append(char* d, _char_str_ auto const& s) noexcept
{
    for(char c : as_str_class(s))
        *d++ = ascii_tolower(c);
    return d;
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr char* ascii_toupper_append(char* d, _char_str_ auto const& s) noexcept
{
    for(char c : as_str_class(s))
        *d++ = ascii_toupper(c);
    return d;
}


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr void ascii_tolower_append(_resizable_char_buf_ auto& d, _char_str_ auto const& s) noexcept
{
    decltype(auto) sc = as_str_class(s);
    ascii_tolower_append(buy_buf(d, str_size(sc)), sc);
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr void ascii_toupper_append(_resizable_char_buf_ auto& d, _char_str_ auto const& s) noexcept
{
    decltype(auto) sc = as_str_class(s);
    ascii_toupper_append(buy_buf(d, str_size(sc)), sc);
}


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr void ascii_tolower_assign(_resizable_char_buf_ auto& d, _char_str_ auto const& s) noexcept
{
    decltype(auto) sc = as_str_class(s);
    resize_buf(d, str_size(sc));
    ascii_tolower_append(buf_data(d), sc);
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr void ascii_toupper_assign(_resizable_char_buf_ auto& d, _char_str_ auto const& s) noexcept
{
    decltype(auto) sc = as_str_class(s);
    resize_buf(d, str_size(sc));
    ascii_toupper_append(buf_data(d), sc);
}

constexpr void ascii_tolower_inplace(char& c) noexcept { c = ascii_tolower(c); }
constexpr void ascii_toupper_inplace(char& c) noexcept { c = ascii_toupper(c); }

constexpr char* ascii_tolower_inplace(char* beg, char* end) noexcept
{
    for(; beg < end; ++beg)
        ascii_tolower_inplace(*beg);
    return end;
}

constexpr char* ascii_toupper_inplace(char* beg, char* end) noexcept
{
    for(; beg < end; ++beg)
        ascii_toupper_inplace(*beg);
    return end;
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr void ascii_tolower_inplace(_char_str_ auto& s) noexcept { ascii_tolower_inplace(str_begin(s), str_end(s)); }
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr void ascii_toupper_inplace(_char_str_ auto& s) noexcept { ascii_toupper_inplace(str_begin(s), str_end(s)); }


constexpr char const* ascii_trim_left(char const* beg, char const* const end) noexcept
{
    while(beg < end && ascii_isspace(*beg))
        ++beg;
    return beg;
}

constexpr char const* ascii_trim_right(char const* const beg, char const* end) noexcept
{
    while(beg < end && ascii_isspace(*(end - 1)))
        --end;
    return end;
}

constexpr void ascii_trim_inplace(char const*& beg, char const*& end) noexcept
{
    beg = ascii_trim_left(beg, end);
    end = ascii_trim_right(beg, end);
}


constexpr string_view ascii_trimed_view(char const* beg, char const* end) noexcept
{
    ascii_trim_inplace(beg, end);
    return {beg, end};
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr string_view ascii_trimed_view(_char_str_ auto const& s) noexcept
{
    return ascii_trimed_view(str_begin(s), str_end(s));
}


// case insensitive compare
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr bool ascii_iequal(_char_str_ auto const& l, _char_str_ auto const& r) noexcept
{
    auto&& cl = as_str_class(l);
    auto&& cr = as_str_class(r);

    if(cl.size() != cr.size())
        return false;

    for(size_t i = 0; i < cl.size(); ++ i)
    {
        if(ascii_tolower(cl[i]) != ascii_tolower(cr[i]))
            return false;
    }

    return true;
}

} // namespace jkl
