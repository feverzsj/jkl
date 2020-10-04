#pragma once

#include <jkl/config.hpp>
#include <jkl/util/str.hpp>
#include <jkl/util/concepts.hpp>
#include <rapidjson/writer.h>
#include <cstdlib>
#include <charconv>


namespace jkl{


template<unsigned Cap>
struct fixed_storage_str
{
    unsigned n;
    char     d[Cap];

    template<unsigned N>
    void assign_literal(char const (&s)[N]) noexcept
    {
        static_assert(N - 1 <= Cap);
        memcpy(d, s, N - 1);
        n = N - 1;
    }

    void set_end(char const* e) noexcept
    {
        BOOST_ASSERT(d <= e && e < d + Cap);
        n = static_cast<unsigned>(e - d);
    }

    char      * data()       noexcept { return d; }
    char const* data() const noexcept { return d; }
    auto        size() const noexcept { return n; }

//     template<class Tx          > operator std::basic_string_view<char, Tx    >() const noexcept { return {data(), size()}; }
//     template<class Tx, class Ax> operator std::basic_string     <char, Tx, Ax>() const          { return {data(), size()}; }
// 
//     template<class Tx          > operator boost::basic_string_view      <char, Tx    >() const noexcept { return {data(), size()}; }
//     template<class Tx, class Ax> operator boost::container::basic_string<char, Tx, Ax>() const          { return {data(), size()}; }
};


template<size_t Base, std::integral V>
consteval size_t stringify_integer_max_bytes() noexcept
{
    static_assert(sizeof(V) <= 16 && (Base == 2 || Base == 8 || Base == 10 || Base == 16));

    constexpr bool sign = std::signed_integral<V>;
    constexpr size_t sv = sizeof(V);

    if constexpr(Base == 2)
    {
        return sv * 8; // 2 = 2^1
    }
    else if constexpr(Base == 8)
    {
        return sv * 8 / 3 + 1; // 8 = 2^3
    }
    else if constexpr(Base == 16)
    {
        return sv * 8 / 4; // 16 = 2^4
    }
    else if constexpr(Base == 10)
    {
        if constexpr(sv == 1)
            return 3 + sign;
        else if constexpr(sv == 2)
            return 5 + sign;
        else if constexpr(sv == 4)
            return 10 + sign;
        else if constexpr(sv == 8)
            return 20 + sign;
        else if constexpr(sv == 16)
            return 39 + sign;
    }
}


constexpr auto const& stringify(_str_ auto const& v) noexcept { return v; }
constexpr char const* stringify(bool              v) noexcept { return v ? "true" : "false"; }
constexpr char const* stringify(std::nullptr_t     ) noexcept { return "null"; }

template<size_t Base = 10, std::integral V>
auto stringify(V v) noexcept
{
    constexpr auto cap = stringify_integer_max_bytes<Base, V>();

    fixed_storage_str<cap> b;

    if constexpr(Base == 10 && sizeof(v) <= sizeof(int64_t))
    {
        char* e = nullptr;

        if constexpr(sizeof(v) <= sizeof(int32_t))
        {
            if constexpr(std::signed_integral<V>)
                e = rapidjson::internal::i32toa(v, b.d);
            else
                e = rapidjson::internal::u32toa(v, b.d);
        }
        else
        {
            if constexpr(std::signed_integral<V>)
                e = rapidjson::internal::i64toa(v, b.d);
            else
                e = rapidjson::internal::u64toa(v, b.d);
        }

        b.set_end(e);
    }
    else
    {
        auto[e, ec] = std::to_chars(b, b + cap, v, Base);
        BOOST_ASSERT(ec == std::errc());
        b.set_end(e);
    }

    return b;
}

template<std::integral V>
auto stringify(V v, int base) noexcept
{
    BOOST_ASSERT(base == 2 || base == 8 || base == 10 || base == 16);

    constexpr auto cap = stringify_integer_max_bytes<2, V>();

    fixed_storage_str<cap> b;

    auto[e, ec] = std::to_chars(b.d, b.d + cap, v, base);

    BOOST_ASSERT(ec == std::errc());

    b.set_end(e);

    return b;
}

auto stringify(std::floating_point auto v, int prec = 324) noexcept
{
    fixed_storage_str<25> b; // check rapidjson::Writer::WriteDouble() for the max buffer size

    if(! rapidjson::internal::Double(v).IsNanOrInf())
    {
        char* e = rapidjson::internal::dtoa(v, b.d, prec); // convert to shortest representation
        b.set_end(e);
    }
    else
    {
        // follow the js spec, use: NaN, Infinity, -Infinity
        if(rapidjson::internal::Double(v).IsNan())
            b.assign_literal("NaN");
        else if(rapidjson::internal::Double(v).Sign())
            b.assign_literal("-Infinity");
        else
            b.assign_literal("Infinity");
    }

    return b;
}


template<class T>
concept _stringifible_ = requires(T& t){ stringify(t); };


constexpr size_t max_stringify_size(_str_ auto const& v) noexcept { return str_size(v); }
constexpr size_t max_stringify_size(bool              v) noexcept { return v ? 4 : 5; }
consteval size_t max_stringify_size(std::nullptr_t     ) noexcept { return 4; }

template<size_t Base = 10, std::integral V>
consteval size_t max_stringify_size(V) noexcept { return stringify_integer_max_bytes<Base, V>(); }
consteval size_t max_stringify_size(std::floating_point auto) noexcept { return 25; }


constexpr size_t max_stringify_size(_stringifible_ auto const&... vs) noexcept
{
    return (... + max_stringify_size(vs));
}


// NOTE: character literal not supported, use single character string literal
//       or use append_str(), cat_str()

void append_as_str(auto& d, _stringifible_ auto const&... t)
{
    append_str(d, stringify(t)...);
};

template<class D = string>
D cat_as_str(_stringifible_ auto const&... t)
{
    D d;
    append_as_str(d, t...);
    return d;
}


} // namespace jkl
