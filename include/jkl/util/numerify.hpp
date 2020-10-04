#pragma once

#include <jkl/config.hpp>
#include <jkl/util/str.hpp>
#include <jkl/util/concepts.hpp>
#include <optional>
#include <charconv>
#include <cstdlib>


namespace jkl{


bool numerify(_char_str_ auto const& s, std::integral auto& v, int base = 10) noexcept
{
    char const* beg = str_data(s);
    char const* end = beg + str_size(s);
    auto[e, ec] = std::from_chars(beg, end, v, base);
    return ec == std::errc{} && e == end;
}


template<std::floating_point V>
bool numerify(_char_str_ auto const& s, V& v, int = 0) noexcept
{
    if constexpr(requires{ std::from_chars("", "", v); })
    {
        char const* beg = str_data(s);
        char const* end = beg + str_size(s);
        auto[e, ec] = std::from_chars(beg, end, v);
        return ec == std::errc{} && e == end;
    }
    else
    {
        auto const css = (as_cstr)(s);
        char const* cstr = cstr_data(css);
        BOOST_ASSERT(cstr);

        if(BOOST_UNLIKELY(*cstr == '\0'))
            return false;

        char* endptr;
        errno = 0;

        if constexpr(std::is_same_v<V, float>)
            v = std::strtof(cstr, &endptr);
        else if constexpr(std::is_same_v<V, double>)
            v = std::strtod(cstr, &endptr);
        else if constexpr(std::is_same_v<V, long double>)
            v = std::strtold(cstr, &endptr);
        else
            JKL_DEPENDENT_FAIL(V, "unsupported type");

        return *endptr == '\0' && errno != ERANGE;
    }
}


template<_arithmetic_ T>
std::optional<T> to_num(_char_str_ auto const& s, int base = 10) noexcept
{
    T v;
    if(numerify(s, v, base))
        return v;
    return std::nullopt;
}


} // namespace jkl
