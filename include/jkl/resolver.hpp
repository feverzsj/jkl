#pragma once


#include <jkl/config.hpp>
#include <jkl/ioc.hpp>
#include <jkl/ec_awaiter.hpp>

#include <boost/asio/ip/basic_resolver.hpp>


namespace jkl{


template<class Protocol, class Executor = asio::executor>
class resolver_t : public asio::ip::basic_resolver<Protocol, Executor>
{
    using base = asio::ip::basic_resolver<Protocol, Executor>;

public:
    using base::base;
    using base::operator=;

    resolver_t() : base(default_ioc()) {}

    template<class Host, class Service, class... P>
    auto resolve(Host const& host, Service const& service, P... p)
    {
        return resolve(host, service, flags(), p...);
    }

    template<class Host, class Service, class... P>
    auto resolve(Host const& host, Service const& service, flags f, P... p)
    {
        return make_ec_awaiter<results_type>(
            [&, host = stringify(host), service = stringify(service), f](auto&& h)
            {
                base::async_resolve(host, service, f, std::move(h));
            },
            p...);
    }

    template<class Host, class Service, class... P>
    auto resolve(protocol_type const& prtcl, Host const& host, Service const& service, P... p)
    {
        return resolve(prtcl, host, service, flags(), p...);
    }

    template<class Host, class Service, class... P>
    auto resolve(protocol_type const& prtcl, Host const& host, Service const& service, flags f, P... p)
    {
        return make_ec_awaiter<results_type>(
            [&, prtcl, host = stringify(host), service = stringify(service), f](auto&& h)
            {
                base::async_resolve(prtcl, host, service, f, std::move(h));
            },
            p...);
    }

    // reverse resolve: ip -> name
    template<class... P>
    auto resolve(endpoint_type const& ep, P... p)
    {
        return make_ec_awaiter<results_type>(
            [&, ep](auto&& h) { base::async_resolve(ep, std::move(h)); },
            p...);
    }
};



} // namespace jkl