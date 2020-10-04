#pragma once

#include <jkl/config.hpp>
#include <jkl/ioc.hpp>
#include <jkl/task.hpp>
#include <jkl/res_pool.hpp>
#include <jkl/curl/easy.hpp>
#include <jkl/curl/multi.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <atomic>
#include <memory>
#include <optional>


namespace jkl{


class curl_client
{
    enum state_e
    {
        state_reseted, // removed
        state_running, // added
        state_paused,  // added
        state_finished // removed, but not reseted, may have error
    };

    static constexpr bool is_added_state   (state_e st) noexcept { return st == state_running || st == state_paused; }
    static constexpr bool is_removed_state (state_e st) noexcept { return st == state_reseted || st == state_finished; }
    static constexpr bool is_runnning_state(state_e st) noexcept { return st == state_running; }
    static constexpr bool is_paused_state  (state_e st) noexcept { return st == state_paused; }
    static constexpr bool is_finished_state(state_e st) noexcept { return st == state_finished; }

    struct socket_data
    {
        typename asio::ip::tcp::socket::rebind_executor<asio::io_context::strand>::other skt;
        curl_socket_t cskt = 0;
        int acts = 0;

        explicit socket_data(asio::io_context::strand const& ex, curl_socket_t s, aerror_code& ec) noexcept
            : skt(ex), cskt(s)
        {
            sockaddr  sa = { .sa_family = AF_UNSPEC };
            socklen_t len = sizeof(sa);

            BOOST_VERIFY(getsockname(s, &sa, &len) == 0);

            switch(sa.sa_family)
            {
                case AF_INET:
                    skt.assign(asio::ip::tcp::v4(), s, ec);
                    break;
                case AF_INET6:
                    skt.assign(asio::ip::tcp::v6(), s, ec);
                    break;
                default:
                    ec = make_error_code(aerrc::address_family_not_supported);
            }
        }

        ~socket_data()
        {
            aerror_code ec;
            skt.shutdown(asio::socket_base::shutdown_both, ec);
        }
    };

public:
    class curl_request
    {
        friend class curl_client;

        curl_client& _cl;
        curl_easy _easy;
        socket_data* _sd = nullptr;

        curlist _headers;
        //char errBuf[CURL_ERROR_SIZE] = {};

        beast::flat_buffer _dbuf;

        aerror_code* _wec = nullptr; // pointer to awaiter's _ec, should not be used inside curl callbacks.
        std::coroutine_handle<> _coro;
        std::shared_ptr<std::atomic_flag> _resumed = std::make_shared<std::atomic_flag>();

        std::unique_ptr<std::atomic<state_e>> _state = std::make_unique<std::atomic<state_e>>(state_reseted);
        // ^^^^^^^^^^^^ make std::atomic moveable

        size_t _writeCbDataUsed = 0;

        state_e get_state(std::memory_order order = std::memory_order_relaxed) const
        {
            BOOST_ASSERT(_state);
            return _state->load(order);
        }

        void set_state(state_e s, std::memory_order order = std::memory_order_relaxed)
        {
            BOOST_ASSERT(_state);
            _state->store(s, order);
        }

        // called inside curl callback, i.e. in _strand
        void mark_pause_and_post_resume(size_t used)
        {
            if(_resumed->test_and_set(std::memory_order_relaxed))
                return;

            BOOST_ASSERT(is_runnning_state(get_state()));
            _writeCbDataUsed = used;
            set_state(state_paused);

            BOOST_ASSERT(! *_wec);
            asio::post(_cl._ioc, [this](){
                _coro.resume();
            });
        }

        // called inside curl callback, i.e. in _strand
        size_t release_writecb_data_used()
        {
            BOOST_ASSERT(is_runnning_state(get_state()) || is_paused_state(get_state()));
            return std::exchange(_writeCbDataUsed, 0);
        }

