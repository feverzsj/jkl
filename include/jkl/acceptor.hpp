#pragma once

#include <jkl/config.hpp>
#include <jkl/ioc.hpp>
#include <jkl/buf.hpp>
#include <jkl/ec_awaiter.hpp>
#include <boost/asio/basic_socket_acceptor.hpp>


namespace jkl{


template<
    class Protocol,
    class DefultSocket,
    class Executor = asio::executor,
>
class acceptor_t : public asio::ip::basic_socket_acceptor<Protocol, Executor>
{
    using base = asio::ip::basic_socket_acceptor<Protocol, Executor>;

public:
    using default_socket_type = rebind_executor_t<DefultSocket, Executor>;

    using base::base;
    using base::operator=;

    acceptor_t() : base(default_ioc()) {}
    explicit acceptor_t(protocol_type const& p) : base(default_ioc(), p) {}
    explicit acceptor_t(endpoint_type const& e, bool reuseAddr = true) : base(default_ioc(), e, reuseAddr) {}
    acceptor_t(endpoint_type const& e, native_handle_type const& h) : base(default_ioc(), e, h) {}

    template<class... P>
    auto accept(P... p)
    {
        return make_ec_awaiter<default_socket_type>(
            [&](auto&& h){ base::async_accept(std::move(h)); },

//             [&, peer = std::make_unique<default_socket_type>(base::get_executor())](auto&& h)
//             {            // ^^^^^^^^^^^ necessary, so the reference won't change
//                 base::async_accept(get_lowest_layer(*peer),
//                     [peer = std::move(peer), h = std::move(h)](auto& ec)
//                     {
//                         h(ec, std::move(*peer));
//                     }
//                 );
//             },

            p...
        );
    }

    template<class... P>
    auto accept(endpoint_type& peerEp, P... p)
    {
        return make_ec_awaiter<default_socket_type>(
            [&](auto&& h){ base::async_accept(exc, peerEp, std::move(h)); },
            p...
        );
    }

    // ExC: executor or execution context
    template<class ExC, class... P>
    auto accept_exc(ExC&& exc, P... p)
    {
        return make_ec_awaiter<typename default_socket_type::rebind_executor<ExC>::type>(
            [&](auto&& h){ base::async_accept(exc, std::move(h)); },
            p...
        );
    }

    template<class ExC, class... P>
    auto accept_exc(ExC&& exc, endpoint_type& peerEp, P... p)
    {
        return make_ec_awaiter<typename default_socket_type::rebind_executor<ExC>::type>(
            [&](auto&& h){ base::async_accept(exc, peerEp, std::move(h)); },
            p...
        );
    }

    template<class Socket, class... P>
    auto accept_peer(Socket& peer, P... p)
    {
        return make_ec_awaiter(
            [&](auto&& h){ base::async_accept(get_lowest_layer(peer), std::move(h)); },
            p...
        );
    }

    template<class Socket, class... P>
    auto accept_peer(Socket& peer, endpoint_type& peerEp, P... p)
    {
        return make_ec_awaiter(
            [&](auto&& h){ base::async_accept(get_lowest_layer(peer), peerEp, std::move(h)); },
            p...
        );
    }
};


} // namespace jkl
