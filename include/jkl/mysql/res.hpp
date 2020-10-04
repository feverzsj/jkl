#pragma once


#include <jkl/mysql/err.hpp>


namespace jkl{


// WIP: for now, it only gives textual representation
//      of data(binary data types are still as-is) as null terminated string,
//      so use mysql_stmt is preferred

class mysql_step_res
{
    struct deleter
    {
        void operator()(MYSQL_RES* h) const
        {
            mysql_free_result(h);
        }
    };

    std::unique_ptr<MYSQL_RES, deleter> _h;

    friend class mysql_conn;
    friend class mysql_stmt;
    friend class mysql_full_res;
    explicit mysql_step_res(MYSQL_RES* h) : _h{h} {}

public:
    MYSQL_RES* raw() { return _h.get(); }
    MYSQL_RES const* raw() const { return _h.get(); }

    operator bool() const { return _h.operator bool(); }

    // https://dev.mysql.com/doc/refman/en/mysql-fetch-row.html
    MYSQL_ROW fetch_row()
    {
        return mysql_fetch_row(raw());
    }

    unsigned long* fetch_lengths()
    {
        return mysql_fetch_lengths(raw());
    }

    MYSQL_FIELD* fetch_next_field()
    {
        return mysql_fetch_field(raw());
    }
};


class mysql_full_res : public mysql_step_res
{
    friend class mysql_conn;
    friend class mysql_stmt;
    using mysql_step_res::mysql_step_res;

public:
    void data_seek(my_ulonglong offset)
    {
        return mysql_data_seek(raw(), offset);
    }

    MYSQL_ROW_OFFSET row_seek(MYSQL_ROW_OFFSET offset)
    {
        return mysql_row_seek(raw(), offset);
    }

    MYSQL_FIELD_OFFSET field_seek(MYSQL_FIELD_OFFSET offset)
    {
        return mysql_field_seek(raw(), offset);
    }

    MYSQL_ROW_OFFSET row_tell()
    {
        return mysql_row_tell(raw());
    }

    MYSQL_FIELD_OFFSET field_tell()
    {
        return mysql_field_tell(raw());
    }
};


} // namespace jkl