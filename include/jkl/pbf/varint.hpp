#pragma once

#include <cstdint>
#include <type_traits>


namespace jkdf{


template<class T>
struct max_varint_size
{
    static_assert(std::is_integral_v<T>);
    static_assert(sizeof(T) <= sizeof(uint64_t));

    static constexpr size_t value = std::is_signed_v<T> ? sizeof(uint64_t) * 8 / 7 + 1
                                                        : sizeof(T) * 8 / 7 + 1;
};

template<class T>
constexpr auto max_varint_size_v = max_varint_size<T>::value;


// ZigZag encoding that maps signed integers with a small absolute value
// to unsigned integers with a small (positive) values. Without this,
// encoding negative values using varint would use up 9 or 10 bytes.
//   if x >= 0, zigzag_encode(x) == 2*x
//   if x <  0, zigzag_encode(x) == -2*x + 1
// ref:
//   https://developers.google.com/protocol-buffers/docs/encoding#signed-integers
//   https://github.com/protocolbuffers/protobuf/blob/master/src/google/protobuf/wire_format_lite.h#L851

template<class T>
BOOST_FORCEINLINE std::make_unsigned_t<T> zigzag_encode(T v)
{
    static_assert(std::is_signed_v<T>);

    return (static_cast<std::make_unsigned_t<T>>(v) << 1) ^ (v >> (sizeof(T)*CHAR_BIT - 1));
    //                                                        Note: the right-shift must be arithmetic
}

template<class T>
BOOST_FORCEINLINE std::make_signed_t<T> zigzag_decode(T v)
{
    static_assert(std::is_unsigned_v<T>);

    return (n >> 1) ^ -static_cast<std::make_signed_t<T>>(n & 1);
}


// ref:
//   https://developers.google.com/protocol-buffers/docs/encoding#varints
//   https://github.com/protocolbuffers/protobuf/blob/master/src/google/protobuf/io/coded_stream.h#L1145
//   
// NOTE: negative number is always encoded using ten bytes, so it must be converted to uint64_t v first
template<class T>
size_t write_varint(void* b, T v)
{
    static_assert(std::is_unsigned_v<T>);

    uint8_t* p = static_cast<uint8_t*>(b);

    while(v >= 0x80)
    {
        *p++ = static_cast<uint8_t>(v | 0x80);
        v >>= 7;
    }
    
    *p++ = static_cast<uint8_t>(v);

    return static_cast<size_t>(p - static_cast<uint8_t*>(b));
}


// ref:
//   https://github.com/protocolbuffers/protobuf/blob/master/src/google/protobuf/io/coded_stream.h#L909
//   https://github.com/facebook/folly/blob/master/folly/Varint.h
//
// NOTE: negative number is always encoded using ten bytes, so val must be uint64_t for such case
// return <bytes_used, succeed_or_not>
// val is only assigned on succeed
template<class Buf, class T>
std::pair<size_t, bool> read_varint(Buf const& buf, T& val)
{
    static_assert(is_byte_buf_v<Buf>);
    static_assert(std::is_unsigned_v<T>);

    constexpr size_t max_size = max_varint_size_v<T>;

    int8_t const* b = reinterpret_cast<int8_t const*>(buf_begin(buf));
    T             v = 0;

    if(BOOST_LIKELY(buf_size(buf) >= max_size)) // fast path
    {
        do
        {
            std::make_signed_t<T> t;
                                       t = *b++; v  = T((t & 0x7f)      ); if(t >= 0) break;
            if constexpr(max_size > 1) t = *b++; v |= T((t & 0x7f) <<  7); if(t >= 0) break;
            if constexpr(max_size > 2) t = *b++; v |= T((t & 0x7f) << 14); if(t >= 0) break;
            if constexpr(max_size > 3) t = *b++; v |= T((t & 0x7f) << 21); if(t >= 0) break;
            if constexpr(max_size > 4) t = *b++; v |= T((t & 0x7f) << 28); if(t >= 0) break;
            if constexpr(max_size > 5) t = *b++; v |= T((t & 0x7f) << 35); if(t >= 0) break;
            if constexpr(max_size > 6) t = *b++; v |= T((t & 0x7f) << 42); if(t >= 0) break;
            if constexpr(max_size > 7) t = *b++; v |= T((t & 0x7f) << 49); if(t >= 0) break;
            if constexpr(max_size > 8) t = *b++; v |= T((t & 0x7f) << 56); if(t >= 0) break;
            if constexpr(max_size > 9) t = *b++; v |= T((t & 0x01) << 63); if(t >= 0) break;
            static_assert(max_size <= 10);
            return {b - buf_begin(buf), false}; // varint too long;
        }
        while(false);
    }
    else // e - b < max_size
    {
        int8_t const* e = reinterpret_cast<int8_t const*>(buf_end(buf));
        unsigned shift = 0;
        
        while(b != e && *b < 0)
        {
            v |= T(*b++ & 0x7f) << shift;
            shift += 7;
        }

        if(b == e) // varint incomplete
            return {b - buf_begin(buf), false};

        // this is unnecessary, since e - b < max_size in this branch
        //if(shift / 7 > max_size - 1)
        //    return {b - buf_begin(buf), false};

        v |= T(*b++) << shift;
    }

    val = v;
    return {b - buf_begin(buf), true};
}


} // namespace jkdf