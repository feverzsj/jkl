#pragma once


#include <jkl/mysql/res.hpp>
#include <jkl/mysql/conn.hpp>
#include <jkl/mysql/stmt/bind.hpp>
#include <jkl/mysql/stmt/param.hpp>
#include <jkl/mysql/stmt/result.hpp>


namespace jkl{


// https://dev.mysql.com/doc/refman/en/c-api-prepared-statements.html
class mysql_stmt
{
    struct deleter
    {
        void operator()(MYSQL_STMT* h) const
        {
            if(0 != mysql_stmt_close(h))
                throw mysql_err("mysql_stmt_close() failed");
        }
    };

    std::unique_ptr<MYSQL_STMT, deleter> _h;


public:
    explicit mysql_stmt(mysql_conn& c)
        : _h{mysql_stmt_init(c.raw())}
    {
        if(! _h)
            throw mysql_err("mysql_stmt_init() failed");
    }


    MYSQL_STMT* raw() { return _h.get(); }
    MYSQL_STMT const* raw() const { return _h.get(); }

    unsigned err() { return mysql_stmt_errno(raw()); }
    char const* err_str() { return mysql_stmt_error(raw()); }


    // https://dev.mysql.com/doc/refman/en/mysql-stmt-attr-set.html
    void attr_set(enum_stmt_attr_type attr_type, void const* attr)
    {
        _JKL_MYSQL_ERR_HANDLE(mysql_stmt_attr_set(raw(), attr_type, attr));
    }

    // https://dev.mysql.com/doc/refman/en/mysql-stmt-attr-get.html
    void attr_get(enum_stmt_attr_type attr_type, void* attr)
    {
        _JKL_MYSQL_ERR_HANDLE(mysql_stmt_attr_get(raw(), attr_type, attr));
    }

    // default 1
    void set_prefetch_rows(unsigned long n)
    {
        attr_set(STMT_ATTR_PREFETCH_ROWS, &n);
    }

    unsigned long prefetch_rows()
    {
        unsigned long n;
        attr_get(STMT_ATTR_PREFETCH_ROWS, &n);
        return n;
    }


    // https://dev.mysql.com/doc/refman/en/mysql-stmt-prepare.html
    template<class Str>
    void prepare(Str const& query)
    {
        _JKL_MYSQL_ERR_HANDLE(
            mysql_stmt_prepare(raw(), str_data(query), static_cast<unsigned long>(str_size(query))));
    }

    unsigned long param_count()
    {
        return mysql_stmt_param_count(raw());
    }

    unsigned field_count() // for result
    {
        return mysql_stmt_field_count(raw());
    }

    // always return null
    mysql_full_res param_metadata()
    {
        return mysql_full_res{_JKL_MYSQL_ERR_HANDLE(mysql_stmt_param_metadata(raw()))};
    }

    mysql_full_res result_metadata()
    {
        return mysql_full_res{_JKL_MYSQL_ERR_HANDLE(mysql_stmt_result_metadata(raw()))};
    }

    // https://dev.mysql.com/doc/refman/en/mysql-stmt-send-long-data.html
    template<class Buf>
    void send_long_param(unsigned param_number, Buf const& data)
    {
        _JKL_MYSQL_ERR_HANDLE(
            mysql_stmt_send_long_data(raw(), param_number, buf_data(data), buf_size(data)));
    }

    // https://dev.mysql.com/doc/refman/en/mysql-stmt-reset.html
    void reset()
    {
        _JKL_MYSQL_ERR_HANDLE(mysql_stmt_reset(raw()));
    }

    // https://dev.mysql.com/doc/refman/en/mysql-stmt-execute.html
    template<class...P>
    void exec(P const&...p)
    {
        BOOST_ASSERT(sizeof...(p) == param_count());

        auto params = mysql_bind_params(p...);

        _JKL_MYSQL_ERR_HANDLE(mysql_stmt_bind_param(raw(), params.binds));
        _JKL_MYSQL_ERR_HANDLE(mysql_stmt_execute(raw()));
    }

    // https://dev.mysql.com/doc/refman/en/mysql-stmt-affected-rows.html
    my_ulonglong affected_rows()
    {
        return mysql_stmt_affected_rows(raw());
    }

    // https://dev.mysql.com/doc/refman/en/mysql-stmt-insert-id.html
    my_ulonglong insert_id()
    {
        return mysql_stmt_insert_id(raw());
    }


    // https://dev.mysql.com/doc/refman/en/mysql-stmt-fetch.html
    // NOTE: each fetch only return one row from server, so don't put long time process in fetch loop,
    //       instead, fetch them all, then process.
    template<class...T>
    bool fetch(T&...t)
    {
        if(sizeof...(t) != field_count())
            throw mysql_err{"result field count is different from expectation"};

        auto results = mysql_bind_results(t...);
        _JKL_MYSQL_ERR_HANDLE(mysql_stmt_bind_result(raw(), results.binds));

        switch(mysql_stmt_fetch(raw()))
        {
            case 0                   : results.process_results(raw(), t...); return true;
            case 1                   : throw mysql_err{err_str()};
            case MYSQL_NO_DATA       : return false;
            case MYSQL_DATA_TRUNCATED: throw mysql_err{"MYSQL_DATA_TRUNCATED"};
        }

        throw mysql_err{"mysql_stmt_fetch(): unknown return code"};
    }


    // NOTE: follows are only usable after mysql_stmt_store_result()

    // https://dev.mysql.com/doc/refman/en/mysql-stmt-row-seek.html
//     MYSQL_ROW_OFFSET row_seek(MYSQL_ROW_OFFSET offset)
//     {
//         return mysql_stmt_row_seek(raw(), offset);
//     }
// 
//     MYSQL_ROW_OFFSET row_tell()
//     {
//         return mysql_stmt_row_tell(raw());
//     }
// 
//     void data_seek(my_ulonglong offset)
//     {
//         mysql_stmt_data_seek(raw(), offset);
//     }
// 
//     my_ulonglong num_rows()
//     {
//         return mysql_stmt_num_rows(raw());
//     }



    // https://dev.mysql.com/doc/refman/en/mysql-stmt-free-result.html
    void free_result()
    {
        _JKL_MYSQL_ERR_HANDLE(mysql_stmt_free_result(raw()));
    }

    // https://dev.mysql.com/doc/refman/en/mysql-stmt-next-result.html
    bool next_result()
    {
        auto r = mysql_stmt_next_result(raw());

        if(r ==  0)
            return true;
        if(r == -1)
            return false;
        if(r > 0)
            throw mysql_err{err_str()};

        throw mysql_err{"mysql_stmt_next_result() returned an invalid error code"};
        return false;
    }


    char const* sqlstate()
    {
        return mysql_stmt_sqlstate(raw());
    }
};


} // namespace jkl