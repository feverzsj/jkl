#pragma once

#include <tuple>
#include <type_traits>


namespace jkl::lambda_placeholders{


template<size_t N>
struct holder{ static constexpr size_t value = N; };


constexpr auto _1 = holder<0>{};
constexpr auto _2 = holder<1>{};
constexpr auto _3 = holder<2>{};
constexpr auto _4 = holder<3>{};
constexpr auto _5 = holder<4>{};
constexpr auto _6 = holder<5>{};

constexpr auto _  = _1;


// NOTE: all the op should be const

template<class F>
struct lambda_holder
{
    F f;

    constexpr explicit lambda_holder(F&& t) : f(std::move(t)) {}
    constexpr explicit lambda_holder(F const& t) : f(t) {}

    template<class...A>
    constexpr decltype(auto) operator()(A&&... a) const
    {
        auto t = std::forward_as_tuple(std::forward<A>(a)...);
        return f(t);
    }

    template<class Tuple>
    constexpr decltype(auto) apply(Tuple&& args) const
    {
        return f(std::forward<Tuple>(args));
    }
};



#define _JKL_LAMBDA_PLACEHOLDER_UNARY_OP(OP)                    \
    template<size_t I>                                          \
    constexpr auto operator OP (holder<I>)                      \
    {                                                           \
        return lambda_holder([](auto& args) -> decltype(auto)   \
        {                                                       \
            return OP std::get<I>(args);                        \
        });                                                     \
    }                                                           \
                                                                \
    template<class F>                                           \
    constexpr auto operator OP (lambda_holder<F> const& h)      \
    {                                                           \
        return lambda_holder([&](auto& args) -> decltype(auto)  \
        {                                                       \
            return OP h.apply(args);                            \
        });                                                     \
    }                                                           \
/**/

#define _JKL_LAMBDA_PLACEHOLDER_BINARY_OP(OP)                                         \
    template<class Q, size_t I>                                                       \
    constexpr auto operator OP (Q const& q, holder<I>)                                \
    {                                                                                 \
        return lambda_holder([&](auto& args) -> decltype(auto)                        \
        {                                                                             \
            return q OP std::get<I>(args);                                            \
        });                                                                           \
    }                                                                                 \
                                                                                      \
    template<size_t I, class Q>                                                       \
    constexpr auto operator OP (holder<I>, Q const& q)                                \
    {                                                                                 \
        return lambda_holder([&](auto& args) -> decltype(auto)                        \
        {                                                                             \
            return std::get<I>(args) OP q;                                            \
        });                                                                           \
    }                                                                                 \
                                                                                      \
    template<size_t I, size_t J>                                                      \
    constexpr auto operator OP (holder<I>, holder<J>)                                 \
    {                                                                                 \
        return lambda_holder([](auto& args) -> decltype(auto)                         \
        {                                                                             \
            return std::get<I>(args) OP std::get<J>(args);                            \
        });                                                                           \
    }                                                                                 \
                                                                                      \
    template<class Q, class F>                                                        \
    constexpr auto operator OP (Q const& q, lambda_holder<F> const& h)                \
    {                                                                                 \
        return lambda_holder([&](auto& args) -> decltype(auto)                        \
        {                                                                             \
            return q OP h.apply(args);                                                \
        });                                                                           \
    }                                                                                 \
                                                                                      \
    template<class F, class Q>                                                        \
    constexpr auto operator OP (lambda_holder<F> const& h, Q const& q)                \
    {                                                                                 \
        return lambda_holder([&](auto& args) -> decltype(auto)                        \
        {                                                                             \
            return h.apply(args) OP q;                                                \
        });                                                                           \
    }                                                                                 \
                                                                                      \
    template<size_t I, class F>                                                       \
    constexpr auto operator OP (holder<I>, lambda_holder<F> const& h)                 \
    {                                                                                 \
        return lambda_holder([&](auto& args) -> decltype(auto)                        \
        {                                                                             \
            return std::get<I>(args) OP h.apply(args);                                \
        });                                                                           \
    }                                                                                 \
                                                                                      \
    template<class F, size_t I>                                                       \
    constexpr auto operator OP (lambda_holder<F> const& h, holder<I>)                 \
    {                                                                                 \
        return lambda_holder([&](auto& args) -> decltype(auto)                        \
        {                                                                             \
            return h.apply(args) OP std::get<I>(args);                                \
        });                                                                           \
    }                                                                                 \
                                                                                      \
    template<class F, class E>                                                        \
    constexpr auto operator OP (lambda_holder<F> const& h, lambda_holder<E> const& g) \
    {                                                                                 \
        return lambda_holder([&](auto& args) -> decltype(auto)                        \
        {                                                                             \
            return h.apply(args) OP g.apply(args);                                    \
        });                                                                           \
    }                                                                                 \
/**/



// Arithmetic
_JKL_LAMBDA_PLACEHOLDER_UNARY_OP (+)
_JKL_LAMBDA_PLACEHOLDER_UNARY_OP (-)
_JKL_LAMBDA_PLACEHOLDER_BINARY_OP(+)
_JKL_LAMBDA_PLACEHOLDER_BINARY_OP(-)
_JKL_LAMBDA_PLACEHOLDER_BINARY_OP(*)
_JKL_LAMBDA_PLACEHOLDER_BINARY_OP(/)
_JKL_LAMBDA_PLACEHOLDER_BINARY_OP(%)

// Bitwise
_JKL_LAMBDA_PLACEHOLDER_UNARY_OP ( ~)
_JKL_LAMBDA_PLACEHOLDER_BINARY_OP( &)
_JKL_LAMBDA_PLACEHOLDER_BINARY_OP( |)
_JKL_LAMBDA_PLACEHOLDER_BINARY_OP( ^)
_JKL_LAMBDA_PLACEHOLDER_BINARY_OP(<<)
_JKL_LAMBDA_PLACEHOLDER_BINARY_OP(>>)

// Comparison
_JKL_LAMBDA_PLACEHOLDER_BINARY_OP(==)
_JKL_LAMBDA_PLACEHOLDER_BINARY_OP(!=)
_JKL_LAMBDA_PLACEHOLDER_BINARY_OP(< )
_JKL_LAMBDA_PLACEHOLDER_BINARY_OP(<=)
_JKL_LAMBDA_PLACEHOLDER_BINARY_OP(> )
_JKL_LAMBDA_PLACEHOLDER_BINARY_OP(>=)

// Logical
_JKL_LAMBDA_PLACEHOLDER_BINARY_OP(||)
_JKL_LAMBDA_PLACEHOLDER_BINARY_OP(&&)
_JKL_LAMBDA_PLACEHOLDER_UNARY_OP (! )

// Member access (array subscript is a member function)
_JKL_LAMBDA_PLACEHOLDER_UNARY_OP(*)

// Other (function call is a member function)

#undef _JKL_LAMBDA_PLACEHOLDER_UNARY_OP
#undef _JKL_LAMBDA_PLACEHOLDER_BINARY_OP


} // namespace jkl::lambda_placeholders
