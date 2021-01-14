#pragma once

#include <jkl/util/log.hpp>
#include <jkl/pb/varint.hpp>
#include <jkl/pb/dsl.hpp>
#include <unordered_map>

#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>


TEST_SUITE("pb"){

using namespace jkl;

static_assert(max_varint_wire_size<bool    > == 1 );
static_assert(max_varint_wire_size<uint8_t > == 2 );
static_assert(max_varint_wire_size<uint16_t> == 3 );
static_assert(max_varint_wire_size<uint32_t> == 5 );
static_assert(max_varint_wire_size<uint64_t> == 10);

static_assert(max_varint_wire_size<int8_t > == 10);
static_assert(max_varint_wire_size<int16_t> == 10);
static_assert(max_varint_wire_size<int32_t> == 10);
static_assert(max_varint_wire_size<int64_t> == 10);

static_assert(max_varint_wire_size<bool   , true> == 1 );
static_assert(max_varint_wire_size<int8_t , true> == 2 );
static_assert(max_varint_wire_size<int16_t, true> == 3 );
static_assert(max_varint_wire_size<int32_t, true> == 5 );
static_assert(max_varint_wire_size<int64_t, true> == 10);


static_assert(max_varint_last_byte<bool    > == 0b01111111);
static_assert(max_varint_last_byte<uint8_t > == 0b00000001);
static_assert(max_varint_last_byte<uint16_t> == 0b00000011);
static_assert(max_varint_last_byte<uint32_t> == 0b00001111);
static_assert(max_varint_last_byte<uint64_t> == 0b00000001);

static_assert(max_varint_last_byte<int8_t > == 0b00000001);
static_assert(max_varint_last_byte<int16_t> == 0b00000001);
static_assert(max_varint_last_byte<int32_t> == 0b00000001);
static_assert(max_varint_last_byte<int64_t> == 0b00000001);

static_assert(max_varint_last_byte<bool   , true> == 0b01111111);
static_assert(max_varint_last_byte<int8_t , true> == 0b00000001);
static_assert(max_varint_last_byte<int16_t, true> == 0b00000011);
static_assert(max_varint_last_byte<int32_t, true> == 0b00001111);
static_assert(max_varint_last_byte<int64_t, true> == 0b00000001);


static_assert(varint_wire_size(true) == 1);
static_assert(varint_wire_size(false) == 1);
static_assert(varint_wire_size(0) == 1);
static_assert(varint_wire_size(1) == 1);
static_assert(varint_wire_size(127) == 1);
static_assert(varint_wire_size(static_cast<uint8_t>(127)) == 1);
static_assert(varint_wire_size(static_cast<uint16_t>(127)) == 1);
static_assert(varint_wire_size(14882) == 2);
static_assert(varint_wire_size(static_cast<uint16_t>(14882)) == 2);
static_assert(varint_wire_size(2961488830) == 5);
static_assert(varint_wire_size(2961488830u) == 5);
static_assert(varint_wire_size(7256456126) == 5);
static_assert(varint_wire_size(7256456126u) == 5);
static_assert(varint_wire_size(41256202580718336) == 8);
static_assert(varint_wire_size(41256202580718336u) == 8);
static_assert(varint_wire_size(11964378330978735131u) == 10);
static_assert(varint_wire_size(-1) == 10);
static_assert(varint_wire_size<true>(0) == 1);
static_assert(varint_wire_size<true>(1) == 1);
static_assert(varint_wire_size<true>(127) == 2);
static_assert(varint_wire_size<true>(14882) == 3);
static_assert(varint_wire_size<true>(2961488830) == 5);
static_assert(varint_wire_size<true>(-1) == 1);
static_assert(varint_wire_size<true>(-127) == 2);
static_assert(varint_wire_size<true>(-14882) == 3);
static_assert(varint_wire_size<true>(-2961488830) == 5);
static_assert(varint_wire_size<true>(std::numeric_limits<int8_t >::min()) == 2);
static_assert(varint_wire_size<true>(std::numeric_limits<int16_t>::min()) == 3);
static_assert(varint_wire_size<true>(std::numeric_limits<int32_t>::min()) == 5);
static_assert(varint_wire_size<true>(std::numeric_limits<int64_t>::min()) == 10);


TEST_CASE("varint decoding"){

    constexpr auto genDecode = [](auto t){
        return [](std::initializer_list<uint8_t> b){
            decltype(t) v;
            auto e = read_varint(b.begin(), b.end(), v).value_or_throw();
            if(e != b.end())
                throw std::runtime_error{"input not fully parsed"};
            return v;
        };
    };

    constexpr auto decodeBool = genDecode(bool    {});
    constexpr auto decode8    = genDecode(uint8_t {});
    constexpr auto decode16   = genDecode(uint16_t{});
    constexpr auto decode32   = genDecode(uint32_t{});
    constexpr auto decode64   = genDecode(uint64_t{});

    CHECK(decodeBool({0x00}) == false);
    CHECK(decodeBool({0x01}) == true);
    CHECK_THROWS(decodeBool({0xa2, 0x74}));
    CHECK_THROWS(decodeBool({})); // No input data.
    CHECK_THROWS(decodeBool({0xf0, 0xab})); // Input ends unexpectedly.
    CHECK_THROWS(decodeBool({0xf0, 0xab, 0xc9, 0x9a, 0xf8, 0xb2})); // Input ends unexpectedly after 32 bits.
    CHECK_THROWS(decodeBool({0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01})); // Input longer than 10 bytes.


    CHECK(decode8({0x00}) == 0u);
    CHECK(decode8({0x01}) == 1u);
    CHECK(decode8({0x7f}) == 127u);
    CHECK_THROWS(decode8({0xa2, 0x74}));
    CHECK_THROWS(decode8({})); // No input data.
    CHECK_THROWS(decode8({0xf0, 0xab})); // Input ends unexpectedly.
    CHECK_THROWS(decode8({0xf0, 0xab, 0xc9, 0x9a, 0xf8, 0xb2})); // Input ends unexpectedly after 32 bits.
    CHECK_THROWS(decode8({0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01})); // Input longer than 10 bytes.

    CHECK(decode16({0x00}) == 0u);
    CHECK(decode16({0x01}) == 1u);
    CHECK(decode16({0x7f}) == 127u);
    CHECK(decode16({0xa2, 0x74}) == 14882u);
    CHECK_THROWS(decode16({0xbe, 0xf7, 0x92, 0x84, 0x0b}));
    CHECK_THROWS(decode8({})); // No input data.
    CHECK_THROWS(decode8({0xf0, 0xab})); // Input ends unexpectedly.
    CHECK_THROWS(decode8({0xf0, 0xab, 0xc9, 0x9a, 0xf8, 0xb2})); // Input ends unexpectedly after 32 bits.
    CHECK_THROWS(decode8({0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01})); // Input longer than 10 bytes.

    CHECK(decode32({0x00}) == 0u);
    CHECK(decode32({0x01}) == 1u);
    CHECK(decode32({0x7f}) == 127u);
    CHECK(decode32({0xa2, 0x74}) == 14882u);
    CHECK(decode32({0xbe, 0xf7, 0x92, 0x84, 0x0b}) == 2961488830u);
    CHECK_THROWS(decode32({0xbe, 0xf7, 0x92, 0x84, 0x1b}));
    CHECK_THROWS(decode32({0x80, 0xe6, 0xeb, 0x9c, 0xc3, 0xc9, 0xa4, 0x49}));
    CHECK_THROWS(decode32({0x9b, 0xa8, 0xf9, 0xc2, 0xbb, 0xd6, 0x80, 0x85, 0xa6, 0x01}));
    CHECK_THROWS(decode8({})); // No input data.
    CHECK_THROWS(decode8({0xf0, 0xab})); // Input ends unexpectedly.
    CHECK_THROWS(decode8({0xf0, 0xab, 0xc9, 0x9a, 0xf8, 0xb2})); // Input ends unexpectedly after 32 bits.
    CHECK_THROWS(decode8({0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01})); // Input longer than 10 bytes.

    CHECK(decode64({0x00}) == 0u);
    CHECK(decode64({0x01}) == 1u);
    CHECK(decode64({0x7f}) == 127u);
    CHECK(decode64({0xa2, 0x74}) == 14882u);
    CHECK(decode64({0xbe, 0xf7, 0x92, 0x84, 0x0b}) == 2961488830u);
    CHECK(decode64({0xbe, 0xf7, 0x92, 0x84, 0x1b}) == 7256456126u);
    CHECK(decode64({0x80, 0xe6, 0xeb, 0x9c, 0xc3, 0xc9, 0xa4, 0x49}) == 41256202580718336u);
    CHECK(decode64({0x9b, 0xa8, 0xf9, 0xc2, 0xbb, 0xd6, 0x80, 0x85, 0xa6, 0x01}) == 11964378330978735131u);
    CHECK_THROWS(decode8({})); // No input data.
    CHECK_THROWS(decode8({0xf0, 0xab})); // Input ends unexpectedly.
    CHECK_THROWS(decode8({0xf0, 0xab, 0xc9, 0x9a, 0xf8, 0xb2})); // Input ends unexpectedly after 32 bits.
    CHECK_THROWS(decode8({0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01})); // Input longer than 10 bytes.

} // TEST_CASE("varint decoding")


TEST_CASE("varint encoding"){

    constexpr auto encode = [](auto v, std::vector<uint8_t> const& expected){
        std::vector<uint8_t> b;
        append_varint(b, v);
        return b == expected;
    };

    CHECK(encode(false, {0x00}));
    CHECK(encode(true , {0x01}));
    CHECK(encode(0u, {0x00}));
    CHECK(encode(1u, {0x01}));
    CHECK(encode(127u, {0x7f}));
    CHECK(encode(14882u, {0xa2, 0x74}));
    CHECK(encode(2961488830u, {0xbe, 0xf7, 0x92, 0x84, 0x0b}));
    CHECK(encode(7256456126u, {0xbe, 0xf7, 0x92, 0x84, 0x1b}));
    CHECK(encode(41256202580718336u, {0x80, 0xe6, 0xeb, 0x9c, 0xc3, 0xc9, 0xa4, 0x49}));
    CHECK(encode(11964378330978735131u, {0x9b, 0xa8, 0xf9, 0xc2, 0xbb, 0xd6, 0x80, 0x85, 0xa6, 0x01}));
} // TEST_CASE("varint encoding")


TEST_CASE("varint signed"){

    constexpr auto exam = [](auto v){
        std::vector<uint8_t> b;
        append_varint(b, v);

        if(v < 0 && b.size() != 10)
            throw std::runtime_error{"negative variant should always use 10 bytes"};

        if(b.size() > max_varint_wire_size<decltype(v)>)
            throw std::runtime_error{"unexpected wire size"};

        decltype(v) r = 0;
        auto e = read_varint(buf_begin(b), buf_end(b), r).value_or_throw();
        if(e != buf_end(b))
            throw std::runtime_error{"input not fully parsed"};

        return v == r;
    };

    CHECK(exam(0));
    CHECK(exam(1));
    CHECK(exam(-1));
    CHECK(exam(1237894));
    CHECK(exam(-37895138));
} // TEST_CASE("varint signed")


TEST_CASE("varint zigzag"){

    constexpr auto exam = [](auto v){

        auto z = static_cast<std::make_unsigned_t<decltype(v)>>(v < 0 ? (-2*v - 1) : 2*v);

        static_assert(std::is_same_v<decltype(zigzag_encode(v)), decltype(z)>);
        static_assert(std::is_same_v<decltype(zigzag_decode(z)), decltype(v)>);

        if(zigzag_encode(v) != z)
            return false;
        if(zigzag_decode(z) != v)
            return false;

        std::vector<uint8_t> b;
        append_varint<true>(b, v);

        if(b.size() > max_varint_wire_size<decltype(v), true>)
            throw std::runtime_error{"unexpected wire size"};

        decltype(v) r = 0;
        auto e = read_varint<true>(buf_begin(b), buf_end(b), r).value_or_throw();
        if(e != buf_end(b))
            throw std::runtime_error{"input not fully parsed"};

        return v == r;
    };

    CHECK(exam(-1));
    CHECK(exam(-1237894));
    CHECK(exam(-37895138));
    CHECK(exam(-14882));
    CHECK(exam(-2961488830));
    CHECK(exam(-7256456126));
    CHECK(exam(-41256202580718336));
    CHECK(exam(std::numeric_limits<int8_t >::min()));
    CHECK(exam(std::numeric_limits<int16_t>::min()));
    CHECK(exam(std::numeric_limits<int32_t>::min()));
    CHECK(exam(std::numeric_limits<int64_t>::min()));
} // TEST_CASE("varint zigzag")


TEST_CASE("fixed field"){

    struct fixed_sub_sub_msg_t
    {
        bool    boolMem = true;
        int32_t sfixed32Mem = -123456;
        float   floatArrMem[2] = {1.226f, 20.1f};
        constexpr bool operator==(fixed_sub_sub_msg_t const&) const = default;
    };

    constexpr auto fixed_sub_sub_msg = pb_message<"fixed_sub_sub_msg">(
        pb_bool             <"boolMem"    , 1>(   JKL_P_VAL(d.boolMem    )),
        pb_sfixed32         <"sfixed32Mem", 2>(   JKL_P_VAL(d.sfixed32Mem)),
        pb_repeated(pb_float<"floatArrMem", 3>(), JKL_P_VAL(d.floatArrMem), JKL_P_CLEAR_VAL(d.floatArrMem[0]={0.f}))
    );

    struct fixed_sub_msg_t
    {
        bool    fixed64ArrMem[2] = {true, true};
        int32_t sfixed32ArrMem[2] = {1291261, -215126};
        int64_t sfixed64ArrMem[2] = {-12323425123, 345389759345};
        float   floatArrMem[2] = {122.325432f, -124234.213f};
        double  doubleArrMem[2] = {-13209386.9832, 124651579.12313};
        float   floatMem = 1456465.123123613f;
        double  doubleMem = 16545684341242.135463021;
        fixed_sub_sub_msg_t subSubMsgMem;
        fixed_sub_sub_msg_t subSubMsgArrMem[6];
        constexpr bool operator==(fixed_sub_msg_t const&) const = default;
    };

    constexpr auto fixed_sub_msg = pb_message<"fixed_sub_msg">(
        pb_repeated(pb_fixed64 <"fixed64ArrMem" , 1>(), JKL_P_VAL(d.fixed64ArrMem), JKL_P_CLEAR_VAL(d.fixed64ArrMem[0]=false)),
        pb_repeated(pb_sfixed32<"sfixed32ArrMem", 2>(), JKL_P_VAL(d.sfixed32ArrMem), JKL_P_CLEAR_VAL(d.sfixed32ArrMem[0]=0)),
        pb_repeated(pb_sfixed64<"sfixed64ArrMem", 3>(), JKL_P_VAL(d.sfixed64ArrMem), JKL_P_CLEAR_VAL(d.sfixed64ArrMem[0]=0)),
        pb_repeated(pb_float   <"floatArrMem"   , 4>(), JKL_P_VAL(d.floatArrMem), JKL_P_CLEAR_VAL(d.floatArrMem[0]=0.f)),
        pb_repeated(pb_double  <"doubleArrMem"  , 5>(), JKL_P_VAL(d.doubleArrMem), JKL_P_CLEAR_VAL(d.doubleArrMem[0]=0)),
        pb_float               <"floatMem"      , 6>(   JKL_P_VAL(d.floatMem)),
        pb_double              <"doubleMem"     , 7>(   JKL_P_VAL(d.doubleMem)),
        fixed_sub_sub_msg._    <"subSubMsgMem"  , 8>(   JKL_P_VAL(d.subSubMsgMem)),
        pb_repeated(fixed_sub_sub_msg._<"subSubMsgArrMem", 9>(), JKL_P_VAL(d.subSubMsgArrMem), JKL_P_CLEAR_VAL(d.subSubMsgArrMem[0].boolMem=false))
    );

    struct fixed_msg_t{
        bool            boolMem = true;
        int32_t         sfixed32Mem = -1239045;
        int64_t         sfixed64Mem = -64894313213123;
        uint32_t        fixed32Mem = 1312231654;
        uint64_t        fixed64Mem = 4651321654613156;
        float           floatMem = 468696332.12323f;
        double          doubleMem = 4651321.09789687576;
        bool            boolArrMem[6] = {true, false, true, false, true, false};
        uint32_t        fixed32ArrMem[6] = {1, 2, 3, 4, 5, 6};
        fixed_sub_msg_t subMsgMem;
        fixed_sub_msg_t subMsgArrMem[6];
        constexpr bool operator==(fixed_msg_t const&) const = default;
    };

    constexpr auto fixed_msg = pb_message<"fixed_msg">(
        pb_bool               <"boolMem"      , 1 >(   JKL_P_VAL(d.boolMem      )),
        pb_sfixed32           <"sfixed32Mem"  , 2 >(   JKL_P_VAL(d.sfixed32Mem  )),
        pb_sfixed64           <"sfixed64Mem"  , 3 >(   JKL_P_VAL(d.sfixed64Mem  )),
        pb_fixed32            <"fixed32Mem"   , 4 >(   JKL_P_VAL(d.fixed32Mem   )),
        pb_fixed64            <"fixed64Mem"   , 5 >(   JKL_P_VAL(d.fixed64Mem   )),
        pb_float              <"floatMem"     , 6 >(   JKL_P_VAL(d.floatMem     )),
        pb_double             <"doubleMem"    , 7 >(   JKL_P_VAL(d.doubleMem    )),
        pb_repeated(pb_bool   <"boolArrMem"   , 8 >(), JKL_P_VAL(d.boolArrMem   ), JKL_P_CLEAR_VAL(d.boolArrMem[0]=false)),
        pb_repeated(pb_fixed32<"fixed32ArrMem", 9 >(), JKL_P_VAL(d.fixed32ArrMem), JKL_P_CLEAR_VAL(d.fixed32ArrMem[0]=0)),
        fixed_sub_msg._       <"subMsgMem"    , 10>(   JKL_P_VAL(d.subMsgMem    )),
        pb_repeated(fixed_sub_msg._<"subMsgArrMem", 11>(), JKL_P_VAL(d.subMsgArrMem), JKL_P_CLEAR_VAL(d.subMsgArrMem[0].fixed64ArrMem[0]=0))
    );

    static_assert(JKL_CEVL(fixed_msg.is_static_len(fixed_msg_t{})));
    static_assert(JKL_CEVL(fixed_msg.len_cache_cnt(fixed_msg_t{})) == 0);
//     static_assert(JKL_CEVL((fixed_msg.wire_size<true, true>(fixed_msg_t{}, std::declval<pb_size_t*&>()))) == );
    static_assert(_integral_constant_<decltype(fixed_msg.is_static_len(fixed_msg_t{}))>);
    static_assert(_integral_constant_<decltype(fixed_msg.len_cache_cnt(fixed_msg_t{}))>);
    static_assert(_integral_constant_<decltype((fixed_msg.wire_size<true, true>(fixed_msg_t{}, std::declval<pb_size_t*&>())))>);

    std::string buf;
    CHECK_NOTHROW(fixed_msg.write(buf, fixed_msg_t{}));
    fixed_msg_t readFixedMsg;
    std::memset(&readFixedMsg, 0, sizeof(readFixedMsg));
    CHECK_NOTHROW(fixed_msg.full_read(buf, readFixedMsg).throw_on_error());

    CHECK(readFixedMsg == fixed_msg_t{});

    auto def = pb_gen_def(fixed_sub_sub_msg, fixed_sub_msg, fixed_msg);
} // TEST_CASE("fixed field")

TEST_CASE("optional field"){
    struct sub_msg_t
    {
        bool                    defaultBoolMem = true;
        std::optional<int32_t>  optInt32Mem = -32156411;
        int64_t                 defaultInt64Mem = 4613246761313;
        constexpr bool operator==(sub_msg_t const&) const = default;
        static sub_msg_t diff_sample()
        {
            return {false, 1, 2};
        }
    };

    constexpr auto sub_msg = pb_message<"sub_msg">(
        pb_bool  <"defaultBoolMem"  , 1>(JKL_P_VAL(d.defaultBoolMem), p_default(false)),
        pb_int32 <"optInt32Mem"     , 2>(JKL_P_VAL(d.optInt32Mem), p_optional),
        pb_int64 <"defaultInt64Mem" , 3>(JKL_P_VAL(d.defaultInt64Mem), p_default(4613246761313))
    );

    struct msg_t
    {
        // pb_varint_fld
        bool                    defaultBoolMem = true;
        std::optional<int32_t>  optInt32Mem = -32156411;
        int64_t                 defaultInt64Mem = 4613246761313;
        uint32_t                uint32Mem = 6878081;
        std::optional<uint64_t> optUint64Mem;
        int32_t                 defaultSint32Mem = -613123123;
        std::optional<int64_t>  optSint64Mem;
        // pb_fixed_fld
        int32_t                 sfixed32Mem = -1239045;
        std::optional<int64_t>  optSfixed64Mem = -64894313213123;
        std::optional<uint32_t> optFixed32Mem;
        uint64_t                defaultFixed64Mem = 4651321654613156;
        std::optional<float>    optFloatMem;
        std::optional<double>   optDoubleMem = 4651321.09789687576;
        // pb_bytes_fld
        std::optional<string>   optStringMem;
        std::optional<std::vector<char>> optDynaBytesMem = std::vector<char>{1, 2, 3, 4, 5, 6};
        string                  optStringMem2 = "other thing";
        std::vector<char>       optDynaBytesMem2;
        char                    fixedBytesMem[6] = {1, 2, 3, 4, 5, 6};
        sub_msg_t                subMsgMem;
        std::optional<sub_msg_t> optSubMsgMem;
        bool operator==(msg_t const&) const = default;
        static msg_t diff_sample()
        {
            msg_t m;
            m.defaultBoolMem = false;
            m.optInt32Mem = std::nullopt;
            m.defaultInt64Mem = 0;
            m.uint32Mem = 1212;
            m.optUint64Mem = 62;
            m.defaultSint32Mem = 162;
            m.optSint64Mem = 926;
            m.sfixed32Mem = 169;
            m.optSfixed64Mem = -6;
            m.optFixed32Mem = 226;
            m.defaultFixed64Mem = 166;
            m.optFloatMem = 1.26f;
            m.optDoubleMem = std::nullopt;
            // pb_bytes_fld
            m.optStringMem = "something";
            m.optDynaBytesMem = std::vector<char>{6, 5, 4, 3, 2, 1};
            m.optStringMem2 = "something2";
            m.optDynaBytesMem2 = std::vector<char>{6, 6, 6, 6, 2, 2};
            memset(m.fixedBytesMem, 0, sizeof(m.fixedBytesMem));
            m.subMsgMem = sub_msg_t::diff_sample();
            m.optSubMsgMem = sub_msg_t::diff_sample();
            return m;
        }
    };

    constexpr auto msg = pb_message<"msg">(
        pb_bool  <"defaultBoolMem"  , 21>(JKL_P_VAL(d.defaultBoolMem), p_default(false)),
        pb_int32 <"optInt32Mem"     , 1>(JKL_P_VAL(d.optInt32Mem), p_optional),
        pb_int64 <"defaultInt64Mem" , 2>(JKL_P_VAL(d.defaultInt64Mem), p_default(4613246761313)),
        pb_uint32<"uint32Mem"       , 3>(JKL_P_VAL(d.uint32Mem), JKL_P_HAS_VAL(d.uint32Mem != 0), JKL_P_CLEAR_VAL(d.uint32Mem = 0)),
        pb_uint64<"optUint64Mem"    , 4>(JKL_P_VAL(d.optUint64Mem), p_optional),
        pb_sint32<"defaultSint32Mem", 5>(JKL_P_VAL(d.defaultSint32Mem), p_default(-613123123)),
        pb_sint64<"optSint64Mem"    , 6>(JKL_P_VAL(d.optSint64Mem), p_optional),

        pb_sfixed32<"sfixed32Mem"      , 7 >(JKL_P_VAL(d.sfixed32Mem), JKL_P_HAS_VAL(d.sfixed32Mem != -1239045), JKL_P_CLEAR_VAL(d.sfixed32Mem = -1239045)),
        pb_sfixed64<"optSfixed64Mem"   , 8 >(JKL_P_VAL(d.optSfixed64Mem), p_optional),
        pb_fixed32 <"optFixed32Mem"    , 9 >(JKL_P_VAL(d.optFixed32Mem), p_optional),
        pb_fixed64 <"defaultFixed64Mem", 10>(JKL_P_VAL(d.defaultFixed64Mem), p_default(0u)),
        pb_float   <"optFloatMem"      , 11>(JKL_P_VAL(d.optFloatMem), p_optional),
        pb_double  <"optDoubleMem"     , 12>(JKL_P_VAL(d.optDoubleMem), p_optional),
        pb_string<"optStringMem"       , 15>(JKL_P_VAL(d.optStringMem), p_optional),
        pb_bytes <"optDynaBytesMem"    , 16>(JKL_P_VAL(d.optDynaBytesMem), p_optional),
        pb_string<"optStringMem2"      , 66>(JKL_P_VAL(d.optStringMem2), p_optional),
        pb_bytes <"optDynaBytesMem2"   , 96>(JKL_P_VAL(d.optDynaBytesMem2), p_optional),
        pb_bytes <"fixedBytesMem"      , 19>(JKL_P_VAL(d.fixedBytesMem), JKL_P_HAS_VAL(d.fixedBytesMem[0] != 0), JKL_P_CLEAR_VAL(d.fixedBytesMem[0] = 0)),
        sub_msg._<"subMsgMem"          , 25>(JKL_P_VAL(d.subMsgMem)),
        sub_msg._<"optSubMsgMem"       , 26>(JKL_P_VAL(d.optSubMsgMem), p_optional)
    );

    std::string buf0;
    msg_t m;
    CHECK_NOTHROW(msg.write(buf0, m));
    CHECK(msg_t{} == m);

    m.defaultFixed64Mem = 0;
    std::string buf;
    CHECK_NOTHROW(msg.write(buf, m));

    CHECK(buf0.size() == buf.size() + 9);

    msg_t readMsg = msg_t::diff_sample();
    CHECK_NOTHROW(msg.full_read(buf, readMsg).throw_on_error());
    CHECK(readMsg == m);

    auto def = pb_gen_def(sub_msg, msg);
} // TEST_CASE("optional field")

TEST_CASE("map field"){
    struct sub_msg_t
    {
        int64_t                 defaultInt64Mem = 4613246761313;
        std::map<int32_t, string> mapMem = {{1, "1"}, {2, "2"}};
        bool operator==(sub_msg_t const&) const = default;
        static sub_msg_t diff_sample()
        {
            return {1, {{6, "6"}, {9, "9"}}};
        }
    };

    constexpr auto sub_msg = pb_message<"sub_msg">(
        pb_int64<"defaultInt64Mem" , 1>(JKL_P_VAL(d.defaultInt64Mem), p_default(4613246761313)),
        pb_map  <"mapMem",           2>(pb_int32(), pb_string(), JKL_P_VAL(d.mapMem))
    );

    struct msg_t
    {
        std::set<int32_t> setMem1 = {1, 2};
        std::set<string> setMem2 = {"1", "2"};

        std::map<int32_t, string> mapMem1 = {{1, "1"}, {2, "2"}};
        std::vector<std::pair<int32_t, string>> vecMem1 = {{1, "1"}, {2, "2"}};
        std::unordered_map<int32_t, string> unMapMem1 = {{1, "1"}, {2, "2"}};

        std::map<string, string> mapMem2 = {{"1", "1"}, {"2", "2"}};
        std::vector<std::pair<string, string>> vecMem2 = {{"1", "1"}, {"2", "2"}};
        std::unordered_map<string, string> unMapMem2 = {{"1", "1"}, {"2", "2"}};

        std::map<string, sub_msg_t> mapMem3 = {{"1", {1}}, {"2", {2}}};
        std::vector<std::pair<string, sub_msg_t>> vecMem3 = {{"1", {1}}, {"2", {2}}};
        std::unordered_map<string, sub_msg_t> unMapMem3 = {{"1", {1}}, {"2", {2}}};

        bool operator==(msg_t const&) const = default;

        static msg_t diff_sample()
        {
            return {
                {6, 9}, {"6", "9"},
                {{6, "6"}, {9, "9"}},
                {{6, "6"}, {9, "9"}},
                {{6, "6"}, {9, "9"}},
                {{"6", "6"}, {"9", "9"}},
                {{"6", "6"}, {"9", "9"}},
                {{"6", "6"}, {"9", "9"}},
                {{"6", sub_msg_t::diff_sample()}, {"9", sub_msg_t::diff_sample()}},
                {{"6", sub_msg_t::diff_sample()}, {"9", sub_msg_t::diff_sample()}},
                {{"6", sub_msg_t::diff_sample()}, {"9", sub_msg_t::diff_sample()}}
            };
        }
    };

    constexpr auto msg = pb_message<"msg">(
        pb_repeated(pb_sfixed32<"setMem1", 10>(), JKL_P_VAL(d.setMem1)),
        pb_repeated(pb_string<"setMem2", 11>(), JKL_P_VAL(d.setMem2)),

        pb_map(pb_int32(), pb_string())._<"mapMem1", 1>(JKL_P_VAL(d.mapMem1)),
        pb_map<"vecMem1", 2>(pb_uint32(), pb_string(), JKL_P_VAL(d.vecMem1)),
        pb_map<"unMapMem1", 3>(pb_sint32(), pb_string(), JKL_P_VAL(d.unMapMem1)),

        pb_map<"mapMem2", 4>(pb_string(), pb_string(), JKL_P_VAL(d.mapMem2)),
        pb_map(pb_string(), pb_string())._<"vecMem2", 5>(JKL_P_VAL(d.vecMem2)),
        pb_map<"unMapMem2", 6>(pb_string(), pb_string(), JKL_P_VAL(d.unMapMem2)),

        pb_map<"mapMem3", 7>(pb_string(), sub_msg, JKL_P_VAL(d.mapMem3)),
        pb_map<"vecMem3", 8>(pb_string(), sub_msg, JKL_P_VAL(d.vecMem3)),
        pb_map<"unMapMem3", 9>(pb_string(), sub_msg, JKL_P_VAL(d.unMapMem3))
    );

    msg_t m;
    std::string buf;
    CHECK_NOTHROW(msg.write(buf, m));
    CHECK(msg_t{} == m);

    msg_t readMsg = msg_t::diff_sample();
    CHECK_NOTHROW(msg.full_read(buf, readMsg).throw_on_error());
    CHECK(readMsg == m);

    auto def = pb_gen_def(sub_msg, msg);
} // TEST_CASE("map field")

TEST_CASE("oneof field"){
    struct sub_msg_t
    {
        int64_t defaultInt64Mem = 4613246761313;
        std::variant<std::monostate, int32_t, string, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t> oneofMem = "1";
        constexpr bool operator==(sub_msg_t const&) const = default;
        static sub_msg_t diff_sample()
        {
            return {266, std::monostate{}};
        }
    };

    constexpr auto sub_msg = pb_message<"sub_msg">(
        pb_int64<"defaultInt64Mem" , 1>(JKL_P_VAL(d.defaultInt64Mem), p_default(4613246761313)),
        pb_oneof<"oneofMem">(
            pb_int32 <"oneofMemInt", 2>(),
            pb_string<"oneofMemStr", 3>(),
            pb_int32   <"a"        , 4>(),
            pb_sint32  <"b"        , 5>(),
            pb_sfixed32<"c"        , 6>(),
            pb_int32   <"d"        , 7>(),
            pb_sint32  <"e"        , 8>(),
            pb_int32   <"f"        , 9>()
        )(JKL_P_VAL(d.oneofMem))
    );

    struct msg_t
    {
        sub_msg_t subMsgMem;
        size_t idx = 1;
        union{
            int32_t oneofMemInt = 1;
            string oneofMemStr;
        };
        msg_t(){}
        ~msg_t()
        {
            if(idx == 2)
                oneofMemStr.~string();
        }
        msg_t(msg_t const& r)
            : subMsgMem{r.subMsgMem}, idx{r.idx}
        {
            if(idx == 1)
                oneofMemInt = r.oneofMemInt;
            else if(idx == 2)
                new (&oneofMemStr) string{r.oneofMemStr};
        }

        constexpr bool operator==(msg_t const& r) const// = default;
        {
            if(subMsgMem == r.subMsgMem)
            {
                if(idx == r.idx)
                {
                    if(idx == 1)
                        return oneofMemInt == r.oneofMemInt;
                    if(idx == 2)
                        return oneofMemStr == r.oneofMemStr;
                }
            }
            return false;
        }

        static msg_t diff_sample()
        {
            msg_t m;
            m.subMsgMem = sub_msg_t::diff_sample();
            m.idx = 0;
            m.oneofMemInt = 0;
            return m;
        }
    };

    constexpr auto msg = pb_message<"msg">(
        sub_msg._<"subMsgMem", 1>(JKL_P_VAL(d.subMsgMem)),
        pb_oneof<"oneofMem">(
            pb_int32   <"oneofMemInt", 2>(),
            pb_string  <"oneofMemStr", 3>()
        )(
            p_get_member([](auto& d, auto I) -> auto& {
                static_assert(0u < I && I < 9u); // 0 for no member
                BOOST_ASSERT(d.idx == I);
                if constexpr(I == 1u) return d.oneofMemInt;
                else return d.oneofMemStr;
            }),
            p_activate_member([](auto& d, auto I){
                static_assert(0u <= I && I < 9u);
                if constexpr(I == 2u)
                {
                    if(d.idx != 2)
                        new (&d.oneofMemStr) string{};
                }
                else
                {
                    if(d.idx == 2)
                        d.oneofMemStr.~string();
                }
                d.idx = I;
            }),
            // p_active_member_idx([](auto& d) { return d.idx; })
            JKL_P_ACTIVE_MEMBER_IDX(d.idx)
        )
    );

    msg_t m;
    std::string buf;
    CHECK_NOTHROW(msg.write(buf, m));
    CHECK(msg_t{} == m);

    msg_t readMsg = msg_t::diff_sample();
    CHECK_NOTHROW(msg.full_read(buf, readMsg).throw_on_error());
    CHECK(readMsg == m);

    auto def = pb_gen_def(sub_msg, msg);
} // TEST_CASE("oneof field")

} // TEST_SUITE("pb")
