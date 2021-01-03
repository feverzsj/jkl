#pragma once

#include <jkl/config.hpp>
#include <variant>


namespace jkl{


template<class... A>
struct variant_awaiter : std::variant<A...>
{
    using base = std::variant<A...>;

    using base::base;

    decltype(auto) await_ready  (         ) { return std::visit([    ](auto& a){ return a.await_ready  (    ); }, static_cast<base&>(*this)); }
    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    decltype(auto) await_suspend(auto coro) { return std::visit([coro](auto& a){ return a.await_suspend(coro); }, static_cast<base&>(*this)); }
    decltype(auto) await_resume (         ) { return std::visit([    ](auto& a){ return a.await_resume (    ); }, static_cast<base&>(*this)); }
};


} // namespace jkl