        void reset_all()
        {
            BOOST_ASSERT(is_removed_state(get_state()));

            _coro = nullptr;
            _wec = nullptr;
            _resumed->clear(std::memory_order_relaxed);

            _dbuf.clear();

            _headers.clear();
            _headers.append("Expect:"); // disable Expect 100 continue

            _easy.reset_opts();

            _easy.setopts(
                curlopt::priv_data(this),
                //curlopt::errbuf(errBuf),
                curlopt::httpheader(_headers),
                curlopt::suppress_connect_headers,
                curlopt::writefunc([](char*, size_t size, size_t nmemb, void*) -> size_t { return size * nmemb; })
                //curlopt::headerfunc([](char*, size_t size, size_t nmemb, void*) -> size_t { return size * nmemb; })
            );

            _sd = nullptr;

            _writeCbDataUsed = 0;
            set_state(state_reseted);
        }

        void begin_await()
        {
            _coro = nullptr;
            _wec = nullptr;
            _resumed->clear(std::memory_order_relaxed);
        }

        enum read_type
        {
            read_type_response,
            read_type_header,
            read_type_left_body,
            read_type_some_body
        };

        template<read_type RT, class Msg>
        class awaiter_base
        {
            static_assert(RT == read_type_response || RT == read_type_header);

            Msg& _msg;
            std::optional<beast::http::response_parser<typename Msg::body_type>> _parser;

        public:
            explicit awaiter_base(Msg& m) : _msg(m), _parser(std::move(m)) { /*_parser->header_limit(UINT_MAX);*/ }

            auto& parser() noexcept { return *_parser; }

            auto& reset_parser()
            {
                _msg = _parser->release();
                return _parser.emplace(std::move(_msg));
            }

            template<class Awaiter>
            bool init(Awaiter* w, state_e const st)
            {
                BOOST_ASSERT(is_removed_state(st));
                //if(! is_removed_state(st))
                //{
                //    w->_ec = aerrc::operation_not_permitted;
                //    return false;
                //}

                auto parser_cb = [](char* d, size_t size, size_t nmemb, void* wd) -> size_t
                {
                    Awaiter* w = reinterpret_cast<Awaiter*>(wd);
                    size_t n = size * nmemb;

                    if(n == 0)
                        return n;

                    if(w->parser().is_done())
                        w->reset_parser();

                    auto b = w->_cr._dbuf.prepare(n);
                    memcpy(b.data(), d, n);
                    w->_cr._dbuf.commit(n);

                    auto used = w->parser().put(w->_cr._dbuf.data(), w->_ec);

                    w->_cr._dbuf.consume(used);

                    if(w->_ec && w->_ec != beast::http::error::need_more)
                        return 0; // as 0 case is handled above, we'll use 0 to signal error. curl will return CURLE_WRITE_ERROR.

                    return n;
                };

                if constexpr(RT == read_type_response)
                {
                    w->_cr.setopts(
                        curlopt::header_to_write_cb,
                        curlopt::write_cb(parser_cb, w)
                    );
                }
                else if constexpr(RT == read_type_header)
                {
                    parser().skip(true); // parser.is_done() will return true when the header is done
                    w->_cr.setopts(
                        curlopt::header_cb(parser_cb, w),
                        curlopt::write_cb(
                            [](char*, size_t size, size_t nmemb, void* wd) -> size_t
                            {
                                Awaiter* w = reinterpret_cast<Awaiter*>(wd);

                                if(size * nmemb == 0 /*|| ! w->parser().is_header_done()*/ || w->_cr._dbuf.size())
                                    return 0;

                                w->_cr.mark_pause_and_post_resume(0);

                                return CURL_WRITEFUNC_PAUSE;
                            },
                            w
                        )
                    );
                }

                return true;
            }

            template<class Awaiter>
            aresult<> get_result(Awaiter* w)
            {
                if(! w->_ec)
                    parser().put_eof(w->_ec);
                _msg = parser().release();
                return w->_ec;
            }
        };

        template<class ResizableBuf>
        class awaiter_base<read_type_left_body, ResizableBuf>
        {
            ResizableBuf& _buf;

        public:
            explicit awaiter_base(ResizableBuf& b) : _buf(b) { clear_buf(_buf); }
            auto& buf() noexcept { return _buf; }
            void append_data(char* d, size_t n) { memcpy(buy_buf(buf(), n), d, n); }

