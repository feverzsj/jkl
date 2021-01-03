#pragma once

#include <jkl/config.hpp>
#include <jkl/util/buf.hpp>
#include <iterator>


namespace jkl{

// MSVC_WORKAROUND
template<class D, class V>
concept __assign_v_cond1 = requires(D& d, V&& v){ d = JKL_FORWARD(v); };
template<class D, class V>
concept __assign_v_cond2 = requires(D& d, V&& v){ d.clear(); d.emplace_back(JKL_FORWARD(v)); };
template<class D, class V>
concept __assign_v_cond3 = requires(D& d, V&& v){ d.clear(); d.push_back(JKL_FORWARD(v)); };
template<class D, class V>
concept __assign_v_cond4 = requires(D& d, V&& v){ resize_buf(d, 1); buf_front(d) = JKL_FORWARD(v); };

// assign value/element
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
void assign_v(auto& d, auto&& v) requires(
                                    requires{ d = JKL_FORWARD(v); }
                                 || requires{ d.clear(); d.emplace_back(JKL_FORWARD(v)); }
                                 || requires{ d.clear(); d.push_back(JKL_FORWARD(v)); }
                                 || requires{ resize_buf(d, 1); buf_front(d) = JKL_FORWARD(v); })
{
    //if constexpr(requires{ d = JKL_FORWARD(v); })
    if constexpr(__assign_v_cond1<decltype(d), decltype(v)>)
    {
        d = JKL_FORWARD(v);
    }
    //else if constexpr(requires{ d.clear(); d.emplace_back(JKL_FORWARD(v)); })
    else if constexpr(__assign_v_cond2<decltype(d), decltype(v)>)
    {
        d.clear();
        d.emplace_back(JKL_FORWARD(v));
    }
    //else if constexpr(requires{ d.clear(); d.push_back(JKL_FORWARD(v)); })
    else if constexpr(__assign_v_cond3<decltype(d), decltype(v)>)
    {
        d.clear();
        d.push_back(JKL_FORWARD(v));
    }
    //else if constexpr(requires{ resize_buf(d, 1); buf_front(d) = JKL_FORWARD(v); })
    else if constexpr(__assign_v_cond4<decltype(d), decltype(v)>)
    {
        resize_buf(d, 1);
        buf_front(d) = JKL_FORWARD(v);
    }
    else
    {
        JKL_DEPENDENT_FAIL(decltype(d), "unsupported type");
    }
}

// MSVC_WORKAROUND
template<class D, class V>
concept __append_v_cond1 = requires(D& d, V&& v){ *d++ = JKL_FORWARD(v); };
template<class D, class V>
concept __append_v_cond2 = requires(D& d, V&& v){ d.emplace_back(JKL_FORWARD(v)); };
template<class D, class V>
concept __append_v_cond3 = requires(D& d, V&& v){ d.push_back(JKL_FORWARD(v)); };
template<class D, class V>
concept __append_v_cond4 = requires(D& d, V&& v){ *buy_buf(d, 1) = JKL_FORWARD(v); };

// append value
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
decltype(auto) append_v(auto& d, auto&& v) requires(
                                                requires{ *d++ = JKL_FORWARD(v); }
                                             || requires{ d.emplace_back(JKL_FORWARD(v)); }
                                             || requires{ d.push_back(JKL_FORWARD(v)); }
                                             || requires{ *buy_buf(d, 1) = JKL_FORWARD(v); })
{
    //if constexpr(requires{ *d++ = JKL_FORWARD(v); })
    if constexpr(__append_v_cond1<decltype(d), decltype(v)>)
    {
        *d++ = JKL_FORWARD(v);
        return d;
    }
    //else if constexpr(requires{ d.emplace_back(JKL_FORWARD(v)); })
    else if constexpr(__append_v_cond2<decltype(d), decltype(v)>)
    {
        d.emplace_back(JKL_FORWARD(v));
    }
    //else if constexpr(requires{ d.push_back(JKL_FORWARD(v)); })
    else if constexpr(__append_v_cond3<decltype(d), decltype(v)>)
    {
        d.push_back(JKL_FORWARD(v));
    }
    //else if constexpr(requires{ *buy_buf(d, 1) = JKL_FORWARD(v); })
    else if constexpr(__append_v_cond4<decltype(d), decltype(v)>)
    {
        *buy_buf(d, 1) = JKL_FORWARD(v);
    }
    else
    {
        JKL_DEPENDENT_FAIL(decltype(d), "unsupported type");
    }
}

// MSVC_WORKAROUND
template<class D, class B, class N>
concept __assign_n_cond1 = requires(D& d, B beg, N n){ d.assign(beg, n); };
template<class D, class B, class N>
concept __assign_n_cond2 = requires(D& d, B beg, N n){ d.assign(beg, beg + n); };
template<class D, class B, class N>
concept __assign_n_cond3 = requires(D& d, B beg, N n){ resize_buf(d, n); std::copy_n(beg, n, buf_data(d)); };
template<class D, class B, class N>
concept __assign_n_cond4 = requires(D& d, B beg, N n){ d.clear(); std::copy_n(beg, n, std::back_inserter(d)); };
template<class D, class B, class N>
concept __assign_n_cond5 = requires(D& d, B beg, N n){ d = {beg, n}; };
template<class D, class B, class N>
concept __assign_n_cond6 = requires(D& d, B beg, N n){ d = {beg, beg + n}; };

// void assign_n(auto& d, std::input_iterator auto beg, std::iter_difference_t<declytpe(beg)> n)
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
void assign_n(auto& d, auto beg, std::integral auto n) requires(
                                                            requires{ d.assign(beg, n); }
                                                         || requires{ d.assign(beg, beg + n); }
                                                         || requires{ resize_buf(d, n); std::copy_n(beg, n, buf_data(d)); }
                                                         || requires{ d.clear(); std::copy_n(beg, n, std::back_inserter(d)); }
                                                         || requires{ d = {beg, n}; }
                                                         || requires{ d = {beg, beg + n}; })
{
    //if constexpr(requires{ d.assign(beg, n); })
    if constexpr(__assign_n_cond1<decltype(d), decltype(beg), decltype(n)>)
    {
        d.assign(beg, n);
    }
    //else if constexpr(requires{ d.assign(beg, beg + n); })
    else if constexpr(__assign_n_cond2<decltype(d), decltype(beg), decltype(n)>)
    {
        d.assign(beg, beg + n);
    }
    //else if constexpr(requires{ resize_buf(d, n); std::copy_n(beg, n, buf_data(d)); })
    else if constexpr(__assign_n_cond3<decltype(d), decltype(beg), decltype(n)>)
    {
        resize_buf(d, n);
        std::copy_n(beg, n, buf_data(d));
    }
    //else if constexpr(requires{ d.clear(); std::copy_n(beg, n, std::back_inserter(d)); })
    else if constexpr(__assign_n_cond4<decltype(d), decltype(beg), decltype(n)>)
    {
        d.clear();
        std::copy_n(beg, n, std::back_inserter(d));
    }
    //else if constexpr(requires{ d = {beg, n}; })
    else if constexpr(__assign_n_cond5<decltype(d), decltype(beg), decltype(n)>)
    {
        d = {beg, n};
    }
    //else if constexpr(requires{ d = {beg, beg + n}; })
    else if constexpr(__assign_n_cond6<decltype(d), decltype(beg), decltype(n)>)
    {
        d = {beg, beg + n};
    }
    else
    {
        JKL_DEPENDENT_FAIL(decltype(d), "unsupported type");
    }
}

// MSVC_WORKAROUND
template<class D, class B, class N>
concept __append_n_cond1 = requires(D& d, B beg, N n){ std::copy_n(beg, n, d); };
template<class D, class B, class N>
concept __append_n_cond2 = requires(D& d, B beg, N n){ d.append(beg, n); };
template<class D, class B, class N>
concept __append_n_cond3 = requires(D& d, B beg, N n){ d.append(beg, beg + n); };
template<class D, class B, class N>
concept __append_n_cond4 = requires(D& d, B beg, N n){ std::copy_n(beg, n, buy_buf(d, n)); };
template<class D, class B, class N>
concept __append_n_cond5 = requires(D& d, B beg, N n){ std::copy_n(beg, n, std::back_inserter(d)); };

// clang workaround
template<class D, class B, class N>
concept __append_n_req = __append_n_cond1<D, B, N>
                      || __append_n_cond2<D, B, N>
                      || __append_n_cond3<D, B, N>
                      || __append_n_cond4<D, B, N>
                      || __append_n_cond5<D, B, N>;

// void append_n(auto& d, std::input_iterator auto beg, std::iter_difference_t<declytpe(beg)> n)
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
decltype(auto) append_n(auto& d, auto beg, std::integral auto n)// requires(
//                                                                     requires{ std::copy_n(beg, n, d); }
//                                                                  || requires{ d.append(beg, n); }
//                                                                  || requires{ d.append(beg, beg + n); }
//                                                                  || requires{ std::copy_n(beg, n, buy_buf(d, n)); }
//                                                                  || requires{ std::copy_n(beg, n, std::back_inserter(d)); })
                                                                requires(__append_n_req<decltype(d), decltype(beg), decltype(n)>)
{
    //if constexpr(requires{ std::copy_n(beg, n, d); })
    if constexpr(__append_n_cond1<decltype(d), decltype(beg), decltype(n)>)
    {
        return std::copy_n(beg, n, d);
    }
    //else if constexpr(requires{ d.append(beg, n); })
    else if constexpr(__append_n_cond2<decltype(d), decltype(beg), decltype(n)>)
    {
        d.append(beg, n);
    }
    //else if constexpr(requires{ d.append(beg, beg + n); })
    else if constexpr(__append_n_cond3<decltype(d), decltype(beg), decltype(n)>)
    {
        d.append(beg, beg + n);
    }
    //else if constexpr(requires{ std::copy_n(beg, n, buy_buf(d, n)); })
    else if constexpr(__append_n_cond4<decltype(d), decltype(beg), decltype(n)>)
    {
        std::copy_n(beg, n, buy_buf(d, n));
    }
    //else if constexpr(requires{ std::copy_n(beg, n, std::back_inserter(d)); })
    else if constexpr(__append_n_cond5<decltype(d), decltype(beg), decltype(n)>)
    {
        std::copy_n(beg, n, std::back_inserter(d));
    }
    else
    {
        JKL_DEPENDENT_FAIL(decltype(d), "unsupported type");
    }
}

// MSVC_WORKAROUND
template<class D, class B, class E>
concept __assign_r2_cond1 = requires(D& d, B beg, E end){ d.assign(beg, end); };
template<class D, class B, class E>
concept __assign_r2_cond2 = requires(D& d, B beg, E end){ assign_n(d, beg, end - beg); };
template<class D, class B, class E>
concept __assign_r2_cond3 = requires(D& d, B beg, E end){ d.clear(); std::copy(beg, end, std::back_inserter(d)); };
template<class D, class B, class E>
concept __assign_r2_cond4 = requires(D& d, B beg, E end){ d = {beg, end}; };

// void assign_r(auto& d, std::input_iterator auto beg, decltype(beg) end)
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
void assign_r(auto& d, auto beg, auto end) requires(
                                                requires{ d.assign(beg, end); }
                                             || requires{ assign_n(d, beg, end - beg); }
                                             || requires{ d.clear(); std::copy(beg, end, std::back_inserter(d)); }
                                             || requires{ d = {beg, end}; })
{
    //if constexpr(requires{ d.assign(beg, end); })
    if constexpr(__assign_r2_cond1<decltype(d), decltype(beg), decltype(end)>)
    {
        d.assign(beg, end);
    }
    //else if constexpr(requires{ assign_n(d, beg, end - beg); })
    else if constexpr(__assign_r2_cond2<decltype(d), decltype(beg), decltype(end)>)
    {
        assign_n(d, beg, end - beg);
    }
    //else if constexpr(requires{ d.clear(); std::copy(beg, end, std::back_inserter(d)); })
    else if constexpr(__assign_r2_cond3<decltype(d), decltype(beg), decltype(end)>)
    {
        d.clear();
        std::copy(beg, end, std::back_inserter(d));
    }
    //else if constexpr(requires{ d = {beg, end}; })
    else if constexpr(__assign_r2_cond4<decltype(d), decltype(beg), decltype(end)>)
    {
        d = {beg, end};
    }
    else
    {
        JKL_DEPENDENT_FAIL(decltype(d), "unsupported type");
    }
}

// MSVC_WORKAROUND
template<class D, class B, class E>
concept __append_r2_cond1 = requires(D& d, B beg, E end){ std::copy(beg, end, d); };
template<class D, class B, class E>
concept __append_r2_cond2 = requires(D& d, B beg, E end){ d.append(beg, end); };
template<class D, class B, class E>
concept __append_r2_cond3 = requires(D& d, B beg, E end){ append_n(d, beg, end - beg); };
template<class D, class B, class E>
concept __append_r2_cond4 = requires(D& d, B beg, E end){ std::copy(beg, end, std::back_inserter(d)); };
// clang workaround
template<class D, class B, class E>
concept __append_r2_req = __append_r2_cond1<D, B, E> ||
                          __append_r2_cond2<D, B, E> ||
                          __append_r2_cond3<D, B, E> ||
                          __append_r2_cond4<D, B, E>;


// void append_r(auto& d, std::input_iterator auto beg, decltype(beg) end)
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
decltype(auto) append_r(auto& d, auto beg, auto end)// requires(
//                                                         requires{ std::copy(beg, end, d); }
//                                                      || requires{ d.append(beg, end); }
//                                                      || requires{ append_n(d, beg, end - beg); }
//                                                      || requires{ std::copy(beg, end, std::back_inserter(d)); })
                                                    requires(__append_r2_req<decltype(d), decltype(beg), decltype(end)>)
{
    //if constexpr(requires{ std::copy(beg, end, d); })
    if constexpr(__append_r2_cond1<decltype(d), decltype(beg), decltype(end)>)
    {
        return std::copy(beg, end, d);
    }
    //else if constexpr(requires{ d.append(beg, end); })
    else if constexpr(__append_r2_cond2<decltype(d), decltype(beg), decltype(end)>)
    {
        d.append(beg, end);
    }
    //else if constexpr(requires{ append_n(d, beg, end - beg); })
    else if constexpr(__append_r2_cond3<decltype(d), decltype(beg), decltype(end)>)
    {
        append_n(d, beg, end - beg);
    }
    //else if constexpr(requires{ std::copy(beg, end, std::back_inserter(d)); })
    else if constexpr(__append_r2_cond4<decltype(d), decltype(beg), decltype(end)>)
    {
        std::copy(beg, end, std::back_inserter(d));
    }
    else
    {
        JKL_DEPENDENT_FAIL(decltype(d), "unsupported type");
    }
}

// MSVC_WORKAROUND
template<class D, class R>
concept __assign_r_cond1 = requires(D& d, R&& r){ d.capacity() < r.capacity(); d = JKL_FORWARD(r); };
template<class D, class R>
concept __assign_r_cond2 = requires(D& d, R&& r){ d.assign(JKL_FORWARD(r)); };

//void assign_r(auto& d, std::ranges::input_range auto&& r)
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
void assign_r(auto& d, auto&& r)
{
    //if constexpr(requires{ d.capacity() < r.capacity(); d = JKL_FORWARD(r); })
    if constexpr(__assign_r_cond1<decltype(d), decltype(r)>)
    {
        if(d.capacity() < r.capacity()) // prefer larger capacity
        {
            d = JKL_FORWARD(r);
            return;
        }
    }

    //if constexpr(requires{ d.assign(JKL_FORWARD(r)); })
    if constexpr(__assign_r_cond2<decltype(d), decltype(r)>)
    {
        d.assign(JKL_FORWARD(r));
    }
    else
    {
        assign_r(d, std::begin(r), std::end(r));
    }
}

// MSVC_WORKAROUND
template<class D, class R>
concept __append_r_cond1 = requires(D& d, R&& r){ d.append(JKL_FORWARD(r)); };

//void append_r(auto& d, std::ranges::input_range auto&& r)
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
decltype(auto) append_r(auto& d, auto&& r)
{
    //if constexpr(requires{ d.append(JKL_FORWARD(r)); })
    if constexpr(__append_r_cond1<decltype(d), decltype(r)>)
    {
        d.append(JKL_FORWARD(r));
    }
    else
    {
        return append_r(d, std::begin(r), std::end(r));
    }
}




/// append_rv: append ranges or values

// MSVC_WORKAROUND
template<class D, class RV>
concept __append_rv1_cond1 = requires(D& d, RV&& rv){ append_v(d, JKL_FORWARD(rv)); };

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
decltype(auto) append_rv1(auto& d, auto&& rv)
{
    //if constexpr(requires{ append_v(d, JKL_FORWARD(rv)); })
    if constexpr(__append_rv1_cond1<decltype(d), decltype(rv)>)
    {
        return append_v(d, JKL_FORWARD(rv));
    }
    else
    {
        return append_r(d, JKL_FORWARD(rv));
    }
}


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
decltype(auto) append_rv(auto* d, auto&&... rvs)
{
    // return (... , (d = append_rv1(d, std::forward<decltype(rvs)>(rvs)))); clang generated malfunction code that gave wrong init result for: aresult<char*> r = append_str(...);
    (... , (d = append_rv1(d, std::forward<decltype(rvs)>(rvs))));
    return d;
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
void append_rv(auto& d, auto&&... rvs)
{
    (... , append_rv1(d, std::forward<decltype(rvs)>(rvs)));
}


// MSVC_WORKAROUND
template<class R>
concept __range_size_cond1 = requires(R const& r){ std::size(r); };
template<class R>
concept __range_size_cond2 = requires(R const& r){ std::end(r) - std::begin(r); };

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr auto range_size(auto const& r)
{
    //if constexpr(requires{ std::size(r); })
    if constexpr(__range_size_cond1<decltype(r)>)
        return std::size(r);
    //else if constexpr(requires{ std::end(r) - std::begin(r); })
    else if constexpr(__range_size_cond2<decltype(r)>)
        return std::end(r) - std::begin(r);
    else
        JKL_DEPENDENT_FAIL(decltype(r), "unsupported type");
}


template<_random_access_range_ R> constexpr size_t range_size_or_val_1(range_value_t<R> const&) noexcept { return 1; }
template<_random_access_range_ R> constexpr size_t range_size_or_val_1(_similar_random_access_range_<R> auto const& r) noexcept { return range_size(r); }

template<_resizable_buf_ B>
void append_rv(B& b, _similar_random_access_range_or_val_<B> auto&&... rvs)
{
    append_rv(buy_buf(b, (... + range_size_or_val_1<B>(rvs))), std::forward<decltype(rvs)>(rvs)...);
}

/// assign_rv: assign ranges or values

// MSVC_WORKAROUND
template<class D, class RV>
concept __assign_rv_cond1 = requires(D& d, RV&& rv){ assign_v(d, JKL_FORWARD(rv)); };

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
void assign_rv(auto& d, auto&& rv)
{
    //if constexpr(requires{ assign_v(d, JKL_FORWARD(rv)); })
    if constexpr(__assign_rv_cond1<decltype(d), decltype(rv)>)
    {
        assign_v(d, JKL_FORWARD(rv));
    }
    else
    {
        assign_r(d, JKL_FORWARD(rv));
    }
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
void assign_rv(auto& d, auto&&... rvs)
{
    d.clear();
    append_rv(d, std::forward<decltype(rvs)>(rvs)...);
}

template<_resizable_buf_ B>
void assign_rv(B& d, _similar_random_access_range_or_val_<B> auto&&... rvs)
{
    resize_buf(d, (... + range_size_or_val_1<B>(rvs)));
    append_rv(buf_data(d), std::forward<decltype(rvs)>(rvs)...);
}


template<class D>
D cat_rv(auto&&... rvs)
{
    D d;
    append_rv(d, std::forward<decltype(rvs)>(rvs)...);
    return d;
}


} // namespace jkl
