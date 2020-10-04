#pragma once


#include <jkl/config.hpp>
#include <jkl/util/hash.hpp>
#include <robin_hood.h>


namespace jkl{


// Comparators support Heterogeneous lookups:
// use any valid types for Comp.
// smart pointer keys can be compared with pointers.

template<class Comp>
struct heterogeneous_comparator
{
    using is_transparent = void;

    template<class L, class R>
    constexpr bool operator()(L const& l, R const& r) const noexcept(noexcept(Comp::apply(l, r)))
    {
        return Comp::apply(l, r);
    }

    template<class L, _smart_ptr_ R>
    constexpr bool operator()(L* l,  R const& r) const noexcept(noexcept(Comp::apply(l, r.get())))
    {
        return Comp::apply(l, r.get());
    }

    template<_smart_ptr_ L, class R>
    constexpr bool operator()(L const& l, R* r) const noexcept(noexcept(Comp::apply(l.get(), r)))
    {
        return Comp::apply(l.get(), r);
    }
};

struct less : heterogeneous_comparator<less>
{
    template<class L, class R>
    static constexpr bool apply(L const& l, R const& r) noexcept(noexcept(l < r))
    {
        return l < r;
    }
};

struct equal_to : heterogeneous_comparator<equal_to>
{
    template<class L, class R>
    static constexpr bool apply(L const& l, R const& r) noexcept(noexcept(l == r))
    {
        return l == r;
    }
};


} // namespace jkl
