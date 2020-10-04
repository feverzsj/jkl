#pragma once


#include <jkl/util/str>

#include <exception>
#include <memory>

#include <mysql/mysql.h>


namespace jkl{


class mysql_err : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;

    template<class Holder>
    explicit mysql_err(Holder& h)
        : std::runtime_error{h.err_str()}
    {}
};


template<class T, class Holder>
void mysql_throw_on_err(T r, Holder& h)
{
    if(r != 0)
        throw mysql_err{h};
}

template<class T, class Holder>
void mysql_throw_on_err(T* r, Holder& h)
{
    if(! r)
        throw mysql_err{h};
}

template<class Holder>
MYSQL_RES* mysql_throw_on_err(MYSQL_RES* r, Holder& h)
{
    if(! r)
    {
        if(char const* err = h.err_str())
            throw mysql_err{err};
    }

    return r;
}


#define _JKL_MYSQL_ERR_HANDLE(expr) mysql_throw_on_err(expr, *this)


} // namespace jkl

