#pragma once


#include <jkl/mysql/stmt/bind.hpp>


namespace jkl{


// variable length data types, typically string and blob
inline void set_mysql_varlen_result_bind(MYSQL_BIND& bind, enum_field_types type)
{
    bind.buffer_type   = type;
    bind.buffer        = nullptr;
    bind.buffer_length = 0;
    bind.length        = &bind.length_value;
}

template<class MysqlStmt, class T>
void process_mysql_varlen_result(MysqlStmt& stmt, size_t i, T& t)
{
    MYSQL_BIND& bind = stmt.resultBinds[i];

    rescale_buf(t, bind.length_value);
    bind.buffer        = buf_data(t);
    bind.buffer_length = static_cast<unsigned long>(buf_size(t));
    mysql_stmt_fetch_column(stmt.raw(), &bind, static_cast<unsigned long>(i), 0);
}



template<class T>
void set_mysql_result_bind(MYSQL_BIND& bind, T& t)
{
    set_mysql_bind(bind, t);
}

template<class T>
void set_mysql_result_bind(MYSQL_BIND& bind, mysql_nullable<T>& t)
{
    set_mysql_result_bind(bind, t.fld);
    bind.is_null = &t.is_null;
}

template<class Str>
void set_mysql_result_bind(MYSQL_BIND& bind, mysql_str_ref<Str> t)
{
    set_mysql_varlen_result_bind(bind, MYSQL_TYPE_STRING); // TEXT, CHAR, VARCHAR
}

template<class Buf>
void set_mysql_result_bind(MYSQL_BIND& bind, mysql_blob_ref<Buf> t)
{
    set_mysql_varlen_result_bind(bind, MYSQL_TYPE_BLOB); // BLOB, BINARY, VARBINARY
}

template<class T, class A>
void set_mysql_result_bind(MYSQL_BIND& bind, std::basic_string<char, T, A>& t)
{
    set_mysql_varlen_result_bind(bind, MYSQL_TYPE_STRING);
}


template<class Buf, class... Flds>
void set_mysql_result_binds(Buf& binds, Flds&... results)
{
    static_assert(sizeof...(results) > 0);
    BOOST_ASSERT(sizeof...(results) == buf_size(binds));

    MYSQL_BIND* b = buf_data(binds);

    memset(b, 0, buf_byte_size(binds));

    (... , set_mysql_result_bind(*(b++), results));
}



template<class MysqlStmt, class T>
void process_mysql_result(MysqlStmt&, size_t, T&)
{} // no need for further process

template<class MysqlStmt, class T>
void process_mysql_result(MysqlStmt& stmt, size_t i, mysql_nullable<T>& t)
{
    if(! t.is_null)
        process_mysql_result(stmt, i, t.fld);
}

template<class MysqlStmt, class Str>
void process_mysql_result(MysqlStmt& stmt, size_t i, mysql_str_ref<Str> t)
{
    process_mysql_varlen_result(stmt, i, t.buf);
}

template<class MysqlStmt, class Buf>
void process_mysql_result(MysqlStmt& stmt, size_t i, mysql_blob_ref<Buf> t)
{
    process_mysql_varlen_result(stmt, i, t.buf);
}

template<class MysqlStmt, class T, class A>
void process_mysql_result(MysqlStmt& stmt, size_t i, std::basic_string<char, T, A>& t)
{
    process_mysql_varlen_result(stmt, i, t);
}



template<class MysqlStmt, class...Flds>
void process_mysql_results(MysqlStmt& stmt, Flds&...flds)
{
    static_assert(sizeof...(flds) > 0);
    BOOST_ASSERT(sizeof...(flds) == stmt.resultBinds.size());

    int i = 0;
    (... , process_mysql_result(stmt, i++, flds));
}


} // namespace jkl