            template<class Awaiter>
            bool init(Awaiter* w, state_e const st)
            {
                BOOST_ASSERT(is_removed_state(st) || is_paused_state(st));
                //if(! (is_removed_state(st) || is_paused_state(st)))
                //{
                //    w->_ec = aerrc::operation_not_permitted;
                //    return false;
                //}

                w->_cr.setopts(
                    curlopt::write_cb(
                        [](char* d, size_t size, size_t nmemb, void* wd) -> size_t
                        {
                            Awaiter* w = reinterpret_cast<Awaiter*>(wd);
                            size_t n = size * nmemb;

                            if(n == 0)
                                return n;

                            size_t prevUsed = w->_cr.release_writecb_data_used();

                            if(prevUsed > n)
                                return 0; // something wrong

                            w->append_data(d + prevUsed, n - prevUsed);
                            return n;
                        },
                        w
                    )
                );

                return true;
            }

            template<class Awaiter>
            aresult<> get_result(Awaiter* w)
            {
                return w->_ec;
            }
        };

        template<class T>
        class awaiter_base<read_type_some_body, T>
        {
            std::conditional_t<
                std::is_lvalue_reference_v<T>,
                std::reference_wrapper<T>, std::remove_cvref_t<T>> _b;
            size_t _avlBSize;
        public:
            explicit awaiter_base(T&& b) : _b(std::forward<T>(b)) { _avlBSize = buf_size(buf()); }
            auto& buf() noexcept { return unwrap_ref(_b); }
            std::pair<size_t, size_t> append_data(char* d, size_t n)
            {
                n = std::min(_avlBSize, n);
                memcpy(buf_data(buf()) + buf_size(buf()) - _avlBSize, d, n);
                return {_avlBSize -= n, n};
            }

            size_t fit_buf()
            {
                size_t n = buf_size(buf()) - _avlBSize;
                if constexpr(_resizable_buf_<decltype(buf())>)
                    resize_buf(buf(), n);
                return n;
            }

            template<class Awaiter>
            bool init(Awaiter* w, state_e const st)
            {
                BOOST_ASSERT(is_removed_state(st) || is_paused_state(st));
                //if(! (is_removed_state(st) || is_paused_state(st)))
                //{
                //    w->_ec = aerrc::operation_not_permitted;
                //    return false;
                //}

                w->_cr.setopts(
                    curlopt::write_cb(
                        [](char* d, size_t size, size_t nmemb, void* wd) -> size_t
                        {
                            Awaiter* w = reinterpret_cast<Awaiter*>(wd);
                            size_t n = size * nmemb;

                            if(n == 0)
                                return 0;

                            size_t prevUsed = w->_cr.release_writecb_data_used();

                            if(prevUsed > n)
                                return 0; // something wrong

                            size_t m = n - prevUsed;
                            auto [avl, copied] = w->append_data(d + prevUsed, m);
                            BOOST_ASSERT(copied <= m);

                            if(! avl && copied < m)
                            {
                                w->_cr.mark_pause_and_post_resume(copied);
                                return CURL_WRITEFUNC_PAUSE;
                            }

                            return n;
                        },
                        w
                    )
                );

                return true;
            }

            template<class Awaiter>
            auto get_result(Awaiter* w)
            {
                return aresult<size_t>(w->_ec, fit_buf());
            }
        };

        // curl will pass multiple responses to write_cb/header_cb when following redirects or tunneling.
        // tunneling headers can be suppressed with CURLOPT_SUPPRESS_CONNECT_HEADERS.
        // in other cases, we suppose there could be header only responses before the last response.
        // we'll skip intermediate responses and return only the last response.
        template<read_type RT, class MsgOrBuf, bool EnableStop>
        class awaiter : awaiter_base<RT, MsgOrBuf>
        {
            friend class awaiter_base<RT, MsgOrBuf>;
            using base = awaiter_base<RT, MsgOrBuf>;

