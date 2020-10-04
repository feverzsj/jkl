#pragma once

#include <jkl/config.hpp>
#include <jkl/util/stringify.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <skyr/url.hpp>


namespace jkl{


class aurl : public skyr::url
{
public:
    std::optional<asio::ip::address> ip_address() const
    {
        if(auto a = ipv4_address())
            return asio::ip::address_v4(a->to_bytes());
        if(auto a = ipv6_address())
            return asio::ip::address_v6(a->to_bytes());
        return std::nullopt;
    }

    string_type real_port_or_protocol() const
    {
        auto p = port();

        if(p.size())
            return p;

        p = protocol();

        auto d = default_port(p);
        if(d)
            return stringify(*d);

        return p;
    }

    std::optional<uint16_t> real_port_num() const
    {
        if(p = port<uint16_t>())
            return p;
        return default_port(protocol());
    }

    template<class Protocol = asio::ip::tcp>
    std::optional<typename Protocol::endpoint> endpoint() const
    {
        auto a = ip_address();
        auto p = real_port_num();

        if(a && p)
            return {*a, *p};
        return std::nullopt;
    }
};



} // namespace jkl