#pragma once

#include <jkl/config.hpp>
#include <jkl/error.hpp>
#include <jkl/result.hpp>
#include <jkl/util/buf.hpp>
#include <cppcodec/base64_url.hpp>
#include <cppcodec/base64_rfc4648.hpp>


namespace jkl{


enum class cppcodec_err
{
    decode_failed = 1,
};

class cppcodec_err_category : public aerror_category
{
public:
    char const* name() const noexcept override { return "cppcodec"; }

    std::string message(int condition) const override
    {
        switch(condition)
        {
            case static_cast<int>(cppcodec_err::decode_failed) :
                return "decode failed";
        }

        return "undefined";
    }
};

inline constexpr cppcodec_err_category g_cppcodec_err_cat;
inline auto const& cppcodec_err_cat() noexcept { return g_cppcodec_err_cat; }

inline aerror_code      make_error_code     (cppcodec_err e) noexcept { return {static_cast<int>(e), cppcodec_err_cat()}; }
inline aerror_condition make_error_condition(cppcodec_err e) noexcept { return {static_cast<int>(e), cppcodec_err_cat()}; }


} // namespace jkl


namespace boost::system{

template<> struct is_error_code_enum<jkl::cppcodec_err> : public std::true_type {};

} // namespace boost::system



namespace jkl{


template<class Codec>
class cppcodec_wrapper : public Codec
{
    using Codec::encoded_size;
    using Codec::decoded_max_size;

    static constexpr size_t encoded_size(_byte_str_ auto const& s) noexcept
    {    cppcodec::base64_url::encode()

        return Codec::encoded_size(str_size(s));
    }

    static constexpr size_t decoded_max_size(_byte_str_ auto const& s) noexcept
    {
        return Codec::decoded_max_size(str_size(s));
    }


    static constexpr auto* encode_append(_byte_ auto* d, _byte_ auto* s, size_t n) noexcept
    {
        return d + Codec::encode(d, Codec::encoded_size(n), s, n);
    }

    static constexpr auto* encode_append(_byte_ auto* d, _byte_str_ auto const& s) noexcept
    {
        return encode_append(d, str_data(s), str_size(s));
    }

    static constexpr void encode_append(_resizable_byte_buf_ auto& d, _byte_str_ auto const& s)
    {
        size_t  n = str_size(s);
        size_t en = Codec::encoded_size(n);
        Codec::encode(buy_buf(d, en), en, str_data(s), n);
    }

    static constexpr void encode_assign(_resizable_byte_buf_ auto& d, _byte_str_ auto const& s)
    {
        size_t  n = str_size(s);
        size_t en = Codec::encoded_size(n);
        resize_buf(d, en);
        Codec::encode(buf_data(d), en, str_data(s), n);

    }

    template<_resizable_byte_buf_ B = string>
    static constexpr B encoded(_byte_str_ auto const& s)
    {
        B d;
        encode_assign(d, s);
        return d;
    }


    template<_byte_ T>
    static constexpr aresult<T> decode_append(T* d, _byte_ auto* s, size_t n)
    {
        try
        {
            return d + Codec::decode(d, Codec::decoded_max_size(n), s, n);
        }
        catch(cppcodec::parse_error const&)
        {
            return cppcodec_err::decode_failed;
        }
    }

    template<_byte_ T>
    static constexpr aresult<T> decode_append(T* d, _byte_str_ auto const& s)
    {
        return decode_append(d, str_data(s), str_size(s));
    }

    static constexpr aresult<> decode_append(_resizable_byte_buf_ auto& d, _byte_str_ auto const& s)
    {
        size_t  n = str_size(s);
        size_t mn = Codec::decoded_max_size(n);
        size_t dn = buf_size(d);
        try
        {
            mn = Codec::decode(buy_buf(d, mn), mn, str_data(s), n);
        }
        catch(cppcodec::parse_error const&)
        {
            return cppcodec_err::decode_failed;
        }
        resize_buf(d, dn + mn);
        return no_err;
    }

    static constexpr aresult<> decode_assign(_resizable_byte_buf_ auto& d, _byte_str_ auto const& s)
    {
        size_t  n = str_size(s);
        size_t mn = Codec::decoded_max_size(n);
        resize_buf(d, mn);
        try
        {
            mn = Codec::decode(buf_data(d), mn, str_data(s), n);
        }
        catch(cppcodec::parse_error const&)
        {
            return cppcodec_err::decode_failed;
        }
        resize_buf(d, mn);
        return no_err;
    }

    template<_resizable_byte_buf_ B = string>
    static constexpr aresult<B> decoded(_byte_str_ auto const& s)
    {
        B d;
        JKL_TRY(decode_assign(d, s));
        return d;
    }
};


using base64_url     = cppcodec_wrapper<cppcodec::base64_url>;
using base64_rfc4648 = cppcodec_wrapper<cppcodec::base64_rfc4648>;


} // namespace jkl
