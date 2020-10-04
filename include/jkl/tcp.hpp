#pragma once

#include <jkl/config.hpp>
#include <jkl/conn.hpp>
#include <jkl/acceptor.hpp>
#include <jkl/resolver.hpp>
#include <boost/asio/ip/tcp.hpp>


namespace jkl{


using tcp_conn     = conn_t<asio::ip::tcp::socket>;
using tcp_acceptor = acceptor_t<asio::ip::tcp, tcp_conn>;
using tcp_resolver = resolver_t<asio::ip::tcp>


} // namespace jkl