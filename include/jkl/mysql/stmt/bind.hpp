#pragma once


#include <jkl/mysql/time.hpp>
#include <jkl/mysql/data_types.hpp>

#include <type_traits>


namespace jkl{


// https://dev.mysql.com/doc/refman/en/c-api-prepared-statement-data-structures.html
// https://dev.mysql.com/doc/refman/en/c-api-prepared-statement-type-codes.html


template<bool IsResult, class...P>
struct mysql_binds
{
    static constexpr int count = sizeof...(P);
    MYSQL_BIND binds[count ? count : 1] = {};

    explicit mysql_binds(P&...p)
        : mysql_binds(std::make_index_sequence<count>{}, p...)
    {}

    template<size_t... I>
    explicit mysql_binds(std::index_sequence<I...>, P&...p)
    {
        [[maybe_unused]] auto l = { (do_bind(binds[I], p), 0)... };
    }

    template<class T>
    static void do_bind(MYSQL_BIND& bind, T& t)
    {
        // NOTE: follows are implementation detail of the mysql c connector, but should be valid in most cases 
        //      if any bind.is_null/length/error is null,
        //      it will be set to bind.is_null_value/length_value/error_value

        if constexpr(is_optional_v<T>)
        {
            if constexpr(IsResult)
            {
                if(! t)
                    t.emplace();
                do_bind(bind, *t);
            }
            else
            {
                if(t)
                    do_bind(bind, *t);
                else
                    bind.is_null_value = true;
            }
        }

        
        else if constexpr(std::is_null_pointer_v<T>)
        {
            bind.buffer_type = MYSQL_TYPE_NULL; // field always NULL
        }


        else if constexpr(std::is_integral_v<T>)
        {
            if constexpr     (sizeof(T) == sizeof(char))
                bind.buffer_type = MYSQL_TYPE_TINY;     // TINYINT
            else if constexpr(sizeof(T) == sizeof(short))
                bind.buffer_type = MYSQL_TYPE_SHORT;    // SMALLINT
            else if constexpr(sizeof(T) == sizeof(int))
                bind.buffer_type = MYSQL_TYPE_LONG;     // INT
            else if constexpr(sizeof(T) == sizeof(long long))
                bind.buffer_type = MYSQL_TYPE_LONGLONG; // BIGINT
            else
                JKL_DEPENDENT_FAIL(T, "unsupported type");

            bind.buffer      = &t;
            bind.is_unsigned = std::is_unsigned_v<T>;
        }


        else if constexpr(std::is_same_v<T, float>)
        {
            bind.buffer_type = MYSQL_TYPE_FLOAT; // FLOAT
            bind.buffer      = &t;
        }
        else if constexpr(std::is_same_v<T, double>)
        {
            bind.buffer_type = MYSQL_TYPE_DOUBLE; // DOUBLE
            bind.buffer      = &t;
        }


        else if constexpr(std::is_same_v<T, mysql_date>)
        {
            bind.buffer_type = MYSQL_TYPE_DATE; // DATE
            bind.buffer      = static_cast<MYSQL_TIME*>(&t);
        }
        else if constexpr(std::is_same_v<T, mysql_time>)
        {
            bind.buffer_type = MYSQL_TYPE_TIME; // TIME
            bind.buffer      = static_cast<MYSQL_TIME*>(&t);
        }
        else if constexpr(std::is_same_v<T, mysql_datetime>)
        {
            bind.buffer_type = MYSQL_TYPE_DATETIME; // DATETIME
            bind.buffer      = static_cast<MYSQL_TIME*>(&t);
        }
        else if constexpr(std::is_same_v<T, mysql_timestamp>)
        {
            bind.buffer_type = MYSQL_TYPE_TIMESTAMP; // TIMESTAMP
            bind.buffer      = static_cast<MYSQL_TIME*>(&t);
        }

        else if constexpr(is_text_v<T>)
        {
            bind.buffer_type = MYSQL_TYPE_STRING; // TEXT, CHAR, VARCHAR

            if constexpr(IsResult)
            {
                //bind.buffer        = nullptr;
                //bind.buffer_length = 0;
                //bind.length        = &bind.length_value;
            }
            else
            {
                bind.buffer        = str_data(t);
                bind.buffer_length = static_cast<unsigned long>(str_size(t));
            }
        }
        else if constexpr(is_blob_v<T>)
        {
            bind.buffer_type = MYSQL_TYPE_BLOB; // BLOB, BINARY, VARBINARY

            if constexpr(IsResult)
            {
                //bind.buffer        = nullptr;
                //bind.buffer_length = 0;
                //bind.length        = &bind.length_value;
            }
            else
            {
                bind.buffer        = buf_data(t);
                bind.buffer_length = static_cast<unsigned long>(buf_size(t));
            }
        }

    }

    void process_results(MYSQL_STMT* stmt, P&...p)
    {
        process_results(std::make_index_sequence<count>{}, stmt, p...)
    }

    template<size_t... I>
    void process_results(std::index_sequence<I...>, MYSQL_STMT* stmt, P&...p)
    {
        [[maybe_unused]] auto l = {(do_process_result<I>(stmt, p), 0)...};
    }

    template<unsigned I, class T>
    void do_process_result(MYSQL_STMT* stmt, T& t)
    {
        if constexpr(! IsResult)
            JKL_DEPENDENT_FAIL(T, "this method is for result");

        MYSQL_BIND& bind = binds[I];

        if constexpr(is_optional_v<T>)
        {
            if(* bind.is_null)
            {
                t.reset();
            }
            else
            {
                if(! t)
                    t.emplace();
                do_process_result(stmt, bind, *t);
            }
        }
        else if constexpr(is_buf_v<T>)
        {
            rescale_buf(t, bind.length_value);
            bind.buffer        = buf_data(t);
            bind.buffer_length = static_cast<unsigned long>(buf_size(t));
            mysql_stmt_fetch_column(stmt, &bind, I, 0);
        }
    }
};


template<class...P>
auto mysql_bind_params(P const&...p)
{
    return mysql_binds<false, P...>{const_cast<P&>(p)...};
}

template<class...P>
auto mysql_bind_results(P&...p)
{
    return mysql_binds<true, P...>{p...};
}


} // namespace jkl