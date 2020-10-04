#pragma once


#include <jkl/mysql/err.hpp>


namespace jkl{


template<class Fld>
struct mysql_nullable
{
    Fld fld;
    my_bool is_null = 0;

    void set_null() { is_null = 1; }
};


template<class Buf>
struct mysql_blob_ref
{
    Buf& buf;
};

template<class Buf>
mysql_blob_ref<Buf> mysql_as_blob(Buf& buf)
{
    static_assert(is_buf_v<buf>);
    return {buf};
}


template<class Str>
struct mysql_str_ref
{
    Str& str;
};

template<class Str>
mysql_str_ref<Str> mysql_as_str(Str& s)
{
    static_assert(is_buf_v<s>);
    return {s};
}


} // namespace jkl

