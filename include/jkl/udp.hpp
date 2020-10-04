#pragma once

#include <jkl/config.hpp>
#include <jkl/conn.hpp>
#include <jkl/acceptor.hpp>
#include <jkl/resolver.hpp>
#include <boost/asio/ip/udp.hpp>


namespace jkl{


using udp_conn     = conn_t<asio::ip::udp::socket>;
using udp_acceptor = acceptor_t<asio::ip::udp, udp_conn>;
using udp_resolver = resolver_t<asio::ip::udp>


} // namespace jkl