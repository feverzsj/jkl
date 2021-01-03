#pragma once

// #define _JKL_PB_USE_RUNTIME_TAG_CODEC
#include <jkl/util/log.hpp>
#include <jkl/pb/varint.hpp>
#include <jkl/pb/dsl.hpp>
#include <nanobench.h>
#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>

#include <google/protobuf/io/coded_stream.h>
#include "benchmark_message1_proto2.pb.h"

// #include <simdjson.h>


TEST_SUITE("pb benchmark"){

using namespace jkl;
namespace nanobench = ankerl::nanobench;


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr bool exam_utf8(_str_ auto const& s)
{
    return google::protobuf::internal::IsStructurallyValidUTF8(str_data(s), str_size(s));
    //return simdjson::validate_utf8(str_data(s), str_size(s));
}


TEST_CASE("varint encode benchmark"){

    system("pause");
    
    nanobench::Bench b;
    b.title("varint encode")
        .relative(true)
        .warmup(10000000)
        .minEpochIterations(10000000)
        ;

    uint8_t buf[max_varint_wire_size<uint64_t>] = {};
    
    b.run("google uint32_t", [&]{
        nanobench::doNotOptimizeAway(
            google::protobuf::io::CodedOutputStream::WriteVarint32ToArray(static_cast<uint32_t>(2961488830), buf));
    });

    b.run("google uint64_t", [&]{
        nanobench::doNotOptimizeAway(
            google::protobuf::io::CodedOutputStream::WriteVarint64ToArray(static_cast<uint64_t>(-41256202580718336), buf));
    });

    b.run("jkl uint32_t", [&]{
        nanobench::doNotOptimizeAway(
            append_varint(buf, static_cast<uint32_t>(2961488830)));
    });

    b.run("jkl uint64_t", [&]{
        nanobench::doNotOptimizeAway(
            append_varint(buf, static_cast<uint64_t>(-41256202580718336)));
    });

//     std::ofstream f{"varint encode benchmark.html"};
//     b.render(nanobench::templates::htmlBoxplot(), f);

} // TEST_CASE("varint encode benchmark")


TEST_CASE("varint decode benchmark"){

    nanobench::Bench b;
    b.title("varint decode")
        .relative(true)
        .warmup(10000000)
        .minEpochIterations(10000000)
        ;

    std::initializer_list<uint8_t> buf32 = {0xbe, 0xf7, 0x92, 0x84, 0x0b};
    std::initializer_list<uint8_t> buf64 = {0x9b, 0xa8, 0xf9, 0xc2, 0xbb, 0xd6, 0x80, 0x85, 0xa6, 0x01};

    b.run("google uint32_t", [&]{
        uint32_t v;
        nanobench::doNotOptimizeAway(
            google::protobuf::io::CodedInputStream{buf32.begin(), (int)buf32.size()}.ReadVarint32(&v));
    });

    b.run("google uint64_t", [&]{
        uint64_t v;
        nanobench::doNotOptimizeAway(
            google::protobuf::io::CodedInputStream{buf64.begin(), (int)buf64.size()}.ReadVarint64(&v));
    });

    b.run("jkl uint32_t", [&]{
        uint32_t v;
        nanobench::doNotOptimizeAway(
            read_varint(buf32.begin(), buf32.end(), v));
    });

    b.run("jkl uint64_t", [&]{
        uint64_t v;
        nanobench::doNotOptimizeAway(
            read_varint(buf64.begin(), buf64.end(), v));
    });

//     std::ofstream f{"varint decode benchmark.html"};
//     b.render(nanobench::templates::htmlBoxplot(), f);

} // TEST_CASE("varint decode benchmark")


TEST_CASE("GoogleMessage1"){

    constexpr uint8_t data[] = {
        0x0A, 0x00, 0x10, 0x08, 0x18, 0xCB, 0x8F, 0x7E, 0x22, 0x06, 0x33, 0x4B, 0x2B, 0x36, 0x29, 0x23,
        0x4A, 0x59, 0x31, 0x30, 0x29, 0x32, 0x75, 0x69, 0x53, 0x75, 0x6F, 0x58, 0x4C, 0x31, 0x5E, 0x29,
        0x76, 0x7D, 0x69, 0x63, 0x46, 0x40, 0x3E, 0x50, 0x28, 0x6A, 0x3C, 0x74, 0x23, 0x7E, 0x74, 0x7A,
        0x5C, 0x6C, 0x67, 0x3F, 0x3F, 0x53, 0x26, 0x28, 0x3C, 0x68, 0x72, 0x37, 0x45, 0x56, 0x73, 0x27,
        0x6C, 0x7B, 0x27, 0x35, 0x60, 0x47, 0x6F, 0x68, 0x63, 0x5F, 0x28, 0x3D, 0x74, 0x20, 0x65, 0x53,
        0x20, 0x73, 0x7B, 0x5F, 0x49, 0x3F, 0x69, 0x43, 0x77, 0x61, 0x47, 0x5D, 0x4C, 0x27, 0x2A, 0x50,
        0x75, 0x35, 0x28, 0x26, 0x77, 0x5F, 0x3A, 0x34, 0x7B, 0x7E, 0x5A, 0x60, 0x01, 0x68, 0x00, 0x70,
        0x01, 0x7A, 0x59, 0x08, 0x19, 0x10, 0x24, 0x7A, 0x43, 0x22, 0x3F, 0x36, 0x50, 0x59, 0x34, 0x5D,
        0x4C, 0x32, 0x63, 0x3C, 0x7D, 0x7E, 0x32, 0x3B, 0x5C, 0x54, 0x56, 0x46, 0x5F, 0x77, 0x5E, 0x5B,
        0x40, 0x59, 0x66, 0x62, 0x49, 0x63, 0x2A, 0x76, 0x2F, 0x4E, 0x2B, 0x5A, 0x2D, 0x6F, 0x59, 0x75,
        0x61, 0x57, 0x5A, 0x72, 0x34, 0x43, 0x3B, 0x35, 0x69, 0x62, 0x7C, 0x2A, 0x73, 0x40, 0x52, 0x43,
        0x42, 0x62, 0x75, 0x76, 0x72, 0x51, 0x33, 0x67, 0x28, 0x6B, 0x2C, 0x4E, 0xA9, 0x01, 0x54, 0xFF,
        0x43, 0x08, 0xDE, 0x1A, 0x0A, 0x27, 0xB0, 0x01, 0x26, 0xB8, 0x01, 0x01, 0x88, 0x01, 0x00, 0x92,
        0x01, 0x0A, 0x7B, 0x3D, 0x51, 0x77, 0x66, 0x65, 0x7E, 0x23, 0x6E, 0x7B, 0x98, 0x04, 0x88, 0x91,
        0x61, 0xA0, 0x06, 0x1F
    };

    struct GoogleMessage1SubMessage_t
    {
        constexpr auto operator<=>(GoogleMessage1SubMessage_t const&) const = default;

        int32_t field1 = 0;
        int32_t field2 = 0;
        int32_t field3 = 0;
        string field15;
        bool field12 = true;
        std::optional<int64_t> field13;
        std::optional<int64_t> field14;
        std::optional<int32_t> field16;
        int32_t field19 = 2;
        bool field20 = true;
        bool field28 = true;
        std::optional<uint64_t> field21;
        std::optional<int32_t > field22;
        bool field23  = false;
        bool field206 = false;
        std::optional<uint32_t> field203;
        std::optional<int32_t > field204;
        string field205;
        std::optional<uint64_t> field207;
        std::optional<uint64_t> field300;
    };

    constexpr auto GoogleMessage1SubMessage = pb_message<"GoogleMessage1SubMessage">(
        pb_int32  <"field1"  , 1  >(JKL_P_VAL(d.field1  ), p_default(0)),
        pb_int32  <"field2"  , 2  >(JKL_P_VAL(d.field2  ), p_default(0)),
        pb_int32  <"field3"  , 3  >(JKL_P_VAL(d.field3  ), p_default(0)),
        pb_string <"field15" , 15 >(JKL_P_VAL(d.field15 ), p_optional, JKL_P_VALIDATE(exam_utf8(d.field15))),
        pb_bool   <"field12" , 12 >(JKL_P_VAL(d.field12 ), p_default(true)),
        pb_int64  <"field13" , 13 >(JKL_P_VAL(d.field13 ), p_optional),
        pb_int64  <"field14" , 14 >(JKL_P_VAL(d.field14 ), p_optional),
        pb_int32  <"field16" , 16 >(JKL_P_VAL(d.field16 ), p_optional),
        pb_int32  <"field19" , 19 >(JKL_P_VAL(d.field19 ), p_default(2)),
        pb_bool   <"field20" , 20 >(JKL_P_VAL(d.field20 ), p_default(true)),
        pb_bool   <"field28" , 28 >(JKL_P_VAL(d.field28 ), p_default(true)),
        pb_fixed64<"field21" , 21 >(JKL_P_VAL(d.field21 ), p_optional),
        pb_int32  <"field22" , 22 >(JKL_P_VAL(d.field22 ), p_optional),
        pb_bool   <"field23" , 23 >(JKL_P_VAL(d.field23 ), p_default(false)),
        pb_bool   <"field206", 206>(JKL_P_VAL(d.field206), p_default(false)),
        pb_fixed32<"field203", 203>(JKL_P_VAL(d.field203), p_optional),
        pb_int32  <"field204", 204>(JKL_P_VAL(d.field204), p_optional),
        pb_string <"field205", 205>(JKL_P_VAL(d.field205), p_optional, JKL_P_VALIDATE(exam_utf8(d.field205))),
        pb_uint64 <"field207", 207>(JKL_P_VAL(d.field207), p_optional),
        pb_uint64 <"field300", 300>(JKL_P_VAL(d.field300), p_optional)
    );

    struct GoogleMessage1_t
    {
        constexpr auto operator<=>(GoogleMessage1_t const&) const noexcept = default;

        string field1;
        string field9;
        string field18;
        bool field80 = false;
        bool field81 = true;
        int32_t field2;
        int32_t field3;
        std::optional<int32_t> field280;
        int32_t field6 = 0;
        std::optional<int64_t> field22;
        string field4;
        std::vector<uint64_t> field5;
        bool field59 = false;
        string field7;
        std::optional<int32_t> field16;
        int32_t field130 = 0;
        bool field12 = true;
        bool field17 = true;
        bool field13 = true;
        bool field14 = true;
        int32_t field104 = 0;
        int32_t field100 = 0;
        int32_t field101 = 0;
        string field102;
        string field103;
        int32_t field29 = 0;
        bool    field30 = false;
        int32_t field60 = -1;
        int32_t field271 = -1;
        int32_t field272 = -1;
        std::optional<int32_t> field150;
        int32_t field23 = 0;
        bool    field24 = false;
        int32_t field25 = 0;
        std::optional<GoogleMessage1SubMessage_t> field15;
        std::optional<bool> field78;
        int32_t field67 = 0;
        std::optional<int32_t> field68;
        int32_t field128 = 0;
        string  field129 = "xxxxxxxxxxxxxxxxxxxxx";
        int32_t field131 = 0;
    };

    constexpr auto GoogleMessage1 = pb_message<"GoogleMessage1">(
        pb_string  <"field1"  , 1  >(JKL_P_VAL(d.field1  ), JKL_P_VALIDATE(exam_utf8(d.field1))), 
        pb_string  <"field9"  , 9  >(JKL_P_VAL(d.field9  ), p_optional, JKL_P_VALIDATE(exam_utf8(d.field9))), 
        pb_string  <"field18" , 18 >(JKL_P_VAL(d.field18 ), p_optional, JKL_P_VALIDATE(exam_utf8(d.field18))), 
        pb_bool    <"field80" , 80 >(JKL_P_VAL(d.field80 ), p_default(false)),
        pb_bool    <"field81" , 81 >(JKL_P_VAL(d.field81 ), p_default(true)),
        pb_int32   <"field2"  , 2  >(JKL_P_VAL(d.field2  )), 
        pb_int32   <"field3"  , 3  >(JKL_P_VAL(d.field3  )), 
        pb_int32   <"field280", 280>(JKL_P_VAL(d.field280), p_optional), 
        pb_int32   <"field6"  , 6  >(JKL_P_VAL(d.field6  ), p_default(0)),
        pb_int64   <"field22" , 22 >(JKL_P_VAL(d.field22 ), p_optional), 
        pb_string  <"field4"  , 4  >(JKL_P_VAL(d.field4  ), p_optional, JKL_P_VALIDATE(exam_utf8(d.field4))), 
        pb_repeated(pb_fixed64<"field5", 5>(), JKL_P_VAL(d.field5)), 
        pb_bool    <"field59" , 59 >(JKL_P_VAL(d.field59 ), p_default(false)),
        pb_string  <"field7"  , 7  >(JKL_P_VAL(d.field7  ), p_optional, JKL_P_VALIDATE(exam_utf8(d.field7))), 
        pb_int32   <"field16" , 16 >(JKL_P_VAL(d.field16 ), p_optional), 
        pb_int32   <"field130", 130>(JKL_P_VAL(d.field130), p_default(0)),
        pb_bool    <"field12" , 12 >(JKL_P_VAL(d.field12 ), p_default(true)),
        pb_bool    <"field17" , 17 >(JKL_P_VAL(d.field17 ), p_default(true)),
        pb_bool    <"field13" , 13 >(JKL_P_VAL(d.field13 ), p_default(true)),
        pb_bool    <"field14" , 14 >(JKL_P_VAL(d.field14 ), p_default(true)),
        pb_int32   <"field104", 104>(JKL_P_VAL(d.field104), p_default(0)),
        pb_int32   <"field100", 100>(JKL_P_VAL(d.field100), p_default(0)),
        pb_int32   <"field101", 101>(JKL_P_VAL(d.field101), p_default(0)),
        pb_string  <"field102", 102>(JKL_P_VAL(d.field102), p_optional, JKL_P_VALIDATE(exam_utf8(d.field102))), 
        pb_string  <"field103", 103>(JKL_P_VAL(d.field103), p_optional, JKL_P_VALIDATE(exam_utf8(d.field103))), 
        pb_int32   <"field29" , 29 >(JKL_P_VAL(d.field29 ), p_default(0)),
        pb_bool    <"field30" , 30 >(JKL_P_VAL(d.field30 ), p_default(false)),
        pb_int32   <"field60" , 60 >(JKL_P_VAL(d.field60 ), p_default(-1)),
        pb_int32   <"field271", 271>(JKL_P_VAL(d.field271), p_default(-1)),
        pb_int32   <"field272", 272>(JKL_P_VAL(d.field272), p_default(-1)),
        pb_int32   <"field150", 150>(JKL_P_VAL(d.field150), p_optional), 
        pb_int32   <"field23" , 23 >(JKL_P_VAL(d.field23 ), p_default(0)),
        pb_bool    <"field24" , 24 >(JKL_P_VAL(d.field24 ), p_default(false)),
        pb_int32   <"field25" , 25 >(JKL_P_VAL(d.field25 ), p_default(0)),
        GoogleMessage1SubMessage._<"field15", 15>(JKL_P_VAL(d.field15), p_optional),
        pb_bool    <"field78" , 78 >(JKL_P_VAL(d.field78 ), p_optional),
        pb_int32   <"field67" , 67 >(JKL_P_VAL(d.field67 ), p_default(0)),
        pb_int32   <"field68" , 68 >(JKL_P_VAL(d.field68 ), p_optional),
        pb_int32   <"field128", 128>(JKL_P_VAL(d.field128), p_default(0)),
        pb_string  <"field129", 129>(JKL_P_VAL(d.field129), p_default(string_view("xxxxxxxxxxxxxxxxxxxxx")), JKL_P_VALIDATE(exam_utf8(d.field129))),
        pb_int32   <"field131", 131>(JKL_P_VAL(d.field131), p_default(0))
    );

    string buf;
    buf.reserve(sizeof(data));

    benchmarks::proto2::GoogleMessage1 gm;
    CHECK(gm.ParseFromArray(data, sizeof(data)));
    CHECK(gm.SerializeToString(&buf));

    GoogleMessage1_t m;
    CHECK(GoogleMessage1.full_read(data, m));
    CHECK_NOTHROW(GoogleMessage1.write(buf, m));


    {
        nanobench::Bench b;
        b.title("GoogleMessage1 write")
            .relative(true)
            .warmup(100000)
            .minEpochIterations(100000)
            ;

        b.run("google.protobuf", [&]{
            nanobench::doNotOptimizeAway(gm.SerializeToString(&buf));
        });
        
        b.run("jkl.pb", [&]{
            GoogleMessage1.write(buf, m);
            nanobench::doNotOptimizeAway(m);
        });
    }

    {
        nanobench::Bench b;
        b.title("GoogleMessage1 read")
            .relative(true)
            .warmup(100000)
            .minEpochIterations(100000)
            ;

        b.run("google.protobuf", [&]{
            nanobench::doNotOptimizeAway(gm.ParseFromArray(data, sizeof(data)));
        });

        b.run("jkl.pb", [&]{
            nanobench::doNotOptimizeAway(GoogleMessage1.full_read(data, m));
        });
    }

    //system("pause");
} // TEST_CASE("GoogleMessage1")


} // TEST_SUITE("pb")
