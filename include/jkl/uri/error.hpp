#pragma once

#include <jkl/config.hpp>
#include <jkl/error.hpp>


namespace jkl{


enum class uri_err
{
    invalid_hex = 1,
    no_scheme_before_colon,
    unterminated_ipv6,
    invalid_port
};

class uri_err_category : public aerror_category
{
public:
    char const* name() const noexcept override { return "uri_err"; }

    std::string message(int condition) const override
    {
        switch(condition)
        {
            case static_cast<int>(uri_err::invalid_hex) :
                return "invalid hex";
            case static_cast<int>(uri_err::no_scheme_before_colon) :
                return "no scheme before ':'";
            case static_cast<int>(uri_err::unterminated_ipv6) :
                return "unterminated IPv6 address";
            case static_cast<int>(uri_err::invalid_port) :
                return "bad or invalid port number";
        }

        return "undefined";
    }
};

inline constexpr uri_err_category g_uri_err_cat;
inline auto const& uri_err_cat() noexcept { return g_uri_err_cat; }

inline aerror_code      make_error_code     (uri_err e) noexcept { return {static_cast<int>(e), uri_err_cat()}; }
inline aerror_condition make_error_condition(uri_err e) noexcept { return {static_cast<int>(e), uri_err_cat()}; }


} // namespace jkl


namespace boost::system{

template<> struct is_error_code_enum<jkl::uri_err> : public std::true_type {};

} // namespace boost::system