            curl_request& _cr;
            aerror_code   _ec;

            [[no_unique_address]]
            not_null_if_t<
                EnableStop, std::optional<std::stop_callback<std::function<void()>>>
            > _stopCb;

        public:
            template<class U>
            awaiter(curl_request& cr, U&& mb)
                : base(std::forward<U>(mb)), _cr(cr)
            {
                _cr.begin_await();
            }

            bool await_ready() const noexcept { return false; }

            template<class Promise>
            bool await_suspend(std::coroutine_handle<Promise> c)
            {
                if constexpr(EnableStop)
                {
                    if(c.promise().stop_requested())
                    {
                        _ec = asio::error::operation_aborted;
                        return false;
                    }
                }

                state_e const st = _cr.get_state();

                if(! base::init(this, st))
                    return false;

                _cr._wec = &_ec;
                _cr._coro = c;

                // start or unpause the request
                asio::post(_cr._cl._strand, [=, this](){

                    if(is_removed_state(st))
                        _ec = _cr._cl._multi.add(_cr._easy).error(); // will set a time-out to trigger very soon
                    else
                        _ec = _cr._easy.pause(CURLPAUSE_CONT).error(); // unpause all

                    if(_ec)
                    {
                        asio::post(_cr._cl._ioc, [this](){
                            _cr._coro.resume();
                        });
                        return;
                    }

                    _cr.set_state(state_running);

                    if constexpr(EnableStop)
                    {
                        BOOST_ASSERT(! _stopCb);
                        _stopCb.emplace(c.promise().get_stop_token(),
                            [this, resumed = _cr._resumed]()
                            {
                                if(resumed->test_and_set(std::memory_order_relaxed))
                                    return;
                            
                                asio::post(_cr._cl._strand, [this](){
                                
                                    if(_cr._sd)
                                    {
                                        BOOST_ASSERT(_cr._sd->acts);
                                        _cr._sd->acts = 0;
                                        _cr._sd->skt.cancel();
                                        _cr._sd = nullptr;
                                    }

                                    _cr._cl._multi.remove(_cr._easy).throw_on_error();
                                    _cr.set_state(state_finished);

                                    _ec = asio::error::operation_aborted;
                                    asio::post(_cr._cl._ioc, [this](){
                                        _cr._coro.resume();
                                    });
                                });
                            }
                        );
                    }
                });

                return true;
            }

            auto await_resume()
            {
                //BOOST_ASSERT(is_removed_state(_cr.get_state()));
                return base::get_result(this);
            }
        };

        template<read_type RT, class MsgOrBuf, class... P>
        auto do_read(MsgOrBuf&& mb, P... p)
        {
            auto params = make_params(p..., p_disable_stop);
            using mbtype = std::conditional_t<std::is_lvalue_reference_v<MsgOrBuf>, std::remove_reference_t<MsgOrBuf>, MsgOrBuf>;
            return awaiter<RT, mbtype, params(t_stop_enabled)>(*this, std::forward<MsgOrBuf>(mb));
        }

    public:
        explicit curl_request(curl_client& cl)
            : _cl(cl)
        {
            // reset_all(); should be called in pool.on_acquire
        }

    #ifndef NDEBUG
        ~curl_request()
        {
            BOOST_ASSERT(! _state || ! is_runnning_state(get_state()));
        }

        // since we write out dtor, move control won't be implicitly-declared
        curl_request(curl_request&& r) = default;
        //curl_request& operator=(curl_request&&) = default;
    #endif

        template<class... T>
        void setopts(T&&... opts)
        {
            _easy.setopts(std::forward<T>(opts)...);
        }

        // still need to check error
        bool finished() const noexcept { return is_finished_state(get_state()); }

        template<class Msg, class... P>
        auto read_response(Msg& m, P... p)
        {
            return do_read<read_type_response>(m, p...);
        }

        template<class Msg, class... P>
        auto read_header(Msg& m, P... p)
        {
            return do_read<read_type_header>(m, p...);
        }

        template<class ResizableBuf, class... P>
        auto read_left_body(ResizableBuf& b, P... p)
        {
            return do_read<read_type_left_body>(b, p...);
        }

