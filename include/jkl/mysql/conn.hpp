#pragma once


#include <jkl/mysql/err.hpp>
#include <jkl/mysql/res.hpp>


namespace jkl{


class mysql_conn
{
    struct deleter
    {
        void operator()(MYSQL* h) const
        {
            mysql_close(h);
        }
    };

    std::unique_ptr<MYSQL, deleter> _h;

public:
    mysql_conn()
        : _h{mysql_init(nullptr)}
    {
        if(! _h)
            throw mysql_err{"mysql_init() failed"};
    }

    MYSQL* raw() { return _h.get(); }
    MYSQL const* raw() const { return _h.get(); }

    // https://dev.mysql.com/doc/refman/en/mysql-errno.html
    unsigned err() { return mysql_errno(raw()); }

    // https://dev.mysql.com/doc/refman/en/mysql-error.html
    char const* err_str() { return mysql_error(raw()); }

    // https://dev.mysql.com/doc/refman/5.7/en/mysql-options.html
    void set_option(mysql_option option, void const* arg)
    {
        _JKL_MYSQL_ERR_HANDLE(mysql_options(raw(), option, arg));
    }

    // https://dev.mysql.com/doc/refman/en/mysql-options4.html
    void set_option(mysql_option option, void const* arg1, void const* arg2)
    {
        _JKL_MYSQL_ERR_HANDLE(mysql_options4(raw(), option, arg1, arg2));
    }

    // https://dev.mysql.com/doc/refman/en/mysql-get-option.html
    void get_option(mysql_option option, void const* arg)
    {
        _JKL_MYSQL_ERR_HANDLE(mysql_get_option(raw(), option, arg));
    }

    void set_server_option(enum_mysql_set_option option)
    {
        _JKL_MYSQL_ERR_HANDLE(mysql_set_server_option(raw(), option));
    }

    // https://dev.mysql.com/doc/refman/en/mysql-real-connect.html
    bool connect(char const* host = nullptr,
                 unsigned port = 0,
                 unsigned long clientflag = 0,
                 char const* db = nullptr,
                 char const* user = nullptr,
                 char const* passwd = nullptr,
                 char const* unix_socket = nullptr)
    {
        return nullptr != mysql_real_connect(raw(), host, user, passwd, db,
                                             port, unix_socket, clientflag);
    }

    // https://dev.mysql.com/doc/refman/en/mysql-reset-connection.html
    bool reset_connect()
    {
        return 0 == mysql_reset_connection(raw());
    }

    // https://dev.mysql.com/doc/refman/en/mysql-change-user.html
    bool change_user(char const* user, char const* passwd, char const* db = nullptr)
    {
        return 0 == mysql_change_user(raw(), user, passwd, db);
    }

    // https://dev.mysql.com/doc/refman/en/mysql-select-db.html
    void select_db(char const* db)
    {
        _JKL_MYSQL_ERR_HANDLE(mysql_select_db(raw(), db));
    }


    // https://dev.mysql.com/doc/refman/en/mysql-real-query.html
    template<class Str>
    void query(Str const& q)
    {
        _JKL_MYSQL_ERR_HANDLE(
            mysql_real_query(raw(), str_data(q), static_cast<unsigned long>(str_size(q))));
    }

    // https://dev.mysql.com/doc/refman/en/mysql-store-result.html
    // fully retrieve result for one statement, be careful with large dataset
    mysql_full_res full_result()
    {
        return mysql_full_res{_JKL_MYSQL_ERR_HANDLE(mysql_store_result(raw()))};
    }

    // https://dev.mysql.com/doc/refman/en/mysql-use-result.html
    // retrieve one row when calling mysql_res::fetch_row(),
    // if your process is slow, it may block the whole table.
    mysql_step_res step_result()
    {
        return mysql_step_res{_JKL_MYSQL_ERR_HANDLE(mysql_use_result(raw()))};
    }

    // https://dev.mysql.com/doc/refman/en/mysql-field-count.html
    // If you want to know whether a statement should return a result set, check this
    unsigned field_count() { return mysql_field_count(raw()); }

    // https://dev.mysql.com/doc/refman/en/mysql-affected-rows.html
    my_ulonglong affected_rows() { return mysql_affected_rows(raw()); }

    // https://dev.mysql.com/doc/refman/en/mysql-next-result.html
    bool next_result()
    {
        auto r = mysql_next_result(raw());

        if(r == 0)
            return true;
        if(r == -1)
            return false;
        if(r > 0)
            throw mysql_err{err_str()};

        throw mysql_err{"mysql_next_result() returned an invalid error code"};
        return false;
    }

    // https://dev.mysql.com/doc/refman/en/mysql-more-results.html
    bool more_results() { return 1 == mysql_more_results(raw()); }


    // https://dev.mysql.com/doc/refman/en/mysql-autocommit.html
    // https://dev.mysql.com/doc/refman/en/innodb-autocommit-commit-rollback.html
    // use mysql_scoped_transaction instead
//     bool autocommit(bool on)
//     {
//         return 0 == mysql_autocommit(raw(), on ? 1 : 0);
//     }

    my_ulonglong insert_id() { return mysql_insert_id(raw()); }

