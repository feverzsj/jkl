#pragma once

#include <jkl/config.hpp>
#include <curl/curl.h>
#include <memory>
#include <string>


namespace jkl{


class curlstr
{
    struct deleter{ void operator()(char* s) noexcept { curl_free(s); } };
    std::unique_ptr<char, deleter> _s;

public:
    explicit curlstr(char* s) : _s(s) {}
    char const* c_str() const noexcept { return _s ? _s.get() : ""; }
    char const* data () const noexcept { return _s ? _s.get() : ""; }
    size_t      size () const noexcept { return _s ? strlen(_s.get()) : 0; }

    template<class Tx>                            // V
    operator std::basic_string_view<char, Tx>() const& { return {data(), size()}; }

    template<class Tx, class Ax>
    operator std::basic_string<char, Tx, Ax>() const { return {data(), size()}; }
};


} // namespace jkl