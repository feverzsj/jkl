#pragma once

#include <jkl/json/read.hpp>
#include <jkl/json/write.hpp>


namespace jkl{


// v is the type supported by read_jval() or writer.val().
// for read:
//      q can be a functor which will be invoked on successfully read v,
//      or default value when conversion failed(in which case it is not considered a failure)
auto jval_(auto& v, auto&& q)
{
    return [&v, cq = decay_capture<Q>(q)]<class Ar>(Ar& ar)
    {
        auto& q = unwrap_ref(cq);

        if constexpr(is_jwritter_v<Ar>)
        {
            ar.val(v);
        }
        else if constexpr(requires{ {q(v)} -> std::boolean; })
        {
            return read_jval(ar, v) && q(v);
        }
        else if constexpr(requires{ v = std::move(q); })
        {
            if(! read_jval(ar, v))
                v = std::move(q);
            return true;
        }
        else
        {
            JKL_DEPENDENT_FAIL(V, "unsupported type");
        }
    };
}


// for read only:
//      V is the type supported by read_jval,
//      f should be a functor which will be invoked on successfully converted V.
template<class V, class F>
auto jval_(F&& f)
{
    return [f = decay_capture<F>(f)]<class Jv>(Jv const& jv)
    {
        static_assert(! is_jwritter_v<Jv>);

        V v;
        return read_jval(jv, v) && f(v);
    };
}


template<class W, class... KVS>
void _jobj_write(W& ar, KVS&&... kvs)
{
    make_jobj_writer(ar).pas(std::forward<KVS>(kvs)...);
}

template<class Jval, class... KVS>
bool _jobj_read(Jval const& ar, KVS&&... kvs)
{
    return is_jobj(ar) && read_jobj(ar, std::forward<KVS>(kvs)...);
}


template<class... KVS>
auto jobj_(KVS&&... kvs)
{
    return [...kvs = decay_capture<KVS>(kvs)]<class Ar>(Ar& ar)
    {
        if constexpr(is_jwritter_v<Ar> || _buf_<Ar>)
        {
            _jobj_write(ar, unwrap_ref(kvs)...);
        }
        else
        {
            return _jobj_read(ar, unwrap_ref(kvs)...);
        }
    };
}


template<class... V>
auto jarr_(V&&... v)
{
    return [...v = decay_capture<V>(v)]<class Ar>(Ar& ar)
    {
        if constexpr(is_jwritter_v<Ar> || _buf_<Ar>)
        {
            make_jarr_writer(ar).elms(unwrap_ref(v)...);
        }
        else
        {
            return is_jarr(ar) && read_jarr(ar, unwrap_ref(v)...);
        }
    };
}



// usage
// jobj_(
//     "prim", prim,
//     "prim", jval_(prim, defaultValue),
//     "prim", jval_(prim, _ > 0),
//     "prim", jval_<int>(_ > 0),
//     "jo", jobj_(
//         "arr", jarr_(prim, functor, array, tuple, seq))
//     )
//     "arr", jarr_(jobj_(...), ...)
// )(jval);


// template<class JA>
// bool serialize_jval(JA& ja, YourType& t)
// {
//     return jobj_(
//         "prim", t.prim,
//         "prim", jval_(t.prim, defaultValue),
//         "prim", jval_(t.prim, _ > 0),
//         "prim", jval_<int>(_ > 0),
//         "jo", jobj_(
//             "arr", jarr_(t.prim, functor, array, tuple, seq)
//         )
//         "arr", jarr_(jobj_(...), ...)
//     )(ja);
// }

} // namespace jkl