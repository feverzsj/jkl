#pragma once

#include <jkl/json/fwd.hpp>
#include <jkl/util/str.hpp>
#include <jkl/util/stringify.hpp>


namespace jkl{



// TODO: sync with rapidjson::Writer::WriteString
// escape " or \ or / or control-character as \u four-hex-digits
inline char* _write_jstr(char const* s, size_t n, char* e)
{
    static char const hexDigits[16] ={
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F'
    };

    static char const escape[256] ={
    #define Z16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        //   0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
        'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'b', 't', 'n', 'u', 'f', 'r', 'u', 'u', // 00
        'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', // 10
        0, 0, '"', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 20
        Z16, Z16,                                                                       // 30~4F
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\\', 0, 0, 0, // 50
        Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16                                // 60~FF
    #undef Z16
    };

    *(e++) = '"';

    for(size_t i = 0; i < n; ++i)
    {
        char const c = s[i];
        unsigned char const uc = c;

        if(char const ec = escape[uc])
        {
            *(e++) = '\\';
            *(e++) = ec;
            if(ec == 'u')
            {
                *(e++) = '0';
                *(e++) = '0';
                *(e++) = hexDigits[uc >> 4];
                *(e++) = hexDigits[uc & 0xF];
            }
        } else
        {
            *(e++) = c;
        }
    }

    *(e++) = '"';

    return e;
}


template<class Buf, class Str>
void write_jstr(Buf& b, Str const& v)
{
    auto len = str_size(v);
    auto maxLen = 2 + len * 6; // suppose all to be escaped "\uxxxx..."
    auto e = _write_jstr(str_data(v), len, buy_buf(b, maxLen));
    resize_buf(b, buf_data(b) - e);
}

template<class Buf, class Str>
void write_jstr_noescape(Buf& b, Str const& v)
{
    auto n = str_size(v);
    auto d = buy_buf(b, 2 + n);

    d[0] = '"';
    std::copy_n(str_data(v), n, d + 1);
    d[1 + n] = '"';
}



constexpr int json_writer_default_precision = stringify_fp_default_prec;


template<
    class Buf,
    char  OpenCh, char CloseCh,
    int   Precision = json_writer_default_precision,
        // digits after the decimal-point
        // similar to %.nf of printf:
        // http://stackoverflow.com/questions/10671600/why-does-printf-6g-value-ignore-zeroes-after-decimal-point
        // for basic_ostream
        // http://stackoverflow.com/questions/3923202/set-the-digits-after-decimal-point
    bool KeyNoEscape = true,
    bool HasParent   = false
>
class json_writer
{
    using self = json_writer;

    Buf& _b;
    size_t _initPos;

    self& end_val()
    {
        _b.push_back(',');
        return *this;
    }

public:
    template<int NewPrecision>
    struct rebind_precision
    {
        using type = json_writer<Buf, OpenCh, CloseCh, NewPrecision,
                                 KeyNoEscape, HasParent>;
    };

    explicit json_writer(Buf& ss)
        : _b(ss)
    {
        _initPos = buf_size(_b);
        _b.push_back(OpenCh);
    }

    ~json_writer()
    {
        if(buf_size(_b) > _initPos + 1)
            buf_back(_b) = CloseCh;
        else
            _b.push_back(CloseCh);

        if constexpr(HasParent)
            _b.push_back(',');
    }

    template<class Buf1, char B, char E, int D, bool K, bool H>
    friend class json_writer;

    // use w._b
    template<char B, char E, int D, bool K, bool H>
    explicit json_writer(json_writer<Buf, B, E, D, K, H>& w)
        : _b{w._b}
    {
        _initPos = buf_size(_b);
        _b.push_back(OpenCh);
    }


// directly write a jprim:

    // json string value, will escape " or \ or / or control-character
    // as \u four-hex-digits
    template<class Str>
    self& str(Str const& v)
    {
        write_jstr(_b, v);
        return *this;
    }

    // v must not contain any character require escaping
    template<class Str>
    self& str_noescape(Str const& v)
    {
        write_jstr_noescape(_b, v);
        return *this;
    }

