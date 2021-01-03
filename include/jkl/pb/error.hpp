#pragma once

#include <jkl/config.hpp>
#include <jkl/error.hpp>


namespace jkl{


enum class pb_err
{
    tag_mismatch = 1,
    varint_too_large,
    varint_incomplete,
    fixed_incomplete,
    repeated_incomplete,
    bytes_incomplete,
    msg_incomplete,
    invalid_length,
    more_data_than_required,
    validation_failed,
    required_field_missing
};

class pb_err_category : public aerror_category
{
public:
    char const* name() const noexcept override { return "pb_err"; }

    std::string message(int condition) const override
    {
        switch(condition)
        {
            case static_cast<int>(pb_err::tag_mismatch) :
                return "tag mismatch";
            case static_cast<int>(pb_err::varint_too_large) :
                return "varint too large";
            case static_cast<int>(pb_err::varint_incomplete) :
                return "varint incomplete";
            case static_cast<int>(pb_err::fixed_incomplete) :
                return "fixed incomplete";
            case static_cast<int>(pb_err::repeated_incomplete) :
                return "repeated incomplete";
            case static_cast<int>(pb_err::bytes_incomplete) :
                return "bytes incomplete";
            case static_cast<int>(pb_err::msg_incomplete) :
                return "msg incomplete";
            case static_cast<int>(pb_err::invalid_length) :
                return "invalid length";
            case static_cast<int>(pb_err::more_data_than_required) :
                return "more data than required";
            case static_cast<int>(pb_err::validation_failed) :
                return "validation failed";
            case static_cast<int>(pb_err::required_field_missing) :
                return "required field missing";
        }

        return "undefined";
    }
};

inline constexpr pb_err_category g_pb_err_cat;
inline auto const& pb_err_cat() noexcept { return g_pb_err_cat; }

inline aerror_code      make_error_code     (pb_err e) noexcept { return {static_cast<int>(e), pb_err_cat()}; }
inline aerror_condition make_error_condition(pb_err e) noexcept { return {static_cast<int>(e), pb_err_cat()}; }


} // namespace jkl


namespace boost::system{

template<> struct is_error_code_enum<jkl::pb_err> : public std::true_type {};

} // namespace boost::system