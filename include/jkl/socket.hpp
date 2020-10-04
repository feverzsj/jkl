#pragma once

#include <jkl/config.hpp>
#include <jkl/buf.hpp>
#include <jkl/ioc.hpp>
#include <jkl/ec_awaiter.hpp>
#include <jkl/read_write.hpp>


namespace jkl{


template<class Protocol, class Executor = asio::executor>
class socket_t : public typename Protocol::socket::template rebind_executor<Executor>::other
{
    using base = typename Protocol::socket::template rebind_executor<Executor>::other;

public:
    using base::base;
    using base::operator=;

    socket_t() : base(default_ioc()) {}
    socket_t(Protocol const& p) : base(default_ioc(), p) {}

    base& astream() noexcept { return *this; }
    base const& astream() const noexcept { return *this; }

    template<class B, class... P>
    auto read_some(B&& b, P... p)
    {
        return jkl::read_some(astream(), std::forward<B>(b), p...);
    }

    template<class Buf, class... P>
    auto read_some_to_tail(Buf& b, size_t maxSize, P... p)
    {
        return jkl::read_some_to_tail(astream(), b, maxSize, p...);
    }

    template<class B, class... P>
    auto write_some(B&& b, P... p)
    {
        return jkl::write_some(astream(), std::forward<B>(b), p...);
    }

    template<class Buf, class... P>
    auto write_some_from(Buf& b, size_t pos, P... p)
    {
        return jkl::write_some_from(astream(), b, pos, p...);
    }


    template<class B, class CompCond, class... P>
    auto read(B&& b, CompCond&& c, P... p)
    {
        return jkl::read(astream(), std::forward<B>(b), std::forward<CompCond>(c), p...);
    }

    template<class B, class... P>
    auto read_to_fill(B&& b, P... p)
    {
        return jkl::read_to_fill(astream(), std::forward<B>(b), p...);
    }

    template<class Buf, class... P>
    auto read_to_tail(Buf& b, size_t size, P... p)
    {
        return jkl::read_to_tail(astream(), b, size, p...);
    }

    template<class B, class Match, class... P>
    auto read_until(B&& b, Match&& m, P... p)
    {
        return jkl::read_until(astream(), std::forward<B>(b), std::forward<Match>(m), p...);
    }
};


} // namespace jkl