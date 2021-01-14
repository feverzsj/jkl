- [Introduction](#Introduction)
- [Supported compilers](#Supported-compilers)
- [Limitations](#Limitations)
- [Basic usage](#Basic-usage)
- [Reference](#Reference)
- [Benchmark](#Benchmark)
  * [Results](#Results)
  * [Analyze](#Analyze)


# Introduction
A `protobuf` DSL for C++. Using this module, you can write something similar to `protobuf` IDL directly in C++, and directly serialize/deserialize from/to your object without intermediate message object. For example:

<table>
<tr><th>Protobuf</th> <th>C++</th></tr>
<tr>
<td>

```protobuf
syntax = "proto2";

message Point {
  required double x = 1;
  required double y = 2;
  optional string label = 3;
}

message Line {
  required Point start = 1;
  required Point end = 2;
}

message Polyline {
  repeated Point points = 1;
}
```
</td>
<td>

```C++
#include "jkl/pb/dsl.hpp"

constexpr auto Point = pb_message<"Point">(
    pb_double<"x"     , 1>(JKL_P_VAL(d.x)),
    pb_double<"y"     , 2>(JKL_P_VAL(d.y)),
    pb_string<"label" , 3>(JKL_P_VAL(d.label), p_optional)
);

constexpr auto Line = pb_message<"Line">(
    Point._  <"start" , 1>(JKL_P_VAL(d.start)),
    Point._  <"end"   , 2>(JKL_P_VAL(d.end))
);

constexpr auto Polyline = pb_message<"Polyline">(
    pb_repeated(Point._<"points", 1>(), JKL_P_VAL(d.points))
);
```
</td>
</tr>
</table>

# Supported compilers
- [LLVM nightly](https://apt.llvm.org/)
- ~~[GCC snapshots](https://jwakely.github.io/pkg-gcc-latest/)~~ ICE in 20201227 Snapshot
- MSVC comes with the lastest Visual Studio Preview

# Limitations
- Only proto2 is supported.
- Enum, embedded message are not supported.
- Repeated field should not be interleaved on wire.
- Merging message (which contains required sub fields) is not supported, so a non repeated message field should only present on wire once;
- By default, strings are not validated as utf8. You can use `p_check/JKL_P_CHECK` to do any check.

# Basic usage
```C++
#include "jkl/pb/dsl.hpp"
#include <string>

using namespace jkl;

struct point_t{
    int32_t x;
    int32_t y;
    std::string label;
};

struct line_t{
    point_t start = {1, 1, "start"};
    point_t end   = {2, 2, "end"};;
};

// Use pb_message<"MessageType">(fields...) to define a protobuf message type.
// Since embedded message is not supported, MessageType should be unique in same IDL.
constexpr auto Point = pb_message<"Point">(
    // Use pb_xxx<"FieldName", ID>(params...) to define fields.
    // FieldName and ID should not be duplicated in same message (including those in oneof field).
    pb_double<"x"     , 1>(JKL_P_VAL(d.x)),
    // p_xxx and JKL_P_XXX are parameters for specifying how to access/check field value.
    // Parameters can present in any order.
    pb_double<"y"     , 2>(JKL_P_VAL(d.y)),
    pb_string<"label" , 3>(JKL_P_VAL(d.label), p_optional)
    );

constexpr auto Line = pb_message<"Line">(
    // Use Message._<"FieldName", ID>(params...) to define field of message type.
    Point._  <"start" , 1>(JKL_P_VAL(d.start)),
    Point._  <"end"   , 2>(JKL_P_VAL(d.end))
    );

int main()
{
    line_t d;
    std::string buf;

    Line.write(buf, d); // serialize to a _resizable_byte_buf_ from l
    Line.full_read(buf, d).throw_on_error(); // deserialize from _byte_buf_ to l
    auto* readEnd = Line.read(buf.data(), buf.data() + buf.size(), d).value_or_throw();
    auto idlDef = pb_gen_def(Point, Line); // generate protobuf IDL
}
```

# Reference
Suppose `d` is what you pass to `write(buf, d)` or `full_read(buf, d)`

## pb_bool/pb_int32/pb_int64/pb_uint32/pb_uint64/pb_sint32/pb_sint64
`protobuf`'s `bool`/`int32`/`int64`/`uint32`/`uint64`/`sint32`/`sint64`.

Expected value type: `_arithmetic_`

Available parameters: `p_val`, `p_optional`, `p_has_val`, `p_clear_val`, `p_default`, `p_check`.

## pb_sfixed32/pb_sfixed64/pb_fixed32/pb_fixed64/pb_float/pb_double
`protobuf`'s `sfixed32`/`sfixed64`/`fixed32`/`fixed64`/`float`/`double`.

Expected value type: `_arithmetic_`

Available parameters: `p_val`, `p_optional`, `p_has_val`, `p_clear_val`, `p_default`, `p_check`.

## pb_bytes/pb_string
`protobuf`'s `bytes`/`string`.

Expected value type: `_buf_` or `_str_`

Available parameters: `p_val`, `p_optional`, `p_has_val`, `p_clear_val`, `p_default`, `p_check`.

NOTE: you can use `p_optional` without using a `std::optional` value. In that case, if value is fixed size(like array, read only range), you should specify `p_clear_val`, and `p_has_val`(if `buf_size()`/`str_size()` are not proper).

## pb_repeated(Field<'Name', ID>(), parameters...)
Expected value type: `_range_`.

Available parameters: `p_val`, `p_clear_val`, `p_check`.

NOTE: for scalar types, always use packed format `[packed=true]`.

## pb_map<'Name', ID>(KeyField, ValueField, parameters...)
Expected value type: `_range_`.

Available parameters: `p_val`, `p_clear_val`, `p_check`.

`KeyField` can be any integral or string field (so, any scalar field except for floating point and bytes).

`ValueField` can be any field except another map.

## pb_oneof<'Name'>(Fields...)(parameters...)
Expected value type: `std::variant` or any type with `p_get_member`, `p_activate_member`, `p_active_member_idx`.

Available parameters: `p_val`, `p_get_member`, `p_activate_member`, `p_active_member_idx`, `p_check`.

NOTE: `p_check` on sub fields won't be called, you should check on oneof field value itself.

## pb_message<'Type'>(Fields...)
Expected value type: struct/class

Available parameters: `p_val`, `p_optional`, `p_has_val`, `p_clear_val`, `p_default`, `p_check`.

## p_val(v)
Specifiy how to access field value.

Depends on `v`, the value of the field with this parameter can be `d.v()`, `d.v`, `v(d)`, `v`.

Without this paramter, the field value is `d`.

## p_check(auto (auto& d) -> std::boolean || std::convertible_to<aresult<>>)
For `read` only: on successfully reading a message, each field with this paramter will be called with its checker. The checker can return `boolean` or anything convertible to `aresult`.


## p_optional
Field value is std::optional or _buf_(for bytes/string fields).

Implies optional field

## p_has_val(bool(auto& d))
For `write` only: check if field value exists.

Implies optional field.

## p_clear_val(void(auto& d))
For `read` only: clear field value.

Implies optional field.

## p_default(DefaultValue)
Same as: `p_has_val([](auto& d){ return val(d) != DefaultValue; }), p_clear_val([](auto& d){ val(d) = DefaultValue; })`.

Implies optional field.

## p_get_member(auto&(auto& d, `_integral_constant_` auto I))
Specify how to get Ith member of oneof field value.

`I` is assured to be large than 0.

NOTE: valid member index starts from `1`. `0` index means there is no active member, i.e. the value is empty.

## p_activate_member(void(auto& d, `_integral_constant_` auto I))
Specify how to activate Ith member of oneof field value.

if `I == 0`, the field value should be cleared.

## p_active_member_idx(auto (auto& d) -> std::integral)
Specify how to get the index of current active member of oneof field value.

Returning `0` means the value is cleared.

## JKL_P_CHECK(Expr)
Macro defined as `p_check([](auto& d) { return Expr; })`
## JKL_P_VAL(Expr)
Macro defined as `p_val([](auto& d) -> auto& { return Expr; })`
## JKL_P_HAS_VAL(Expr)
Macro defined as `p_has_val([](auto& d) -> bool { return Expr; })`
## JKL_P_CLEAR_VAL(Expr)
Macro defined as `p_clear_val([](auto& d) { Expr; })`
## JKL_P_GET_MEMBER(Expr)
Macro defined as `p_get_member([](auto& d, _integral_constant_ auto I) -> auto& { return Expr; })`
## JKL_P_ACTIVATE_MEMBER(Expr)
Macro defined as `p_activate_member([](auto& d, _integral_constant_ auto I) { Expr; })`
## JKL_P_ACTIVE_MEMBER_IDX(Expr)
Macro defined as `p_active_member_idx([](auto& d) { return Expr; })`



# Benchmark
Micro-benchmark framework:
- [nanobench](https://github.com/martinus/nanobench)

Hardware:
- i3-3220@3.3GHz, 8GB ram.

Platforms: 
- Windows 10, 19041.685, set process affinity to none hyperthreaded core, and process priority to realtime.
- Ubuntu 20.04, pyperf system tune, pin process to isolated cpu core.

Compilers:
- msvc cl /O2
- ~~g++ -O3~~
- clang++ -O3

Dataset:
- [GoogleMessage1](https://github.com/protocolbuffers/protobuf/tree/master/benchmarks/datasets/google_message1)

Method:
- Focus on serialize/deserialize only.
- Reuse same message struct, so no allocation is involved.
- Both use `google::protobuf::internal::IsStructurallyValidUTF8` to validate strings.

## Results:

| relative |               ns/op |                op/s |    err% |     total | GoogleMessage1 write
|---------:|--------------------:|--------------------:|--------:|----------:|:---------------------
|   100.0% |              509.35 |        1,963,288.23 |    0.6% |      0.62 | `google.protobuf`
|   193.5% |              263.29 |        3,798,136.23 |    0.5% |      0.32 | `jkl.pb`

| relative |               ns/op |                op/s |    err% |     total | GoogleMessage1 read
|---------:|--------------------:|--------------------:|--------:|----------:|:--------------------
|   100.0% |              617.36 |        1,619,807.11 |    0.5% |      0.75 | `google.protobuf`
|   109.6% |              563.15 |        1,775,737.83 |    0.8% |      0.70 | `jkl.pb`


## Analyze:

Although `protoc` generated C++ code is highly optimized, there is still some room for optimization.

While serializing a length delimited field, the length comes before the payload. This forces us to calculate the length before serializing the payload. And even worse, since length is varint, we cannot skip it and serialize payload first. It's easy for fields like string/bytes to calculate their length, but hard for any thing containing a varint field. This is the main reason why protobuf serialization is far less effective than plain serialization.

The obvious optimization here is caching the length. `google.protobuf` does this in `MessageLite::GetCachedSize()`, but only for message fields. We go steps further by caching **any** hard-to-calculate field length, and finding the minimal number of length to be cached at compile time, so we can use a stack allocated cache array if possible. With all the length known, the total wire size is also known, so we only allocate the buffer once for the whole serialization.

For deserializing, the biggest performance gain comes from using `switch-case` to match field tags. `protoc` generated code does this already. We use complie time generated `switch-case` to match any number of fields in any order.

For packed repeated fixed size scalar fields, if same memory layout is detected, `memcpy` is used.

Furthermore, using C++'s powerful compile time programming, we encode the tag varint at compile time and use `memcpy/memcmp` to serialize and compare tag. Since `memcpy/memcmp` are mostly intrinsic functions, compiler can generate better optimized code.

