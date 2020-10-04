#pragma once

#include <jkl/json/fwd.hpp>
#include <jkl/util/file.hpp>
#include <jkl/util/concepts.hpp>
#include <jkl/util/assign_append.hpp>

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>


namespace jkl{


// jdoc is also jval
class jdoc : public rapidjson::Document
{
public:
    std::string buf;
    explicit operator bool() const { return ! HasParseError(); }
};


template<class Str>
jdoc parse_json(Str const& str)
{
    jdoc doc;
    doc.Parse(str_data(str), str_size(str));
    return doc;
}

// parse json string in-place, the buf containing the string will be modified, and must exist with doc.
template<class CStr>
jdoc parse_json_insitu(CStr& s)
{
    jdoc doc;
    doc.ParseInsitu(cstr_data(s));
    return doc;
}

// this prefers to parse the file chunk by chunk
template<size_t ReadBufSize = 4096, class Str>
jdoc parse_json_file(Str const& path)
{
    static_assert(ReadBufSize >= 4);

    jdoc doc;

    auto file = open_ro_bin_file(path);

    if(! file)
        return doc;

    char readBuf[ReadBufSize];

    rapidjson::FileReadStream is(file.handle(), readBuf, ReadBufSize);

    doc.ParseStream(is);
    return doc;
}

template<class CStr>
jdoc parse_json_file_full_buffered(CStr const& path)
{
    jdoc doc;
    if(read_whole_file(path, doc.buf))
        doc.ParseInsitu(cstr_data(doc.buf));
    return doc;
}


// follow the naming of the json spec.: http://json.org/
// jval s
template<class E, class A> bool is_jobj (rapidjson::GenericValue<E, A> const& v) { return v.IsObject(); }
template<class E, class A> bool is_jarr (rapidjson::GenericValue<E, A> const& v) { return v.IsArray (); }
template<class E, class A> bool is_jnull(rapidjson::GenericValue<E, A> const& v) { return v.IsNull  (); }
template<class E, class A> bool is_jbool(rapidjson::GenericValue<E, A> const& v) { return v.IsBool  (); }
template<class E, class A> bool is_jnum (rapidjson::GenericValue<E, A> const& v) { return v.IsNumber(); }
template<class E, class A> bool is_jstr (rapidjson::GenericValue<E, A> const& v) { return v.IsString(); }

template<class E, class A>
bool is_jprim(rapidjson::GenericValue<E, A> const& v)
{
    return is_jnum(v) || is_jstr(v) || is_jbool(v) || is_jnull(v);
}


template<class E, class A>
auto jarr_size(rapidjson::GenericValue<E, A> const& v)
{
    return v.Size();
}


template<class E, class A1, class A2>
auto* get_jval(rapidjson::GenericValue<E, A1> const& obj, rapidjson::GenericValue<E, A2> const& name)
{
    auto it = obj.FindMember(name);
    return (it != obj.MemberEnd()) ? &it->value : nullptr;
}

template<class E, class A, class Str>
auto* get_jval(rapidjson::GenericValue<E, A> const& obj, Str const& name)
{
    return get_jval(obj, rapidjson::GenericValue<E, A>{
                            typename rapidjson::GenericValue<E, A>::
                                StringRefType(str_data(name),
                                              static_cast<rapidjson::SizeType>(str_size(name)))});
}


bool read_jprim(auto const& v, bool& t)
{
    if(v.IsBool())
    {
        t = v.GetBool();
        return true;
    }
    return false;
}

bool read_jprim(auto const& v, std::integral auto& t)
{
    if constexpr(sizeof(t) < sizeof(int))
    {
        if(v.IsInt())
            return cvt_num(v.GetInt(), t);
    }
    else if constexpr(sizeof(t) > sizeof(int64_t))
    {
        if constexpr(std::is_signed_v<T>)
        {
            if(v.IsInt64())
            {
                t = v.GetInt64();
                return true;
            }

            if(v.IsUint64())
            {
                t = v.GetUint64();
                return true;
            }
        }
        else
        {
            if(v.IsUint64())
            {
                t = v.GetUint64();
                return true;
            }
        }
    }
    else
    {
        if(v.template Is<T>())
        {
            t = v.template Get<T>();
            return true;
        }
    }

    return false;
}

bool read_jprim(auto const& v, std::floating_point auto& t)
{
    if(v.IsDouble())
        return cvt_num(v.GetDouble(), t);
    return false;
}

bool read_jprim(auto const& v, std::_str_class_ auto& t)
{
    uassign(t, v.GetString(), v.GetStringLength());
    return true;
}




// extended

template<class Jval, class Str>
Jval const* get_jobj(Jval const& obj, Str const& name)
{
    if(auto* v = get_jval(obj, name))
    {
        if(is_jobj(*v))
            return v;
    }
    return nullptr;
}

template<class Jval, class Str>
Jval const* get_jarr(Jval const& obj, Str const& name)
{
    if(auto* v = get_jval(obj, name))
    {
        if(is_jarr(*v))
            return v;
    }
    return nullptr;
}


bool read_jprim(auto const& obj, _str_ auto const& name, auto& t)
{
    if(auto* v = get_jval(obj, name))
        return read_jprim(*v, t);
    return false;
}

// get obj.name1.name2...
bool read_jprim(auto const& obj, _str_ auto const& name1, _str_ auto const& name2, auto& t)
{
    if(auto* o1 = get_jobj(obj, name1))
        if(auto* o2 = get_jval(*o1, name2))
            return read_jprim(*o2, t);
    return false;
}

bool read_jprim(auto const& obj, _str_ auto const& name, auto& t, auto&& def)
{
    if(! read_jprim(obj, name, t))
    {
        t = std::forward<decltype(def)>(def);
        return false;
    }
    return true;
}


bool read_jval(auto& jv, auto& v) requires(requires{ read_jprim(jv, v); })
{
    return read_jprim(jv, v);
}

bool read_jval(auto& jv, auto& v) requires(requires{ serialize_jval(jv, v); })
{
    return serialize_jval(jv, v);
}

bool read_jval(auto& jv, auto&& v) requires(requires{ std::forward<decltype(v)>(v)(jv); })
{
    return std::forward<decltype(v)>(v)(jv);
}

template<class V>
bool read_jval(auto& jv, std::reference_wrapper<V>& v)
{
    return read_jval(jv, v.get());
}

template<class V>
bool read_jval(auto& jv, std::optional<V>& v)
{
    if(is_jnull(jv))
    {
        v.reset();
        return true;
    }

    if(! v)
        v.emplace();
    return read_jval(jv, *v);
}



bool read_jobj(auto const& jo, auto const& k, auto&& v)
{
    BOOST_ASSERT(is_jobj(jo));

    if(auto* p = get_jval(jo, jopt_get_key(k)))
        return read_jval(*p, std::forward<decltype(v)>(v));
    return false;
}

bool read_jobj(auto const& jo, auto const& k, auto&& v, auto&&... kvs)
{
    return read_jobj(jo, k, std::forward<V>(v))
        && read_jobj(jo, std::forward<KVS>(kvs)...);
}


// initially lastUsed element should be -1
// try best to map as many jarr elements to v
bool _read_jarr_elems(auto const& ja, int& lastUsed, int s, auto&& v)
{
    using vtype = std::remove_reference_t<decltype(v)>;

    int remain = s - lastUsed + 1;

    if(remain < 1)
        return false;

    if constexpr(std::is_array_v<vtype>)
    {
        if(std::extent_v<vtype> > remain)
            return false;

        for(auto& e : v)
        {
            ++lastUsed;
            if(! read_jval(ja[lastUsed], e))
                return false;
        }
        return true;
    }
    else if constexpr(requires{v.resize(remain); std::begin(v); std::end(v);})
    {
        v.resize(remain);
        for(auto& e : v)
        {
            ++lastUsed;
            if(! read_jval(ja[lastUsed], e))
                return false;
        }
        return true;
    }
    else if constexpr(_tuple_like<vtype>)
    {
        if(std::tuple_size_v<vtype> > remain)
            return false;

        return std::apply([&](auto&&...e){
            return (... && (++lastUsed, read_jval(ja[lastUsed], std::forward<decltype(e)>(e))));
        }, v);
    }
    else if constexpr(requires{ std::forward<decltype(v)>(v)(ja[++lastUsed]); })
    {
        return std::forward<decltype(v)>(v)(ja[++lastUsed]);
    }
    else
    {
        return read_jval(ja[++lastUsed], std::forward<V>(v));
    }
}

// try best to map jarr elements to v...
// any of v can be prim, functor, array, tuple like, sequential container
// prim, functor use exactly 1 remaining element
// array, tuple like use exact how many elements they have from remaining elements
// seq will use up all remaining elements
// if still more v to match, return false
template<class Jval, class... V>
bool read_jarr(Jval const& ja, V&&... v)
{
    BOOST_ASSERT(is_jarr(ja));

    int lastUsed = -1;
    int s = jarr_size(ja);
    return (... && _read_jarr_elems(ja, lastUsed, s, v));
}


} // namespace jkl