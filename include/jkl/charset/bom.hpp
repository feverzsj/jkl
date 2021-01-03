#pragma once

#include <jkl/config.hpp>


namespace jkl{


struct detect_bom_result
{
    char const* charset = nullptr;
    unsigned    bom_len = 0;

    explicit operator bool() const noexcept { return charset != nullptr; }
};


// return <code page, bom length>
// https://en.wikipedia.org/wiki/Byte_order_mark
detect_bom_result detect_bom(string_view s)
{
    if(s.starts_with("\0xEF\0xBB\0xBF"     )) return {"UTF-8"     , 3};
    if(s.starts_with("\0xFE\0xFF"          )) return {"UTF-16 BE" , 2};
    if(s.starts_with("\0xFF\0xFE"          )) return {"UTF-16 LE" , 2};
    if(s.starts_with("\0x00\0x00\0xFE\0xFF")) return {"UTF-32 BE" , 4};
    if(s.starts_with("\0xFF\0xFE\0x00\0x00")) return {"UTF-32 LE" , 4};
    if(s.starts_with("\0xF7\0x64\0x4C"     )) return {"UTF-1"     , 3};
    if(s.starts_with("\0xDD\0x73\0x66\0x73")) return {"UTF-EBCDIC", 4};
    if(s.starts_with("\0x0E\0xFE\0xFF"     )) return {"SCSU"      , 3};
    if(s.starts_with("\0xFB\0xEE\0x28"     )) return {"BOCU-1"    , 3};
    if(s.starts_with("\0x84\0x31\0x95\0x33")) return {"GB-18030"  , 4};

    if(s.starts_with("\0x2B\0x2F\0x76"))
    {
        s = s.subview(3);

        if(s.starts_with("\0x38\0x2D"))
            return {"UTF-7", 5};

        if(s.starts_with("\0x38") || s.starts_with("\0x39") || s.starts_with("\0x2B") || s.starts_with("\0x2F"))
            return {"UTF-7", 4};
    }

    return {};
}


} // namespace jkl
