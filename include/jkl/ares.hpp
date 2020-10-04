#pragma once

#include <jkl/config.hpp>
#include <jkl/error.hpp>
#include <jkl/util/str.hpp>
#include <jkl/util/endian.hpp>
#include <jkl/util/concepts.hpp>
#include <ares.h>


namespace jkl{


class ares_error_category : public aerror_category
{
public:
	char const* name() const noexcept { return "c-ares"; }
	std::string message(int rc) const noexcept { return ::ares_strerror(rc); }
};

inline ares_error_category g_ares_category;

inline auto& ares_category()
{
// 	static ares_error_category category;
// 	return category;
    return g_ares_error_category;
}

inline aerror_code make_ares_error(int c)
{
    return {c, ares_category()};
}

inline void throw_ares_error_on_failure(int e)
{
    if(e)
        throw asystem_error(make_ares_error(e));
}

// ares_library_init(ARES_LIB_INIT_ALL)
// ares_library_cleanup()


class ares_opts
{
    ares_options _o;
    int          _m = 0;

    void set_bit_if(bool cond, int b) noexcept
    {
        if(cond)
            _m |= b;
        else
            _m &= ~b;
    }

public:
    ares_opts() = default;
    ares_opts(ares_options& o, int m) : _o(o), _m(m) {}

    int mask() noexcept { return _m; }
    ares_options* handle() noexcept { return &_o; }

    void clear() noexcept { _m = 0; }
    void set_flags(int f) noexcept { set_bit_if(_o.flags = f, ARES_OPT_FLAGS); }
    void set_timeout_ms(int ms) noexcept { set_bit_if(_o.timeout = ms, ARES_OPT_TIMEOUTMS); }
    void set_tries(int n) noexcept { set_bit_if(_o.tries = n, ARES_OPT_TRIES); }
    void set_ndots(int n) noexcept { set_bit_if(_o.ndots = n, ARES_OPT_NDOTS); }
    void set_udp_port(uint16_t p) noexcept { set_bit_if(_o.udp_port = bendian::native_to_big(p), ARES_OPT_UDP_PORT); }
    void set_tcp_port(uint16_t p) noexcept { set_bit_if(_o.tcp_port = bendian::native_to_big(p), ARES_OPT_TCP_PORT); }
    
    void set_sock_sndbuf_size(int bytes) noexcept { set_bit_if(_o.socket_send_buffer_size    = bytes, ARES_OPT_SOCK_SNDBUF); }
    void set_sock_rcvbuf_size(int bytes) noexcept { set_bit_if(_o.socket_receive_buffer_size = bytes, ARES_OPT_SOCK_RCVBUF); }

    void set_ednspsz(int bytes) noexcept { set_bit_if(_o.ednspsz = bytes, ARES_OPT_EDNSPSZ); _o.flags |= ARES_FLAG_EDNS; }


//     ARES_OPT_SERVERS struct in_addr *servers;
//                             int nservers;
//         The list of IPv4 servers to contact, instead of the servers specified in resolv.conf or the local named. In order to allow specification of either IPv4 or IPv6 name servers, the ares_set_servers function must be used instead.
// 
//     ARES_OPT_DOMAINS char **domains;
//                      int ndomains;
//         The domains to search, instead of the domains specified in resolv.conf or the domain derived from the kernel hostname variable.
// 
//     ARES_OPT_LOOKUPS char *lookups;
//         The lookups to perform for host queries. lookups should be set to a string of the characters "b" or "f", where "b" indicates a DNS lookup and "f" indicates a lookup in the hosts file.
// 
//     ARES_OPT_SOCK_STATE_CB void (*sock_state_cb)(void *data, ares_socket_t socket_fd, int readable, int writable);
//                            void *sock_state_cb_data;
//         A callback function to be invoked when a socket changes state. socket_fd will be passed the socket whose state has changed; readable will be set to true if the socket should listen for read events, and writable will be set to true if the socket should listen for write events. The value of sock_state_cb_data will be passed as the data argument.
// 
//     ARES_OPT_SORTLIST struct apattern *sortlist;
//                       int nsort;
//         A list of IP address ranges that specifies the order of preference that results from ares_gethostbyname should be returned in. Note that this can only be used with a sortlist retrieved via ares_save_options (because struct apattern is opaque); to set a fresh sort list, use ares_set_sortlist.
// 
//     ARES_OPT_RESOLVCONF char *resolvconf_path;
//         The path to use for reading the resolv.conf file. The resolvconf_path should be set to a path string, and will be honoured on *nix like systems. The default is /etc/resolv.conf
//         The optmask parameter also includes options without a corresponding field in the ares_options structure, as follows:
//         ARES_OPT_ROTATE Perform round-robin selection of the nameservers configured for the channel for each resolution.
//         ARES_OPT_NOROTATE Do not perform round-robin nameserver selection; always use the list of nameservers in the same order. 
};

class ares_ch
{
    struct ch_deleter { void operator()(ares_channel ch) noexcept { ::ares_destroy(ch); } };
    std::unique_ptr<ares_channel, ch_deleter> _ch;

    ares_ch(ares_channel h) : _ch(h) {}

public:
    ares_ch()
    {
        ares_channel ch = nullptr;
        throw_ares_error_on_failure(::ares_init(&ch));
        _ch.reset(ch);
    }

    explicit ares_ch(ares_opts& opts)
    {
        ares_channel ch = nullptr;
        throw_ares_error_on_failure(::ares_init_options(&ch, opts.handle(), opts.mask()));
        _ch.reset(ch);
    }

    ares_channel handle() const noexcept { return _ch.get(); }

    ares_opts options()
    {
        ares_options* p = nullptr;
        int m = 0;
        throw_ares_error_on_failure(::ares_save_options(handle(), &p, &m));
        ares_opts opts(*p, m);
        ::ares_destroy_options(p);
        return opts;
    }