    self& double_(double v, int prec = Precision)
    {
        append_str(_b, stringify(v, prec));
        return *this;
    }

    self& val(_str_ const&  v) { write_jstr(_b, v); return *this; }
    self& val(nullptr_t      ) { append_str(_b, "null"); return *this; }
    self& val(bool          v) { if(v) append_str(_b, "true") else append_str(_b, "false"); return *this; }
    self& val(std::integral v) { append_str(_b, stringify(v)); return *this; }
    template<int Prec = Precision>
    self& val(std::floating_point v) { append_str(_b, stringify(v, Prec)); return *this; }


    self& val(std::reference_wrapper<auto> v) { return val(v.get()); }
    self& val(std::optional<auto> const&   v) { if(v) val(*v) else append_str("null"); return *this; }

    template<class V>
    self& val(V&& v) requires(requires{ std::forward<V>(v)(*this); })
    {
        std::forward<V>(v)(*this);
        return *this;
    }

    template<class V>
    self& val(V&& v) requires(requires{ serialize_jval(*this, v); })
    {
        serialize_jval(*this, v);
        return *this;
    }

// array

    template<class T>
    self& elm(T const& v)
    {
        val(v).end_val();
        return *this;
    }

    template<class V>
    self& elm_if(bool cond, V const& v)
    {
        return cond ? elm(v) : *this;
    }

    template<class V1, class V2>
    self& elm_if(bool cond, V1 const& v1, V2 const& v2)
    {
        return cond ? elm(v1) : elm(v2);
    }

    template<class V, class...T>
    self& elms(V const& v, T const&... vs)
    {
        return elm(v).elms(vs...);
    }

    self& elms()
    {
        return *this;
    }


// member pair
    template<class K>
    self& begin_pa(K const& k)
    {
        if(KeyNoEscape)
            str_noescape(k);
        else
            val(k); // user still has chance to use jnoescape()

        _b.push_back(':');
        return *this;
    }

    self& end_pa()
    {
        return end_val();
    }

    self& pa(auto const& k, jopt_<auto> const& v)
    {
        if(v.write)
            pa(k.str, v);
        return *this;
    }

    self& pa(auto const& k, auto const& v)
    {
        if constexpr(KeyNoEscape)
            str_noescape(k);
        else
            val(k); // user still has chance to use jnoescape()

        _b.push_back(':');
        val(v);
        end_val();

        return *this;
    }

    template<class K, class V>
    self& pa_if(bool cond, K const& k, V const& v)
    {
        return cond ? pa(k, v) : *this;
    }

    template<class K, class V1, class V2>
    self& pa_if(bool cond, K const& k, V1 const& v1, V2 const& v2)
    {
        return cond ? pa(k, v1) : pa(k, v2);
    }

    template<class K, class V, class...T>
    self& pas(K const& k, V const& v, T const&... kvs)
    {
        return pa(k, v).pas(kvs...);
    }

