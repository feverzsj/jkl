#pragma once

#include <jkl/config.hpp>
#include <jkl/util/buf.hpp>



namespace jkl{


// assign value/element
void assign_v(auto& d, auto&& v) requires(
                                    requires{ d = std::forward<decltype(v)>(v); }
                                 || requires{ d.clear(); d.emplace_back(std::forward<decltype(v)>(v)); }
                                 || requires{ d.clear(); d.push_back(std::forward<decltype(v)>(v)); }
                                 || requires{ resize_buf(d, 1); buf_front(d) = std::forward<decltype(v)>(v); })
{
    if constexpr(requires{ d = std::forward<decltype(v)>(v); })
    {
        d = std::forward<decltype(v)>(v);
    }
    else if constexpr(requires{ d.clear(); d.emplace_back(std::forward<decltype(v)>(v)); })
    {
        d.clear();
        d.emplace_back(std::forward<decltype(v)>(v));
    }
    else if constexpr(requires{ d.clear(); d.push_back(std::forward<decltype(v)>(v)); })
    {
        d.clear();
        d.push_back(std::forward<decltype(v)>(v));
    }
    else if constexpr(requires{ resize_buf(d, 1); buf_front(d) = std::forward<decltype(v)>(v); })
    {
        resize_buf(d, 1);
        buf_front(d) = std::forward<decltype(v)>(v);
    }
    else
    {
        JKL_DEPENDENT_FAIL(decltype(d), "unsupported type");
    }
}


// append value
decltype(auto) append_v(auto& d, auto&& v) requires(
                                                requires{ *d++ = std::forward<decltype(v)>(v); }
                                             || requires{ d.emplace_back(std::forward<decltype(v)>(v)); }
                                             || requires{ d.push_back(std::forward<decltype(v)>(v)); }
                                             || requires{ *buy_buf(d, 1) = std::forward<decltype(v)>(v); })
{
    if constexpr(requires{ *d++ = std::forward<decltype(v)>(v); })
    {
        *d++ = std::forward<decltype(v)>(v);
        return d;
    }
    else if constexpr(requires{ d.emplace_back(std::forward<decltype(v)>(v)); })
    {
        d.emplace_back(std::forward<decltype(v)>(v));
    }
    else if constexpr(requires{ d.push_back(std::forward<decltype(v)>(v)); })
    {
        d.push_back(std::forward<decltype(v)>(v));
    }
    else if constexpr(requires{ *buy_buf(d, 1) = std::forward<decltype(v)>(v); })
    {
        *buy_buf(d, 1) = std::forward<decltype(v)>(v);
    }
    else
    {
        JKL_DEPENDENT_FAIL(decltype(d), "unsupported type");
    }
}


// void assign_n(auto& d, std::input_iterator auto beg, std::iter_difference_t<declytpe(beg)> n)
void assign_n(auto& d, auto beg, std::integral auto n) requires(
                                                            requires{ d.assign(beg, n); }
                                                         || requires{ d.assign(beg, beg + n); }
                                                         || requires{ resize_buf(d, n); std::copy_n(beg, n, buf_data(d)); }
                                                         || requires{ d.clear(); std::copy_n(beg, n, std::back_inserter(d)); }
                                                         || requires{ d = {beg, n}; }
                                                         || requires{ d = {beg, beg + n}; })
{
    if constexpr(requires{ d.assign(beg, n); })
    {
        d.assign(beg, n);
    }
    else if constexpr(requires{ d.assign(beg, beg + n); })
    {
        d.assign(beg, beg + n);
    }
    else if constexpr(requires{ resize_buf(d, n); std::copy_n(beg, n, buf_data(d)); })
    {
        resize_buf(d, n);
        std::copy_n(beg, n, buf_data(d));
    }
    else if constexpr(requires{ d.clear(); std::copy_n(beg, n, std::back_inserter(d)); })
    {
        d.clear();
        std::copy_n(beg, n, std::back_inserter(d));
    }
    else if constexpr(requires{ d = {beg, n}; })
    {
        d = {beg, n};
    }
    else if constexpr(requires{ d = {beg, beg + n}; })
    {
        d = {beg, beg + n};
    }
    else
    {
        JKL_DEPENDENT_FAIL(decltype(d), "unsupported type");
    }
}

// void append_n(auto& d, std::input_iterator auto beg, std::iter_difference_t<declytpe(beg)> n)
decltype(auto) append_n(auto& d, auto beg, std::integral auto n) requires(
                                                                    requires{ std::copy_n(beg, n, d); }
                                                                 || requires{ d.append(beg, n); }
                                                                 || requires{ d.append(beg, beg + n); }
                                                                 || requires{ std::copy_n(beg, n, buy_buf(d, n)); }
                                                                 || requires{ std::copy_n(beg, n, std::back_inserter(d)); })
{
    if constexpr(requires{ std::copy_n(beg, n, d); })
    {
        return std::copy_n(beg, n, d);
    }
    else if constexpr(requires{ d.append(beg, n); })
    {
        d.append(beg, n);
    }
    else if constexpr(requires{ d.append(beg, beg + n); })
    {
        d.append(beg, beg + n);
    }
    else if constexpr(requires{ std::copy_n(beg, n, buy_buf(d, n)); })
    {
        std::copy_n(beg, n, buy_buf(d, n));
    }
    else if constexpr(requires{ std::copy_n(beg, n, std::back_inserter(d)); })
    {
        std::copy_n(beg, n, std::back_inserter(d));
    }
    else
    {
        JKL_DEPENDENT_FAIL(decltype(d), "unsupported type");
    }
}


// void assign_r(auto& d, std::input_iterator auto beg, decltype(beg) end)
void assign_r(auto& d, auto beg, auto end) requires(
                                                requires{ d.assign(beg, end); }
                                             || requires{ assign_n(d, beg, end - beg); }
                                             || requires{ d.clear(); std::copy(beg, end, std::back_inserter(d)); }
                                             || requires{ d = {beg, end}; })
{
    if constexpr(requires{ d.assign(beg, end); })
    {
        d.assign(beg, end);
    }
    else if constexpr(requires{ assign_n(d, beg, end - beg); })
    {
        assign_n(d, beg, end - beg);
    }
    else if constexpr(requires{ d.clear(); std::copy(beg, end, std::back_inserter(d)); })
    {
        d.clear();
        std::copy(beg, end, std::back_inserter(d));
    }
    else if constexpr(requires{ d = {beg, end}; })
    {
        d = {beg, end};
    }
    else
    {
        JKL_DEPENDENT_FAIL(decltype(d), "unsupported type");
    }
}

// void append_r(auto& d, std::input_iterator auto beg, decltype(beg) end)
decltype(auto) append_r(auto& d, auto beg, auto end) requires(
                                                        requires{ std::copy(beg, end, d); }
                                                     || requires{ d.append(beg, end); }
                                                     || requires{ append_n(d, beg, end - beg); }
                                                     || requires{ std::copy(beg, end, std::back_inserter(d)); })
{
    if constexpr(requires{ std::copy(beg, end, d); })
    {
        return std::copy(beg, end, d);
    }
    else if constexpr(requires{ d.append(beg, end); })
    {
        d.append(beg, end);
    }
    else if constexpr(requires{ append_n(d, beg, end - beg); })
    {
        append_n(d, beg, end - beg);
    }
    else if constexpr(requires{ std::copy(beg, end, std::back_inserter(d)); })
    {
        std::copy(beg, end, std::back_inserter(d));
    }
    else
    {
        JKL_DEPENDENT_FAIL(decltype(d), "unsupported type");
    }
}


//void assign_r(auto& d, std::ranges::input_range auto&& r)
void assign_r(auto& d, auto&& r)
{
    if constexpr(requires{ d.capacity() < r.capacity(); d = std::forward<decltype(r)>(r); })
    {
        if(d.capacity() < r.capacity()) // prefer larger capacity
        {
            d = std::forward<decltype(r)>(r);
            return;
        }
    }

    if constexpr(requires{ d.assign(std::forward<decltype(r)>(r)); })
    {
        d.assign(std::forward<decltype(r)>(r));
    }
    else
    {
        assign_r(d, std::begin(r), std::end(r));
    }
}

//void append_r(auto& d, std::ranges::input_range auto&& r)
decltype(auto) append_r(auto& d, auto&& r)
{
    if constexpr(requires{ d.append(std::forward<decltype(r)>(r)); })
    {
        d.append(std::forward<decltype(r)>(r));
    }
    else
    {
        return append_r(d, std::begin(r), std::end(r));
    }
}




/// append_rv: append ranges or values

decltype(auto) append_rv1(auto& d, auto&& rv)
{
    if constexpr(requires{ append_v(d, std::forward<decltype(rv)>(rv)); })
    {
        return append_v(d, std::forward<decltype(rv)>(rv));
    }
    else
    {
        return append_r(d, std::forward<decltype(rv)>(rv));
    }
}


decltype(auto) append_rv(auto* d, auto&&... rvs)
{
    // return (... , (d = append_rv1(d, std::forward<decltype(rvs)>(rvs)))); clang generated malfunction code that gave wrong init result for: aresult<char*> r = append_str(...);
    (... , (d = append_rv1(d, std::forward<decltype(rvs)>(rvs))));
    return d;
}

void append_rv(auto& d, auto&&... rvs)
{
    (... , append_rv1(d, std::forward<decltype(rvs)>(rvs)));
}


template<class T>
concept _range_ = requires(T& r){ std::begin(r); std::end(r); };

template<class T>
concept _random_access_range_ = _range_<T> && requires(T& r){ {std::end(r) - std::begin(r)} -> std::integral; };

template<_range_ R>
using range_value_t = std::remove_cvref_t<decltype(*std::begin(std::declval<R&>()))>;

template<class T, class R>
concept _similar_random_access_range_ = _random_access_range_<R> && _random_access_range_<T> && std::same_as<range_value_t<T>, range_value_t<R>>;

template<class T, class R>
concept _similar_random_access_range_or_val_ = _random_access_range_<R> && (std::same_as<T, range_value_t<R>>
                                                || (_random_access_range_<T> && std::same_as<range_value_t<T>, range_value_t<R>>));

constexpr auto range_size(auto const& r)
{
    if constexpr(requires{ std::size(r); })
        return std::size(r);
    else if constexpr(requires{ std::end(r) - std::begin(r); })
        return std::end(r) - std::begin(r);
    else
        JKL_DEPENDENT_FAIL(decltype(r), "unsupported type");
}


template<_random_access_range_ R> consteval size_t range_size_or_val_1(range_value_t<R> const&) noexcept { return 1; }
template<_random_access_range_ R> constexpr size_t range_size_or_val_1(_similar_random_access_range_<R> auto const& r) noexcept { return range_size(r); }

template<_resizable_buf_ B>
void append_rv(B& b, _similar_random_access_range_or_val_<B> auto&&... rvs)
{
    append_rv(buy_buf(b, (... + range_size_or_val_1<B>(rvs))), std::forward<decltype(rvs)>(rvs)...);
}

/// assign_rv: assign ranges or values

void assign_rv(auto& d, auto&& rv)
{
    if constexpr(requires{ assign_v(d, std::forward<decltype(rv)>(rv)); })
    {
        assign_v(d, std::forward<decltype(rv)>(rv));
    }
    else
    {
        assign_r(d, std::forward<decltype(rv)>(rv));
    }
}

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
