#pragma once

#include <jkl/config.hpp>
#include <jkl/util/str.hpp>
#include <chrono>


namespace jkl{


// since deduction guide doesn't support alias template, we have to write the class out
#define _JKL_UNIT_CLASS_DEF(NAME, SUFFIX, RATIO)                  \
    template<class T>                                             \
    class NAME : public std::chrono::duration<T, RATIO>           \
    {                                                             \
    public:                                                       \
        static constexpr char const* suffix() { return #SUFFIX; } \
        using base = std::chrono::duration<T, RATIO>;             \
        explicit NAME(T const& t) : base(t) {}                    \
        template<class Q, class U>                                \
        NAME(std::chrono::duration<Q, U> const& d) : base(d) {}   \
    };                                                            \
//     template<class T>                                             \
//     auto stringify(NAME<T> const& u)                              \
//     {                                                             \
//         return cat_as_str(u.count(), "_" #SUFFIX);                \
//     }                                                             \
/**/


// may fail silently, if To cannot represent From exactly
template<class To, class From>
constexpr To unit_cast(From const& f) { return std::chrono::duration_cast<typename To::base>(f); }

// may fail silently, if To doesn't include From's range
template<class To, class From>
constexpr To unit_floor(From const& f) { return std::chrono::floor<typename To::base>(f); }
template<class To, class From>
constexpr To unit_ceil(From const& f) { return std::chrono::ceil<typename To::base>(f); }
template<class To, class From>
constexpr To unit_round(From const& f) { return std::chrono::round<typename To::base>(f); }



_JKL_UNIT_CLASS_DEF(t_ns, ns, std::nano            )
_JKL_UNIT_CLASS_DEF(t_us, us, std::micro           )
_JKL_UNIT_CLASS_DEF(t_ms, ms, std::milli           )
_JKL_UNIT_CLASS_DEF(t_s , s , std::ratio<1>        )
_JKL_UNIT_CLASS_DEF(t_m , m , std::ratio<60>       )
_JKL_UNIT_CLASS_DEF(t_h , h , std::ratio<3600>     )
_JKL_UNIT_CLASS_DEF(t_d , d , std::ratio<3600 * 24>)


_JKL_UNIT_CLASS_DEF(_B_, B  ,std::ratio<1>)
_JKL_UNIT_CLASS_DEF(KiB, KiB, std::ratio<1024LL>)
_JKL_UNIT_CLASS_DEF(MiB, MiB, std::ratio<1024LL * 1024LL>)
_JKL_UNIT_CLASS_DEF(GiB, GiB, std::ratio<1024LL * 1024LL * 1024LL>)
_JKL_UNIT_CLASS_DEF(TiB, TiB, std::ratio<1024LL * 1024LL * 1024LL * 1024LL>)
_JKL_UNIT_CLASS_DEF(PiB, PiB, std::ratio<1024LL * 1024LL * 1024LL * 1024LL * 1024LL>)
_JKL_UNIT_CLASS_DEF(EiB, EiB, std::ratio<1024LL * 1024LL * 1024LL * 1024LL * 1024LL * 1024LL>)
// overflowed
// _JKL_UNIT_CLASS_DEF(ZiB, ZiB, std::ratio<1024LL * 1024LL * 1024LL * 1024LL * 1024LL * 1024LL * 1024LL>)
// _JKL_UNIT_CLASS_DEF(YiB, YiB, std::ratio<1024LL * 1024LL * 1024LL * 1024LL * 1024LL * 1024LL * 1024LL * 1024LL>)


} // namespace jkl