    self& pas()
    {
        return *this;
    }
};


template<
    class Buf,
    int  Precision   = json_writer_default_precision,
    bool KeyNoEscape = true,
    bool HasParent   = false
>
using jobj_writer = json_writer<Buf, '{', '}', Precision, KeyNoEscape, HasParent>;

template<
    class Buf,
    int  Precision   = json_writer_default_precision,
    bool KeyNoEscape = true,
    bool HasParent   = false
>
using jarr_writer = json_writer<Buf, '[', ']', Precision, KeyNoEscape, HasParent>;



template<int Prec, class Buf>
auto make_jobj_writer(Buf& b)
{
    return jobj_writer<Buf, Prec>(b);
}

template<int Prec, class Buf>
auto make_jarr_writer(Buf& b)
{
    return jarr_writer<Buf, Prec>(b);
}

template<class Buf>
auto make_jobj_writer(Buf& b)
{
    return jobj_writer<Buf>(b);
}

template<class Buf>
auto make_jarr_writer(Buf& b)
{
    return jarr_writer<Buf>(b);
}

// use w._b

template<int Prec, class Buf, char B, char E, int P, bool K, bool H>
auto make_jobj_writer(json_writer<Buf, B, E, P, K, H>& w)
{
    return jobj_writer<Buf, Prec, K>(w);
}

template<int Prec, class Buf, char B, char E, int P, bool K, bool H>
auto make_jarr_writer(json_writer<Buf, B, E, P, K, H>& w)
{
    return jarr_writer<Buf, Prec, K>(w);
}

// preserve precision from given writer

template<class Buf, char B, char E, int P, bool K, bool H>
auto make_jobj_writer(json_writer<Buf, B, E, P, K, H>& w)
{
    return jobj_writer<Buf, P, K>(w);
}

template<class Buf, char B, char E, int P, bool K, bool H>
auto make_jarr_writer(json_writer<Buf, B, E, P, K, H>& w)
{
    return jarr_writer<Buf, P, K>(w);
}


// child, element

// for object parent

template<int Prec, class Buf, int P, bool K, bool H, class Str>
auto add_jobj(jobj_writer<Buf, P, K, H>& parent, Str const& key)
{
    parent.begin_pa(key);
    return jobj_writer<Buf, Prec, K, true>(parent);
}

template<int Prec, class Buf, int P, bool K, bool H, class Str>
auto add_jarr(jobj_writer<Buf, P, K, H>& parent, Str const& key)
{
    parent.begin_pa(key);
    return jarr_writer<Buf, Prec, K, true>(parent);
}

// preserve precision from given parent
template<class Buf, int P, bool K, bool H, class Str>
auto add_jobj(jobj_writer<Buf, P, K, H>& parent, Str const& key)
{
    parent.begin_pa(key);
    return jobj_writer<Buf, P, K, true>(parent);
}

template<class Buf, int P, bool K, bool H, class Str>
auto add_jarr(jobj_writer<Buf, P, K, H>& parent, Str const& key)
{
    parent.begin_pa(key);
    return jarr_writer<Buf, P, K, true>(parent);
}


// for array parent

template<int Prec, class Buf, int P, bool K, bool H>
auto add_jobj(jarr_writer<Buf, P, K, H>& w)
{
    return jobj_writer<Buf, Prec, K, true>(w);
}

template<int Prec, class Buf, int P, bool K, bool H>
auto add_jarr(jarr_writer<Buf, P, K, H>& w)
{
    return jarr_writer<Buf, Prec, K, true>(w);
}

// preserve precision from given parent
template<class Buf, int P, bool K, bool H>
auto add_jobj(jarr_writer<Buf, P, K, H>& w)
{
    return jobj_writer<Buf, P, K, true>(w);
}

template<class Buf, int P, bool K, bool H>
auto add_jarr(jarr_writer<Buf, P, K, H>& w)
{
    return jarr_writer<Buf, P, K, true>(w);
}




// operations, should write 1 value to the stream, so don't use add_xxx(), as they would append ','


template<int Prec>
auto jprec(double v)
{
    return [=](auto& w)
    {
        w.double_(v, Prec);
    };
}

template<class Str>
auto jescape(Str const& s)
{
    return [&](auto& w)
    {
        w.str(s);
    };
}

template<class Str>
auto jnoescape(Str const& s)
{
    return [&](auto& w)
    {
        w.str_noescape(s);
    };
}

template<class V1, class V2>
auto j_if(bool cond, V1 const& v1, V2 const& v2)
{
    return [&, cond](auto& w)
    {
        if(cond)
            w.val(v1);
        else
            w.val(v2);
    };
}


// user defined string literal: "raw string"_raw
// inline auto operator "" _raw(char const* s, size_t n)
// {
//     return [=](auto& w)
//     {
//         w.str_noescape(string_view(s, n));
//     };
// }
// 

template<class T>
struct is_jwritter_helper : std::false_type{};

template<class B, char O, char C, int P, bool K, bool H>
struct is_jwritter_helper<json_writer<B, O, C, P, K, H>> : std::true_type{};

template<class T>
using is_jwritter = is_jwritter_helper<std::remove_cv_t<T>>;

template<class T>
inline constexpr bool is_jwritter_v = is_jwritter<T>::value;


} // namespace jkl