        template<class Buf, class... P>
        auto read_some_body(Buf&& b, P... p)
        {
            return do_read<read_type_some_body>(std::forward<Buf>(b), p...);
        }
    };

private:
    // NOTE: all curl callbacks, anything involving _multi, _easy should go through _strand
    asio::io_context&  _ioc;
    asio::io_context::strand _strand{_ioc}; // mutex could lock multiple threads under high contention, while strand would minimize that.
                                            // Strand may add some (slight?) latency, since the extra cost and
                                            // it still need be executed on the underlying executor.
                                            // In general, mutex is preferred when the contention is low(short critical section, low concurrency, etc.)
                                            // or (extremely?) low latency is necessary. Strand is preferred, when the contention is high and you don't want
                                            // to block other threads for too long and the extra latency is acceptable.
    typename asio::steady_timer::rebind_executor<asio::io_context::strand>::other _timer{_strand};

    curl_multi _multi;
    unordered_flat_set<std::unique_ptr<socket_data>> _sds;
    res_pool<curl_request> _pool;

    template<class F>
    void strand_post(F&& f) { asio::post(_strand, std::forward(std::forward<F>(f))); }
    template<class F>
    void ioc_post(F&& f) { asio::post(_ioc, std::forward(std::forward<F>(f))); }

    int process_socket_action(curl_socket_t cskt, int acts)
    {
        //                        vvvvvvvvvvvvv invokes other curl callbacks
        int stillRunning = _multi.socket_action(cskt, acts).value_or_throw();

        while(auto msg = _multi.next_done())
        {
            curl_request* cr = msg->easy.priv_data<curl_request*>();
            msg->easy.release();

            if(cr->_resumed->test_and_set(std::memory_order_relaxed))
                continue;
            
            if(! *(cr->_wec))
                *(cr->_wec) = msg->err;

            asio::post(_strand, [cr](){

                BOOST_ASSERT(cr->get_state() == state_running);
                cr->_cl._multi.remove(cr->_easy).throw_on_error();
                cr->set_state(state_finished);

                asio::post(cr->_cl._ioc, [cr](){
                    cr->_coro.resume();
                });
            });
        }

        if(stillRunning <= 0)
            _timer.cancel();

        return stillRunning;
    }

    void event_cb(curl_request* cr, socket_data* sd, aerror_code const& ec, int act)
    {
        if(ec == asio::error::operation_aborted || ! cr->_sd)
            return;

        BOOST_ASSERT(cr->_sd == sd);
        BOOST_ASSERT(sd->acts & act);

        int stillRunning = process_socket_action(sd->cskt, ec ? CURL_CSELECT_ERR : act);

        if(ec || ! stillRunning)
        {
            sd->acts &= ~act;

            if(! sd->acts)
                cr->_sd = nullptr;

            return;
        }

        if(! _sds.contains(sd)) // sd may be removed by _multi.socket_action() -> multi_sock_cb()
            return;

        BOOST_ASSERT(act == CURL_POLL_IN || act == CURL_POLL_OUT);

        if(act == CURL_POLL_IN)
            sd->skt.async_wait(asio::socket_base::wait_read, [this, cr, sd](auto&& ec){
                event_cb(cr, sd, ec, CURL_POLL_IN);
            });
        else // if(act == CURL_POLL_OUT)
            sd->skt.async_wait(asio::socket_base::wait_write, [this, cr, sd](auto&& ec){
                event_cb(cr, sd, ec, CURL_POLL_OUT);
            });
    }

