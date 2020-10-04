#pragma once


#include <jkl/mysql/stmt/bind.hpp>


namespace jkl{


template<class T>
void set_mysql_param_bind(MYSQL_BIND& bind, T& t)
{
    set_mysql_bind(bind, t);
}

template<class T>
void set_mysql_param_bind(MYSQL_BIND& bind, mysql_nullable<T>& t)
{
    set_mysql_param_bind(bind, t.fld);
    bind.is_null = &t.is_null;
}

template<class Str>
void set_mysql_param_bind(MYSQL_BIND& bind, mysql_str_ref<Str> t)
{
    bind.buffer_type   = MYSQL_TYPE_STRING; // TEXT, CHAR, VARCHAR
    bind.buffer        = buf_data(t.str);
    bind.buffer_length = buf_data(t.str);
}

template<class Buf>
void set_mysql_param_bind(MYSQL_BIND& bind, mysql_blob_ref<Buf> t)
{
    bind.buffer_type   = MYSQL_TYPE_BLOB; // BLOB, BINARY, VARBINARY
    bind.buffer        = buf_data(t.buf);
    bind.buffer_length = buf_size(t.buf);
}

inline void set_mysql_param_bind(MYSQL_BIND& bind, std::nullptr_t)
{
    bind.buffer_type = MYSQL_TYPE_NULL; // field always NULL
}

inline void set_mysql_param_bind(MYSQL_BIND& bind, char* t)
{
    bind.buffer_type   = MYSQL_TYPE_STRING; // TEXT, CHAR, VARCHAR
    bind.buffer        = t;
    bind.buffer_length = static_cast<unsigned long>(strlen(t));
}

template<class T>
void set_mysql_param_bind(MYSQL_BIND& bind, boost::basic_string_ref<char, T> const& t)
{
    bind.buffer_type   = MYSQL_TYPE_STRING; // TEXT, CHAR, VARCHAR
    bind.buffer        = t.data();
    bind.buffer_length = t.size();
}

template<class T>
void set_mysql_param_bind(MYSQL_BIND& bind, boost::basic_string_view<char, T> const& t)
{
    bind.buffer_type   = MYSQL_TYPE_STRING; // TEXT, CHAR, VARCHAR
    bind.buffer        = t.data();
    bind.buffer_length = t.size();
}

template<class T>
void set_mysql_param_bind(MYSQL_BIND& bind, std::basic_string_view<char, T> const& t)
{
    bind.buffer_type   = MYSQL_TYPE_STRING; // TEXT, CHAR, VARCHAR
    bind.buffer        = t.data();
    bind.buffer_length = t.size();
}

template<class T, class A>
void set_mysql_param_bind(MYSQL_BIND& bind, std::basic_string<char, T, A>& t)
{
    // need rebind when t being modified, since t.data() may reallocate new buffer.
    bind.buffer_type   = MYSQL_TYPE_STRING; // TEXT, CHAR, VARCHAR
    bind.buffer        = t.data();
    bind.buffer_length = t.size();
}


// binds: MYSQL_BIND buffer
template<class Buf, class...Flds>
void set_mysql_param_binds(Buf& binds, Flds&...params)
{
    static_assert(sizeof...(params) > 0);
    BOOST_ASSERT(sizeof...(params) == buf_size(binds));

    MYSQL_BIND* b = buf_data(binds);

    memset(b, 0, buf_byte_size(binds));

    (... , set_mysql_param_bind(*(b++), params));
}


} // namespace jkl