    ares_ch dup()
    {
        ares_channel h = nullptr;
        throw_ares_error_on_failure(::ares_dup(h, handle()));
        return {h};
    }

    void cancel() { ::ares_cancel(handle()); }

    template<class T>
    void set_local_ip4(T const& t)
    {
        if constexpr(_str_<T>)
        {
            unsigned d = 0;
            if(ares_inet_pton(AF_INET, as_cstr(t).data(), &d) != 1)
                throw std::runtime_error("invalid ip4 address");
            ares_set_local_ip4(handle(), d);
        }
        else
        {
            ares_set_local_ip4(handle(), t);
        }
    }

    template<class T>
    void set_local_ip6(T const& t)
    {
        if constexpr(_str_<T>)
        {
            unsigned char d[16];
            if(ares_inet_pton(AF_INET6, as_cstr(t).data(), d) != 1)
                throw std::runtime_error("invalid ip4 address");
            ares_set_local_ip6(handle(), d);
        }
        else
        {
            ares_set_local_ip6(handle(), t);
        }
    }

    template<class Str>
    void set_local_dev(Str const& name)
    {
        ares_set_local_dev(handle(), as_cstr(name).data());
    }
};



class ares_resolver
{
    using TIME_S = boost::posix_time::seconds;

public:
    explicit ares_resolver(asio::io_context& ioc = default_ioc())
        : io_context_(ioc), channel_(), timer_(io)
    {
        //printf("new channel\n");
        ares_options opts;
        opts.timeout = DNS_TIMEOUT;
        opts.tries = MAX_TRIES;
        ::ares_init_options(&channel_, &opts, ARES_OPT_TIMEOUT | ARES_OPT_TRIES);


        ares_addr_node node;
        node.next = nullptr;
        node.family = AF_INET;
        node.addr.addr4.s_addr = inet_addr("114.114.114.114");

        ::ares_set_servers(channel_, &node);
    }


    ~channel()
    {
        ares_destroy(channel_);
        printf("channel die\n");
    }

    ares_channel& get_channel_()
    {
        return channel_;
    }

    void FinishResolve()
    {
        timer_.cancel();
    }

    template<class CALLBACK>
    static void query_callback_wrapper(void* args, int status, int timeouts, unsigned char* abuf, int alen)
    {
        std::unique_ptr<CALLBACK> callback(static_cast<CALLBACK*>(args));
        (*callback)(status, timeouts, abuf, alen);
    }



    template<class CALLBACK>
    void query(const std::string &domain, int dnsclass, int type, const CALLBACK &callback)
    {
        auto arg = new CALLBACK(std::move(callback));

        ares_query(channel_, domain.c_str(), dnsclass, type, &query_callback_wrapper<CALLBACK>, arg);

        this->timer_.expires_from_now(TIME_S(2 * DNS_TIMEOUT));
        this->timer_.async_wait(boost::bind(&channel::handlerOnTimeUp<CALLBACK>,this->shared_from_this(),boost::asio::placeholders::error, arg));

        getsock();
    }

    void getsock()
    {
        boost::array<ares_socket_t, ARES_GETSOCK_MAXNUM> sockets_ = {{0}};
        int bitmask = ::ares_getsock(channel_, sockets_.data(), sockets_.size());
        for (int i = 0; i < ARES_GETSOCK_MAXNUM; ++i)
        {
            if (ARES_GETSOCK_READABLE(bitmask, i))
            {

                boost::asio::posix::stream_descriptor descriptor(io_context_);
                auto result = stream_descriptors_.emplace(sockets_[i], std::move(descriptor));
                if (result.second)
                {
                    assert(!result.first->second.is_open());
                    result.first->second.assign(sockets_[i]);
                }
                assert(result.first->second.is_open());
                if (ARES_GETSOCK_READABLE(bitmask, i))
                {
                    result.first->second.async_read_some(boost::asio::null_buffers(),
                        boost::bind(&channel::read_handler, shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred,
                            sockets_[i]));
                    //printf("ARES_GETSOCK_READABLE %d\n", sockets_[i]);

                }


                return;
            }
        }
    }




private:

    boost::asio::io_context &io_context_;
    boost::asio::deadline_timer timer_;
    unsigned int tries_ = 0;
    ares_channel channel_;

    typedef std::map<int, boost::asio::posix::stream_descriptor> stream_descriptors_type;
    stream_descriptors_type stream_descriptors_;

    template<class CALLBACK>
    void handlerOnTimeUp(const boost::system::error_code &ec, CALLBACK* callback)
    {
        if(ec)
        {
            //printf("handlerOnTimeUp err -> %s \n",ec.message().c_str());
            stream_descriptors_.begin()->second.cancel();
            return;
        }
        //printf("timeup\n");
        if(tries_ == MAX_TRIES + 1)
        {
            //printf("max tries\n");
            stream_descriptors_.begin()->second.cancel();
            return;
        }

        ++tries_;
        ::ares_process_fd(channel_, ARES_SOCKET_BAD, ARES_SOCKET_BAD);

        this->timer_.expires_from_now(TIME_S(2 * DNS_TIMEOUT));
        this->timer_.async_wait(boost::bind(&channel::handlerOnTimeUp<CALLBACK>,this->shared_from_this(),boost::asio::placeholders::error, callback));


    }

    void read_handler(const boost::system::error_code & ec, std::size_t size,
        ares_socket_t fd)
    {
        if(ec)
        {
            //printf("read_handler err -> %s \n",ec.message().c_str());
            return;
        }

        //printf("read_handler\n");

        ::ares_process_fd(channel_, fd, ARES_SOCKET_BAD);

    }

};

} // namespace jkl