    static int multi_sock_cb(CURL* e, curl_socket_t cskt, int acts, void* cbp, void* sockp)
    {
        curl_easy ce(e);
        auto* cr = ce.priv_data<curl_request*>();
        ce.release();

        auto* cl = reinterpret_cast<curl_client*>(cbp);
        auto* sd = reinterpret_cast<socket_data*>(sockp);

        aerror_code ec;

        switch(acts)
        {
            case CURL_POLL_IN:
            case CURL_POLL_OUT:
            case CURL_POLL_INOUT:
                {
                    if(! sd)
                    {
                        auto usd = std::make_unique<socket_data>(cl->_strand, cskt, ec);

                        if(ec)
                            break;

                        sd = usd.get();

                        //BOOST_VERIFY(cl->_sds.emplace(std::move(usd)).second);
                        cl->_sds.emplace(std::move(usd));

                        cl->_multi.assign(cskt, sd).throw_on_error();
                    }

                    sd->acts |= acts;
                    cr->_sd = sd;
                    
                    if(acts & CURL_POLL_IN)
                        sd->skt.async_wait(asio::socket_base::wait_read, [cl, cr, sd](auto&& ec){
                            cl->event_cb(cr, sd, ec, CURL_POLL_IN);
                        });

                    if(acts & CURL_POLL_OUT)
                        sd->skt.async_wait(asio::socket_base::wait_write, [cl, cr, sd](auto&& ec){
                            cl->event_cb(cr, sd, ec, CURL_POLL_OUT);
                        });
                }
                break;
            case CURL_POLL_REMOVE:
                BOOST_ASSERT(sd);
                BOOST_ASSERT(cl->_sds.contains(sd));
                //BOOST_ASSERT(! sd->acts);
                BOOST_ASSERT(cr->_sd == sd);

                cr->_sd = nullptr;

                if(! sd)
                    throw std::runtime_error("multi_sock_cb: null sd for remove");

                if(auto it = cl->_sds.find(sd); it != cl->_sds.end())
                    cl->_sds.erase(it);
                else
                    throw std::runtime_error("multi_sock_cb: untracked socket_data");

                cl->_multi.assign(cskt, nullptr).throw_on_error();

                break;
            default:
                BOOST_ASSERT(false);
                throw std::runtime_error("multi_sock_cb: unknown action");
        }

        if(ec && ! cr->_resumed->test_and_set(std::memory_order_relaxed))
        {
            *(cr->_wec) = ec;

            asio::post(cl->_strand, [cr](){
                
                BOOST_ASSERT(cr->get_state() == state_running);
                cr->_cl._multi.remove(cr->_easy).throw_on_error();
                cr->set_state(state_finished);
                
                asio::post(cr->_cl._ioc, [cr](){
                    cr->_coro.resume();
                });
            });
        }

        return CURLM_OK;
    }

    static int multi_timer_cb(CURLM*, long ms, void* ud)
    {
        curl_client* cl = reinterpret_cast<curl_client*>(ud);

        cl->_timer.cancel();

        if(ms > 0)
        {
            cl->_timer.expires_after(std::chrono::milliseconds(ms));
            cl->_timer.async_wait([cl](auto&& ec){
                if(! ec)
                    cl->process_socket_action(CURL_SOCKET_TIMEOUT, 0);
            });
        }
        else if(ms == 0)
        {
            asio::post(cl->_strand, [cl](){
                cl->process_socket_action(CURL_SOCKET_TIMEOUT, 0);
            });
        }

        return CURLM_OK;
    }

public:
    explicit curl_client(size_t cap, asio::io_context& ctx = default_ioc())
        : _ioc(ctx), _pool(cap, [this](){ return curl_request(*this); })
    {
        _pool.on_acquire([](curl_request& cr){ cr.reset_all(); });

        _multi.setopts(
            curlmopt::socket_cb(multi_sock_cb, this),
            curlmopt::timer_cb(multi_timer_cb, this)
        );
    }

    ~curl_client()
    {
        BOOST_ASSERT(_sds.empty());
    }

    curl_client(curl_client const&) = delete;
    curl_client& operator=(curl_client const&) = delete;
    curl_client(curl_client&&) = delete;
    curl_client& operator=(curl_client&&) = delete;

    auto& io_ctx() noexcept { return _ioc; }

    // do not call clear_unused() when running
    auto& pool() noexcept { return _pool; }

    template<class... P>
    auto acquire_request(P... p)
    {
        return _pool.acquire(p...);
    }
};


} // namespace jkl
