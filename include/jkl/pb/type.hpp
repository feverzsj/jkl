#pragma once


namespace jkl{


enum pb_wire_type : uint32_t
{
    pb_wt_varint  = 0, // int32/64, uint32/64, sint32/64, bool, enum
    pb_wt_fix64   = 1, // fixed64, sfixed64, double
    pb_wt_len_dlm = 2, // string, bytes, nested messages, packed repeated fields
    pb_wt_fix32   = 5  // fixed32, sfixed32, float
};


} // namespace jkl
