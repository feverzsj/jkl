#pragma once

#include <jkl/config.hpp>
#include <jkl/util/str.hpp>
#include <compare>


namespace jkl{


template<class C, size_t N>
struct strlit
{
    C arr[N + 1] = {};

    constexpr strlit(C const (&s)[N + 1]) noexcept
    {
        std::copy_n(s, N, arr);
    }

    template<size_t L, size_t R> requires(L + R == N)
    constexpr strlit(strlit<C, L> const& l, strlit<C, R> const& r) noexcept
    {
        l.copy(arr, L);
        r.copy(arr + L, R);
    }

    C& operator[](size_t i) noexcept { return arr[i]; }
    C const& operator[](size_t i) const noexcept { return arr[i]; }

    constexpr C*       data() noexcept { return arr; }
    constexpr C const* data() const noexcept { return arr; }
    static constexpr size_t size() noexcept { return N; }
    static constexpr bool  empty() noexcept { return N == 0; }
    constexpr C* begin() noexcept { return data(); }
    constexpr C* end  () noexcept { return data() + size(); }
    constexpr C const* begin() const noexcept { return data(); }
    constexpr C const* end  () const noexcept { return data() + size(); }

    template<size_t Pos = 0, size_t Count = size_t(-1)> requires(Pos <= N)
    constexpr auto substr() const noexcept
    {
        constexpr size_t M = std::min(Count, N - Pos);
        C s[M + 1] = {};
        copy(s, M, Pos);
        return strlit<C, M>{s};
    }

    template<size_t Count = size_t(-1)>
    constexpr auto left() const noexcept
    {
        return substr<0, Count>();
    }

    template<size_t Count = size_t(-1)>
    constexpr auto right() const noexcept
    {
        return substr<N - std::min(Count, N), Count>();
    }

    template<class StrView = basic_string_view<C>>
    constexpr StrView view() const noexcept
    {
        return {data(), size()};
    }

    template<class StrView = basic_string_view<C>>
    constexpr StrView subview(size_t pos = 0, size_t count = size_t(-1)) const
    {
        if(pos > size())
            throw std::out_of_range{"invalid pos"};
        return {data() + pos, std::min(count, size() - pos)};
    }

    constexpr size_t copy(C* dest, size_t count, size_t pos = 0) const
    {
        return subview(pos, count).copy(dest, count);
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr bool starts_with(auto&& u) const noexcept
    {
        return view().starts_with(JKL_FORWARD(u));
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr bool ends_with(auto&& u) const noexcept
    {
        return view().ends_with(JKL_FORWARD(u));
    }
};

template<class C, size_t N>
strlit(C const (&s)[N]) -> strlit<C, N - 1>;



template<class C, size_t L, size_t R>
constexpr strlit<C, L + R> operator+(strlit<C, L> const& l, strlit<C, R> const& r) noexcept
{
    return {l, r};
}

template<class C, size_t L, size_t R>
constexpr strlit<C, L + R - 1> operator+(C const (&l)[L], strlit<C, R> const& r) noexcept
{
    return {strlit{l}, r};
}

template<class C, size_t L, size_t R>
constexpr strlit<C, L + R - 1> operator+(strlit<C, L> const& l, C const (&r)[R]) noexcept
{
    return {l, strlit{r}};
}


} // namespace jkl
