#pragma once


#include <string_view>
#include <cstdint>


namespace jkdf{


enum pbf_wire_type : uint32_t
{
    pbf_varint  = 0, // int32/64, uint32/64, sint32/64, bool, enum
    pbf_fix64   = 1, // fixed64, sfixed64, double
    pbf_len_dlm = 2, // string, bytes, nested messages, packed repeated fields
    pbf_fix32   = 5, // fixed32, sfixed32, float

    // our define, not an actual wire type
    // detailed wired type
    pbf_wt_mask       = 0xff,
    pbf_zigzag_varint = 1 << 8 | pbf_varint, // sint32/64
    pbf_bool_varint   = 2 << 8 | pbf_varint,
    pbf_sub_msg       = 1 << 8 | pbf_len_dlm
};


template<uint32_t Id, class T, pbf_wire_type DWT>
struct pbf_type
{
    static_assert(Id > 0);

    using type = T;

    constexpr uint32_t      id = Id;
    constexpr pbf_wire_type wire_type          = static_cast<pbf_wire_type>(DWT & pbf_wt_mask);
    constexpr pbf_wire_type detailed_wire_type = DWT;
};


template<uint32_t Id> using pbf_uint32 = pbf_type<Id, uint32_t, pbf_varint       >;
template<uint32_t Id> using pbf_uint64 = pbf_type<Id, uint64_t, pbf_varint       >;
template<uint32_t Id> using pbf_sint32 = pbf_type<Id, int32_t , pbf_zigzag_varint>;
template<uint32_t Id> using pbf_sint64 = pbf_type<Id, int64_t , pbf_zigzag_varint>;
template<uint32_t Id> using pbf_bool   = pbf_type<Id, bool    , pbf_bool_varint  >;
template<uint32_t Id> using pbf_enum   = pbf_uint32<Id>;

template<uint32_t Id> using pbf_fixed32  = pbf_type<Id, uint32_t, pbf_fix32>;
template<uint32_t Id> using pbf_fixed64  = pbf_type<Id, uint64_t, pbf_fix64>;
template<uint32_t Id> using pbf_sfixed32 = pbf_type<Id, int32_t , pbf_fix32>;
template<uint32_t Id> using pbf_sfixed64 = pbf_type<Id, int64_t , pbf_fix64>;
template<uint32_t Id> using pbf_float    = pbf_type<Id, float   , pbf_fix32>;
template<uint32_t Id> using pbf_double   = pbf_type<Id, double  , pbf_fix64>;

template<uint32_t Id> using pbf_bytes   = pbf_type<Id, std::string_view, pbf_len_dlm>;
template<uint32_t Id> using pbf_string  = pbf_type<Id, std::string_view, pbf_len_dlm>;
template<uint32_t Id> using pbf_message = pbf_type<Id, void            , pbf_sub_msg>;


} // namespace jkdf