#pragma once


#include <mysql/mysql_time.h>


namespace jkl{


// https://dev.mysql.com/doc/refman/en/datetime.html
// https://dev.mysql.com/doc/refman/en/c-api-prepared-statement-data-structures.html


// '1000-01-01' to '9999-12-31'
struct mysql_date : public MYSQL_TIME
{
    mysql_date(unsigned year, unsigned month, unsigned day)
    {
        this->year  = year;
        this->month = month;
        this->day   = day;
        this->time_type = MYSQL_TIMESTAMP_DATE;
    }
};

// '00:00:00' to '23:59:59'
struct mysql_time : public MYSQL_TIME
{
    mysql_time(unsigned hour, unsigned minute, unsigned second,
               unsigned long second_part = 0, bool neg = false)
    {
        this->day         = 0;
        this->hour        = hour;
        this->minute      = minute;
        this->second      = second;
        this->second_part = second_part;
        this->neg         = neg ? 1 : 0;
        this->time_type   = MYSQL_TIMESTAMP_TIME;

    }
};

// '1000-01-01 00:00:00' to '9999-12-31 23:59:59'
struct mysql_datetime : public MYSQL_TIME
{
    mysql_datetime(unsigned year, unsigned month, unsigned day,
                   unsigned hour, unsigned minute, unsigned second, unsigned long second_part = 0)
    {
        this->year        = year;
        this->month       = month;
        this->day         = day;
        this->hour        = hour;
        this->minute      = minute;
        this->second      = second;
        this->second_part = second_part;
        this->time_type   = MYSQL_TIMESTAMP_DATETIME;
    }
};

// '1970-01-01 00:00:01' UTC to '2038-01-19 03:14:07' UTC
struct mysql_timestamp : public mysql_datetime
{
    using mysql_datetime::mysql_datetime;
};


} // namespace jkl