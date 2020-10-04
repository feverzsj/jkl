#pragma once


#include <jkl/mysql/err.hpp>


namespace jkl{


class mysql_env
{
public:
    explicit mysql_env(int argc = 0, char** argv = nullptr, char** groups = nullptr)
    {
        if(0 != mysql_library_init(argc, argv, groups))
            throw mysql_err{"mysql_library_init() failed"};
    }

    ~mysql_env()
    {
        mysql_library_end();
    }
};


class mysql_thread_env
{
public:
    mysql_thread_env()
    {
        if(0 != mysql_thread_init())
            throw mysql_err{"mysql_thread_init() failed"};
    }

    ~mysql_thread_env()
    {
        mysql_thread_end();
    }
};


} // namespace jkl