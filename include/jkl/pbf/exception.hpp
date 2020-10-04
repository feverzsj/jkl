#pragma once


#include <stdexcept>


namespace jkdf{


struct pbf_exception : std::runtime_error
{
    explicit pbf_exception(char const* msg)
        : std::runtime_error(msg)
    {}
};


} // namespace jkdf