#pragma once


#include <cassert>
#include <iostream>


namespace jkl{



struct dummy_stream
{
    template<class T>
    dummy_stream& operator<<(T const&)
    {
        return *this;
    }

    void flush() {}
};


template<class Stream>
struct line_stream
{
    Stream& stream;
    
    explicit line_stream(Stream& s) : stream{s} {}

    template<class T>
    line_stream& operator<<(T const& t)
    {
        stream << t;
        return *this;
    }

    ~line_stream()
    {
        stream << '\n';
        stream.flush();
    }
};


} // namespace jkl


// you can repalce these macros with your logger

#ifndef JKL_LOG
#   define JKL_LOG ::jkl::line_stream{std::cout}
#endif

#ifndef JKL_WARN
#   define JKL_WARN ::jkl::line_stream{std::cerr}
#endif

#ifndef JKL_ERR
#   define JKL_ERR ::jkl::line_stream{std::cerr}
#endif


#ifndef JKL_DEBUG
#   ifdef NDEBUG
#       define JKL_DEBUG ::jkl::dummy_stream{}
#   else
#       define JKL_DEBUG ::jkl::line_stream{std::cout}
#   endif
#endif
