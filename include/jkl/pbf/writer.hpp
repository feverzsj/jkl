#pragma once


#include <jkdf/sery/varint.hpp>
#include <jkdf/pbf/type.hpp>
#include <jkdf/pbf/exception.hpp>


namespace jkdf{


constexpr uint32_t pbf_always_shrink = std::numeric_limits<uint32_t>::max();
constexpr uint32_t pbf_never_shrink  = pbf_always_shrink - 1;


template<class Buf>
class pbf_writer
{
    static_assert(is_byte_buf_v<Buf>);

    Buf& _buf;

    template<class T>
    void add_varint(T v)
    {
        constexpr auto s = max_varint_size_v<T>;
        auto n = write_varint(buy_buf(_buf, s), v);
        resize_buf(_buf, buf_size(_buf) - s + n);
    }

    template<uint32_t Id, uint32_t WireType>
    void write_tag()
    {
        add_varint<uint32_t>((Id << 3u) | (WireType & pbf_wt_mask))
    }

    template<class Ptype, class T>
    void write_value(T const& v)
    {
        using type = typename Ptype::type;

        if constexpr(Ptype::detailed_wire_type == pbf_varint)
        {
            add_varint<type>(v); // only allow implicit conversion
        }
        else if constexpr(Ptype::detailed_wire_type == pbf_zigzag_varint)
        {
            add_varint(zigzag_encode<type>(v)); // only allow implicit conversion
        }
        else if constexpr(Ptype::detailed_wire_type == pbf_bool_varint)
        {
            *buy_buf(_buf, 1) = v ? 1 : 0;
        }
        else if constexpr(Ptype::detailed_wire_type == pbf_fix32 || Ptype::detailed_wire_type == pbf_fix64)
        {
            type const cv = v; // only allow implicit conversion
            store_le(buy_buf(_buf, sizeof(type)), cv);
        }
        else if constexpr(Ptype::wire_type == pbf_len_dlm)
        {
            add_varint(static_cast<uint32_t>(std::size(v)));
            uappend(_buf, v);
        }
        else
        {
            _JKDF_DEPENDENT_FAIL(Ptype, "unsupported type");
        }
    }

public:
    explicit pbf_writer(Buf& b)
        : _buf{b}
    {}

    template<class Ptype, class T>
    void add(T const& v)
    {
        write_tag<Ptype::id, Ptype::wire_type>();
        write_value<Ptype>(v);
    }

    template<class Ptype, class Range>
    void add_repeated(Range const& r)
    {
        for(auto const& v : r)
            add<Ptype>(v);
    }

    // to avoid move large buffer to fit len varint, we use "fixed length" varint to represent length.
    // since most varint decode implementation read bytes until encounter a bytes with first bit set to 0,
    // we can append dummy bytes to fill a fixed length buffer and still make the varint decode
    // to read all the bytes and give same result.
    template<
        // initial length delimiter bytes, can hold maxLen = 2^(7*InitLenDlmBytes) bytes
        size_t   InitLenDlmBytes  = 2,
        uint32_t MaxBytesToShrink = pbf_never_shrink>
    class scoped_len_dlm_guard
    {
        static_assert(0 < InitLenDlmBytes && InitLenDlmBytes <= max_varint_size_v<uint32_t>);

        Buf&   _buf;
        size_t _pos;

        explicit scoped_len_dlm_guard(Buf& b)
            : _buf{b}, _pos{buf_size(b)}
        {
            resize_buf(_buf, _pos + InitLenDlmBytes);
        }

        ~scoped_len_dlm_guard()
        {
            auto const b = buf_data(_buf) + _pos;
            auto const len = static_cast<uint32_t>(buf_size(_buf) - _pos - InitLenDlmBytes);

            if(BOOST_UNLIKELY(len == 0))
            {
                *b = 0;
                resize_buf(_pos + 1);
                return;
            }

            // |--InitLenDlmBytes-|: this forms a "fixed length" varint represents 0 for InitLenDlmBytes
            // {0x80, ..., 0x80, 0, ....}
            uint8_t t[max_varint_size_v<uint32_t>];
            memset(t, 0x80, InitLenDlmBytes - 1);
            t[InitLenDlmBytes - 1] = 0;

            auto dlm = write_varint(t, len); // actual bytes for length delimiter

            if(dlm < InitLenDlmBytes)
            {
                if(MaxBytesToShrink == pbf_never_shrink || len > MaxBytesToShrink) // not shrink if it's too large
                {
                    // make it "fixed length" varint represents 'len' for InitLenDlmBytes
                    t[dlm - 1] |= 0x80;
                    dlm = InitLenDlmBytes;
                }
                else
                {
                    memmove(b + dlm, b + InitLenDlmBytes, len); // move to left
                    resize_buf(_pos + dlm + len); // shrink
                }
            }
            else if(dlm > InitLenDlmBytes)
            {
                resize_buf(_pos + dlm + len); // enlarge
                memmove(b + dlm, b + InitLenDlmBytes, len); // move to right
            }

            memcpy(b, t, dlm);
        }
    };

    template<class Ptype, size_t InitLenDlmBytes = 2, uint32_t MaxBytesToShrink = pbf_never_shrink, class Range>
    void add_packed_repeated(Range const& r)
    {
        static_assert(Ptype::wire_type != pbf_len_dlm); // only repeated scalar type can be packed

        using type = Ptype::type;

        if(std::size(r) == 0)
            return;

        write_tag<Ptype::id, pbf_len_dlm>();

        if constexpr(Ptype::detailed_wire_type == pbf_fix32 || Ptype::detailed_wire_type == pbf_fix64)
        {
            auto len = static_cast<uint32_t>(std::size(r) * sizeof(type));
            
            add_varint(len);
            
            auto p = buy_buf(_buf, len);
            
            for(type const v : r)
            {
                store_le(p, v);
                p += sizeof(type);
            }
        }
        else if constexpr(Ptype::detailed_wire_type == pbf_bool_varint)
        {
            auto len = static_cast<uint32_t>(std::size(r));

            add_varint(len);

            auto p = buy_buf(_buf, len);

            for(type const v : r)
            {
                *p = v ? 1 : 0;
                p += sizeof(type);
            }
        }
        else if constexpr(Ptype::wire_type == pbf_varint)
        {
            scoped_len_dlm_guard<InitLenDlmBytes, MaxBytesToShrink> sub{_buf};

            for(type const v : r)
                add_varint(v);
        }
        else
        {
            _JKDF_DEPENDENT_FAIL(Ptype, "unsupported type");
        }
    }

    // hold the return type in a scope to add message field
    template<uint32_t Id, size_t InitLenDlmBytes = 2, uint32_t MaxBytesToShrink = pbf_never_shrink>
    scoped_len_dlm_guard<InitLenDlmBytes, MaxBytesToShrink> add_message()
    {
        write_tag<Id, pbf_len_dlm>();
        return {_buf};
    }

    template<template<uint32_t> class KPtype, template<uint32_t> class VPtype, uint32_t Id,
             size_t InitLenDlmBytes = 1, uint32_t MaxBytesToShrink = pbf_never_shrink, class Map>
    void add_map(Map const& m)
    {
        if(m.size())
        {
            for(auto const& [k, v] : m)
            {
                auto sub = add_message<Id>();
                add<KPtype<1>>(k);
                add<VPtype<2>>(v);
            }
        }
    }
};


} // namespace jkdf