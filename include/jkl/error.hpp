#pragma once

#include <jkl/config.hpp>
#include <jkl/util/concepts.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/error.hpp>


namespace jkl{


namespace aerrc = boost::system::errc;

using aerror_code      = boost::system::error_code;
using aerror_condition = boost::system::error_condition;
using asystem_error    = boost::system::system_error;
using aerror_category  = boost::system::error_category;


template<class T>
concept _ecenum_ = boost::system::is_error_code_enum<std::remove_cv_t<T>>::value;

template<class T>
concept _ec_or_ecenum_ = std::same_as<std::remove_cv_t<T>, aerror_code> || _ecenum_<T>;


enum class gerrc
{
    failed = 1,
    not_found,
    timeout,
    file_exists
};

class gerrc_category : public aerror_category
{
public:
    char const* name() const noexcept override { return "general"; }

    std::string message(int condition) const override
    {
        switch(condition)
        {
            case static_cast<int>(gerrc::failed) :
                return "operation failed";
            case static_cast<int>(gerrc::not_found) :
                return "resource not found";
            case static_cast<int>(gerrc::timeout) :
                return "operation timeout";
            case static_cast<int>(gerrc::file_exists) :
                return "file exists";
        }

        return "undefined";
    }
};

inline constexpr gerrc_category g_gerrc_cat;
inline auto const& gerrc_cat() noexcept { return g_gerrc_cat; }

inline aerror_code      make_error_code     (gerrc e) noexcept { return {static_cast<int>(e), gerrc_cat()}; }
inline aerror_condition make_error_condition(gerrc e) noexcept { return {static_cast<int>(e), gerrc_cat()}; }


inline void throw_on_error(aerror_code const& e)
{
    if(e)
        throw asystem_error(e);
}


} // namespace jkl


namespace boost::system{

template<> struct is_error_code_enum<jkl::gerrc> : public std::true_type {};

} // namespace boost::system