    // https://dev.mysql.com/doc/refman/en/mysql-sqlstate.html
    char const* sqlstate() { return mysql_sqlstate(raw()); }

    // https://dev.mysql.com/doc/refman/en/mysql-warning-count.html
    unsigned warning_count() { return mysql_warning_count(raw()); }

    // https://dev.mysql.com/doc/refman/en/mysql-info.html
    char const* info() { return mysql_info(raw()); }

    // https://dev.mysql.com/doc/refman/en/mysql-thread-id.html
    unsigned long thread_id() { return mysql_thread_id(raw()); }

    // https://dev.mysql.com/doc/refman/en/mysql-character-set-name.html
    char const* character_set_name() { return mysql_character_set_name(raw()); }

    // https://dev.mysql.com/doc/refman/en/mysql-set-character-set.html
    void set_character_set(char const* csname)
    {
        _JKL_MYSQL_ERR_HANDLE(mysql_set_character_set(raw(), csname));
    }


    // https://dev.mysql.com/doc/refman/en/mysql-ssl-set.html
    void ssl_set(char const* key,
                 char const* cert, char const* ca,
                 char const* capath, char const* cipher)
    {
        mysql_ssl_set(raw(), key, cert, ca, capath, cipher);
    }

    // https://dev.mysql.com/doc/refman/en/mysql-get-ssl-cipher.html
    char const* get_ssl_cipher()
    {
        return mysql_get_ssl_cipher(raw());
    }


    void ping()
    {
        _JKL_MYSQL_ERR_HANDLE(mysql_ping(raw()));
    }

    // https://dev.mysql.com/doc/refman/en/mysql-dump-debug-info.html
    void server_dump_debug_info()
    {
        mysql_dump_debug_info(raw());
    }

    char const* stat()
    {
        return mysql_stat(raw());
    }

    char const* get_server_info()
    {
        return mysql_get_server_info(raw());
    }

    char const* get_host_info()
    {
        return mysql_get_host_info(raw());
    }

    unsigned long get_server_version()
    {
        return mysql_get_server_version(raw());
    }

    unsigned get_proto_info()
    {
        return mysql_get_proto_info(raw());
    }

    mysql_full_res list_dbs(char const* wild)
    {
        return mysql_full_res{_JKL_MYSQL_ERR_HANDLE(mysql_list_dbs(raw(), wild))};
    }

    mysql_full_res list_tables(char const* wild)
    {
        return mysql_full_res{_JKL_MYSQL_ERR_HANDLE(mysql_list_tables(raw(), wild))};
    }

    mysql_full_res list_processes()
    {
        return mysql_full_res{_JKL_MYSQL_ERR_HANDLE(mysql_list_processes(raw()))};
    }

    mysql_full_res list_fields(char const* table, char const* wild)
    {
        return mysql_full_res{_JKL_MYSQL_ERR_HANDLE(mysql_list_fields(raw(), table, wild))};
    }


    unsigned long real_escape_string(char* to, char const* from, unsigned long length)
    {
        return mysql_real_escape_string(raw(), to, from, length);
    }

    unsigned long real_escape_string_quote(char* to, char const* from, unsigned long length,
                                           char quote)
    {
        return mysql_real_escape_string_quote(raw(), to, from, length, quote);
    }


//     void remove_escape(char* name)
//     {
//         return myodbc_remove_escape(raw(), name);
//     }



    static unsigned long hex_string(char* to, char const* from, unsigned long from_length)
    {
        return mysql_hex_string(to, from, from_length);
    }

//     static unsigned long escape_string(char* to, char const* from, unsigned long from_length)
//     {
//         return mysql_escape_string(to, from, from_length);
//     }


    static void debug(char const* debug)
    {
        return mysql_debug(debug);
    }

    static bool thread_safe()
    {
        return 1 == mysql_thread_safe();
    }

    static bool embedded()
    {
        return 1 == mysql_embedded();
    }

    static char const* client_info()
    {
        return mysql_get_client_info();
    }

    static unsigned long client_version()
    {
        return mysql_get_client_version();
    }

#ifdef LIBMARIADB
    void cancel()
    {
        // https://mariadb.com/kb/en/library/mariadb_cancel/
        _JKL_MYSQL_ERR_HANDLE(mariadb_cancel(raw()));
    }

    void reconnect()
    {
        // https://mariadb.com/kb/en/library/mariadb_reconnect/
        _JKL_MYSQL_ERR_HANDLE(mariadb_reconnect(raw()));
    }
#endif
};



// https://dev.mysql.com/doc/refman/en/innodb-autocommit-commit-rollback.html
class mysql_scoped_transaction
{
    mysql_conn& _c;
    bool _commited = false;

public:
    mysql_scoped_transaction(mysql_conn& c)
        : _c{c}
    {
        _c.query("BEGIN");
    }

    void commit()
    {
        BOOST_ASSERT(! _commited);
        _c.query("COMMIT");
        _commited = true;
    }

    ~mysql_scoped_transaction()
        noexcept(noexcept(std::declval<mysql_conn>().query("ROLLBACK")))
    {
        if(! _commited)
            _c.query("ROLLBACK");
    }
};


} // namespace jkl