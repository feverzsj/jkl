#pragma once

#include <jkl/config.hpp>
#include <jkl/result.hpp>
#include <jkl/util/buf.hpp>
#include <jkl/util/integral_constant.hpp>
#include <jkl/pb/type.hpp>
#include <jkl/pb/error.hpp>
#include <bit>
#include <cstdint>


namespace jkl{


template<std::integral T>
using fast_sint_t = std::conditional_t<sizeof(T) <= sizeof(int32_t), int32_t, std::make_signed_t<T>>;

template<std::integral T>
using fast_uint_t = std::conditional_t<sizeof(T) <= sizeof(uint32_t), uint32_t, std::make_unsigned_t<T>>;

template<std::integral T>
using fast_int_t = std::conditional_t<std::is_signed_v<T>, fast_sint_t<T>, fast_uint_t<T>>;


// ZigZag encoding that maps signed integers with a small absolute value
// to unsigned integers with a small (positive) values. Without this,
// encoding negative values using varint would use up 9 or 10 bytes.
//   if x >= 0, zigzag_encode(x) == 2*x
//   if x <  0, zigzag_encode(x) == 2*(-x) - 1
// ref:
//   https://developers.google.com/protocol-buffers/docs/encoding#signed-integers
//   https://github.com/protocolbuffers/protobuf/blob/master/src/google/protobuf/wire_format_lite.h#L851

template<bool ReturnFastUint = false>
constexpr auto zigzag_encode(_signed_ auto v) noexcept
{
    using ut = fast_uint_t<decltype(v)>;
    using st = std::make_signed_t<ut>;

    // NOTE: the integral promotion then arithmetic conversions apply to any arithmetic op(including bitwise op).
    //       https://en.cppreference.com/w/cpp/language/operator_arithmetic#Bitwise_logic_operators
    //       https://en.cppreference.com/w/cpp/language/implicit_conversion#Integral_promotion
    //       https://en.cppreference.com/w/cpp/language/operator_arithmetic#Conversions

    // NOTE: the left-shift must be unsigned because of overflow
    //       the right-shift must be arithmetic
    ut u = (static_cast<ut>(v) << 1) ^ (static_cast<st>(v) >> (sizeof(v) * 8 - 1));

    if constexpr(ReturnFastUint)
        return u;
    else
        return static_cast<std::make_unsigned_t<decltype(v)>>(u);
}

// NOTE: accept only unsigned types to prevent undefined behavior
template<bool ReturnFastUint = false>
constexpr auto zigzag_decode(_unsigned_ auto v) noexcept
{
    using ut = fast_uint_t<decltype(v)>;

    ut u = v;
    u = (u >> 1) ^ (~(u & 1) + 1);

    if constexpr(ReturnFastUint)
        return u;
    else
        return static_cast<std::make_signed_t<decltype(v)>>(u);
}


// T is the type that can be encoded as varint
template<class T, bool Zigzag>
concept _varint_ = std::same_as<T, bool>
                    || (std::integral<T> && (sizeof(T) <= sizeof(uint64_t)) && (! Zigzag || _signed_<T>));

// the actual unsigned type that will be encoded as varint
template<class T, bool Zigzag = false> requires(_varint_<T, Zigzag>)
using varint_uint_t = typename
                          std::conditional_t<std::is_same_v<T, bool>  , std::type_identity<uint8_t >,
                          std::conditional_t<(! Zigzag && _signed_<T>), std::type_identity<uint64_t>,
                                                                        std::make_unsigned<T>>
                      >::type;

template<class T, bool Zigzag = false> requires(_varint_<T, Zigzag>)
using varint_fast_uint_t = fast_uint_t<varint_uint_t<T, Zigzag>>;


using pb_size_t = uint32_t;

template<pb_size_t V>
inline constexpr auto pb_size_c = integral_c<V>;

// the max bytes required to encode T.
template<class T, bool Zigzag = false> requires(_varint_<T, Zigzag>)
inline constexpr pb_size_t max_varint_wire_size = std::is_same_v<T, bool> ? 1 : (sizeof(varint_uint_t<T, Zigzag>) * 8 / 7 + 1);

// the max value for last/msb byte on wire
template<class T, bool Zigzag = false> requires(_varint_<T, Zigzag>)
inline constexpr uint8_t max_varint_last_byte = std::is_same_v<T, bool> ? 0x7f : (1 << (sizeof(varint_uint_t<T, Zigzag>) * 8 % 7)) - 1;


// the bytes required to encode 'v'.
template<bool Zigzag = false, _varint_<Zigzag> T>
constexpr pb_size_t varint_wire_size(T v)
{
    if constexpr(std::is_same_v<T, bool>)
    {
        return 1;
    }
    else
    {
        using ut = varint_fast_uint_t<T, Zigzag>;

        ut u;

        if constexpr(Zigzag)
            u = zigzag_encode<true>(v);
        else
            u = static_cast<ut>(v);

        return (sizeof(u)*8 - std::countl_zero(u | 1) + 6) / 7;
    }
}


// ref:
//   https://developers.google.com/protocol-buffers/docs/encoding#varints
//   https://github.com/protocolbuffers/protobuf/blob/master/src/google/protobuf/io/coded_stream.h#L1145
//   
// b should be at least max_varint_wire_size<T, Zigzag>
// NOTE: negative number is always encoded using ten bytes, so it must be converted to uint64_t v first
template<bool Zigzag = false, _varint_<Zigzag> T>
constexpr auto* append_varint(uint8_t* b, T v) noexcept
{
    if constexpr(std::is_same_v<T, bool>)
    {
        *b = v;
        return b + 1;
    }
    else
    {
        using ut = varint_fast_uint_t<T, Zigzag>;
    
        ut u;

        if constexpr(Zigzag)
            u = zigzag_encode<true>(v);
        else
            u = static_cast<ut>(v);

        while(u >= 0x80)
        {
            *b++ = static_cast<uint8_t>(u | 0x80);
            u >>= 7;
        }
    
        *b++ = static_cast<uint8_t>(u);

        return b;
    }
}

// reinterpret_cast is not allowed when constexpr function is evaluated for constant
template<bool Zigzag = false, _varint_<Zigzag> T>
constexpr auto* append_varint(_byte_ auto* b, T v) noexcept
{
    return reinterpret_cast<decltype(b)>(
        append_varint<Zigzag>(reinterpret_cast<uint8_t*>(b), v));
}

template<bool Zigzag = false, _varint_<Zigzag> T>
constexpr void append_varint(_resizable_byte_buf_ auto& b, T v)
{
    if constexpr(std::is_same_v<T, bool>)
    {
        *reinterpret_cast<uint8_t*>(buy_buf(b, 1)) = v;
    }
    else
    {
        auto* e = append_varint<Zigzag>(buy_buf(b, max_varint_wire_size<T, Zigzag>), v);
        resize_buf(b, e - buf_data(b));
    }
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr decltype(auto) append_pb_tag(auto&& b, uint32_t id, pb_wire_type wt)
{
    return append_varint(JKL_FORWARD(b), static_cast<uint32_t>((id << 3) | wt));
}


template<auto V, bool Zigzag = false> requires(_varint_<decltype(V), Zigzag>)
inline constexpr auto varint_bytes = [](){
    std::array<uint8_t, varint_wire_size<Zigzag>(V)> b;
    append_varint<Zigzag>(b.data(), V);
    return b;
}();

template<auto V, bool Zigzag = false, _byte_ B> requires(_varint_<decltype(V), Zigzag>)
constexpr B* append_varint_bytes(B* b) noexcept
{
    constexpr auto s = varint_bytes<V, Zigzag>;
    memcpy(b, s.data(), s.size());
    return b + s.size();
}

template<auto V, bool Zigzag = false> requires(_varint_<decltype(V), Zigzag>)
constexpr void append_varint_bytes(_resizable_byte_buf_ auto& b)
{
    constexpr auto s = varint_bytes<V, Zigzag>;
    memcpy(buy_buf(b, s.size()), s.data(), s.size());
}

// check if varint V is encoded at head of [beg, end), return nullptr if not or the end of varint bytes in [beg, end)
template<auto V, bool Zigzag = false, _byte_ B> requires(_varint_<decltype(V), Zigzag>)
constexpr B const* starts_with_varint_bytes(B const* beg, B const* end) noexcept
{
    constexpr auto s = varint_bytes<V, Zigzag>;

    if((beg + s.size() < end) && (memcmp(beg, s.data(), s.size()) == 0))
        return beg + s.size();
    return nullptr;
}

// ref:
//   https://github.com/protocolbuffers/protobuf/blob/master/src/google/protobuf/io/coded_stream.h#L909
//   https://github.com/facebook/folly/blob/master/folly/Varint.h
//
// NOTE: negative number is always encoded using ten bytes, so v must be uint64_t for such case
// return <bytes_used, succeed_or_not>
// v is only assigned on succeed
template<bool Zigzag = false, _varint_<Zigzag> T>
constexpr aresult<uint8_t const*> read_varint(uint8_t const* b, uint8_t const* e, T& v) noexcept
{
    if constexpr(std::is_same_v<T, bool>)
    {
        if(BOOST_LIKELY(b < e))
        {
            v = *b;
            return b + 1;
        }
        return pb_err::varint_incomplete;
    }
    else
    {
        using ut = varint_fast_uint_t<T, Zigzag>;

        constexpr auto maxSize     = max_varint_wire_size<T, Zigzag>;
        constexpr ut   maxLastByte = max_varint_last_byte<T, Zigzag>;

        ut u = 0;

        if(BOOST_LIKELY(b + maxSize <= e)) // fast path
        {
            do
            {
                ut t;

            #define __ONE_BYTE(ms) if constexpr(maxSize > ms) { t = *b++; u |= (t & 0x7f) << (7*ms); if(t < 0x80) break; }
            #define __LAST_BYTE(ms)  \
                if constexpr(maxSize > ms)                                                                \
                {                                                                                         \
                    t = *b++;                                                                             \
                    if constexpr(maxSize == ms + 1) { if(t <= maxLastByte) { u |= t << (7*ms); break; } } \
                    else                            { u |= (t & 0x7f) << (7*ms); if(t < 0x80) break; }    \
                }                                                                                         \
                /**/

                t = *b++; u = (t & 0x7f); if(t < 0x80) break;

                __LAST_BYTE(1) // uint8_t
                __LAST_BYTE(2) // uint16_t
                __ONE_BYTE (3)
                __LAST_BYTE(4) // uint32_t
                __ONE_BYTE (5)
                __ONE_BYTE (6)
                __ONE_BYTE (7)
                __ONE_BYTE (8)
                __LAST_BYTE(9) // uint64_t

                static_assert(maxSize <= 10);
                return pb_err::varint_too_large;

            #undef __ONE_BYTE
            #undef __LAST_BYTE
            }
            while(false);
        }
        else // b + maxSize > e, so no overflow in this branch
        {
            unsigned shift = 0;
        
            while(b != e && *b >= 0x80)
            {
                u |= static_cast<ut>(*b++ & 0x7f) << shift;
                shift += 7;
            }

            if(BOOST_UNLIKELY(b == e))
                return pb_err::varint_incomplete;

            u |= static_cast<ut>(*b++) << shift;
        }

        if constexpr(Zigzag)
            v = static_cast<T>(zigzag_decode<true>(u));
        else
            v = static_cast<T>(u);

        return b;
    }
}


template<bool Zigzag = false, _byte_ B, _varint_<Zigzag> T>
constexpr aresult<B const*> read_varint(B const* beg, B const* end, T& v) noexcept
{
    JKL_TRY(uint8_t const* t, read_varint<Zigzag>(reinterpret_cast<uint8_t const*>(beg), reinterpret_cast<uint8_t const*>(end), v));
    return reinterpret_cast<B const*>(t);
}


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr aresult<uint8_t const*> skip_varint(uint8_t const* b, uint8_t const* e) noexcept
{
    if(BOOST_LIKELY(b + (max_varint_wire_size<uint64_t>) <= e)) // fast path
    {
        do
        {
            if(*b++ < 0x80                           ) break; // 1
            if(*b++ < 0x80                           ) break; // 2
            if(*b++ < 0x80                           ) break; // 3
            if(*b++ < 0x80                           ) break; // 4
            if(*b++ < 0x80                           ) break; // 5
            if(*b++ < 0x80                           ) break; // 6
            if(*b++ < 0x80                           ) break; // 7
            if(*b++ < 0x80                           ) break; // 8
            if(*b++ < 0x80                           ) break; // 9
            if(*b++ <= max_varint_last_byte<uint64_t>) break; // 10
            return pb_err::varint_too_large;
        }
        while(false);
    }
    else // b + maxSize > e, so no overflow in this branch
    {
        while(b != e && *b >= 0x80)
            ++b;

        if(BOOST_UNLIKELY(b == e))
            return pb_err::varint_incomplete;

        ++b;
    }

    return b;
}

// reinterpret_cast is not allowed when constexpr function is evaluated for constant
template<_byte_ B>
constexpr aresult<B const*> skip_varint(B const* beg, B const* end) noexcept
{
    JKL_TRY(uint8_t const* t, skip_varint(reinterpret_cast<uint8_t const*>(beg), reinterpret_cast<uint8_t const*>(end)));
    return reinterpret_cast<B const*>(t);
}

} // namespace jkl
