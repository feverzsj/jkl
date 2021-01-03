#pragma once

#include <jkl/config.hpp>
#include <jkl/ioc.hpp>
#include <jkl/buf.hpp>
#include <jkl/task.hpp>
#include <jkl/params.hpp>
#include <jkl/res_pool.hpp>
#include <jkl/http_msg.hpp>
#include <jkl/curl/easy.hpp>
#include <jkl/curl/multi.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <atomic>
#include <memory>
#include <optional>


namespace jkl{


class curl_fields : public curlist
{
public:
    curl_fields() = default;

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    curl_fields(auto const&... kvs) { append_field(kvs...); };

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    void append_field1(_str_ auto const& k, _str_ auto const& v)
    {
        curlist::append1(cat_as_str(k, ": ", v));
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    void append_field1(beast::http::field k, _str_ auto const& v)
    {
        append_field1(beast::http::to_string(k), v);
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    void append_field(auto const& k, _str_ auto const& v, auto const&... kvs)
    {
        append_field1(k, v);
        append_field(kvs...);
    }
};


template<bool ReadSome, class B>
class cr_buf_wrapper
{
public:
    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    explicit cr_buf_wrapper(auto&&){}
    static consteval bool is_buf() { return false; }
};

template<bool ReadSome, _byte_buf_ B>
class cr_buf_wrapper<ReadSome, B>
{
    static constexpr bool is_resizable_buf = _resizable_buf_<B>;

    lref_or_val_t<B> _b;

    JKL_DEF_MEMBER_IF((ReadSome || ! is_resizable_buf), size_t, _curSize) = 0;

public:
    explicit cr_buf_wrapper(B&& t) : _b(std::forward<B>(t)) {}

    static consteval bool is_buf() { return true; }
    static consteval bool is_resizable() { return is_resizable_buf; }

    auto* data() noexcept { return buf_data(_b); }

    size_t size() const noexcept
    {
        if constexpr(ReadSome || ! is_resizable_buf)
            return _curSize;
        else // if constexpr(! ReadSome && is_resizable_buf)
            return buf_size(_b);
    }

    void clear() noexcept
    {
        if constexpr(ReadSome || ! is_resizable_buf)
            _curSize = 0;
        else // if constexpr(! ReadSome && is_resizable_buf)
            clear_buf(_b);
    }

    void remove_suffix(size_t n)
    {
        BOOST_ASSERT(n <= size());

        if constexpr(ReadSome || ! is_resizable_buf)
            _curSize -= n;
        else // if constexpr(! ReadSome && is_resizable_buf)
            resize_buf(_b, buf_size(_b) - n);
    }

    auto append(char* d, size_t n)
    {
        if constexpr(ReadSome)
        {
            size_t avl = buf_size(_b) - _curSize;
            n = std::min(avl, n);
            memcpy(buf_data(_b) + _curSize, d, n);
            _curSize += n;
            return n;
        }
        else if constexpr(! is_resizable_buf)
        {
            if(_curSize + n > buf_size(_b))
                return false;
            memcpy(buf_data(_b) + _curSize, d, n);
            _curSize += n;
            return true;
        }
        else // if constexpr(! ReadSome && is_resizable_buf)
        {
            memcpy(buy_buf(_b, n), d, n);
            return true;
        }
    }
};


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
        asio::ip::tcp::socket skt;
        curl_socket_t cskt = 0;
        int acts = 0;
        size_t id = 0;

        explicit socket_data(asio::io_context& ioc) noexcept
            : skt{ioc}
        {}

        ~socket_data() { release(); }

        // since we write out dtor, move control won't be implicitly-declared
        socket_data(socket_data&&) = default;
        socket_data& operator=(socket_data&&) = default;

        bool hold_socket() const { return skt.is_open(); }

        aerror_code assign(curl_socket_t s) noexcept
        {
            BOOST_ASSERT(! hold_socket());

            cskt = s;

            sockaddr  sa  = { .sa_family = AF_UNSPEC };
            socklen_t len = sizeof(sa);

            if(getsockname(s, &sa, &len) != 0)
                return aerror_code{errno, boost::system::generic_category()};

            aerror_code ec = make_error_code(aerrc::address_family_not_supported);

            if(     AF_INET  == sa.sa_family) skt.assign(asio::ip::tcp::v4(), s, ec);
            else if(AF_INET6 == sa.sa_family) skt.assign(asio::ip::tcp::v6(), s, ec);

            return ec;
        }

        // also cancel all outstanding action
        // socket open/close are left to curl
        // we can use CURLOPT_OPENSOCKETFUNCTION/CURLOPT_CLOSESOCKETFUNCTION, but they are unstable.
        aerror_code release()
        {
            aerror_code ec;

            if(hold_socket())
            {
                skt.release(ec);
                cskt = 0;
                acts = 0;
            }

            return ec;
        }
    };

public:
    class curl_request
    {
        friend class curl_client;

        struct data_awaiter_base
        {
            curl_request&           _cr;
            std::coroutine_handle<> _coro;
            aerror_code             _ec;

            explicit data_awaiter_base(curl_request& cr) : _cr{cr} {}
        };

        curl_client& _cl;
        curl_easy    _easy;

        curl_fields _fields;
        //char errBuf[CURL_ERROR_SIZE] = {};

        data_awaiter_base* _aw = nullptr;
        state_e _state = state_reseted;

        enum pause_type{ pause_type_none, pause_type_hd, pause_type_wd };
        pause_type _pauseType = pause_type_none;
        size_t _writeCbDataUsed = 0;

        aerror_code& awaiter_ec() { BOOST_ASSERT(_aw); return _aw->_ec; };
        std::coroutine_handle<> awaiter_coro() const { BOOST_ASSERT(_aw); return _aw->_coro; };

        state_e get_state(std::memory_order order = std::memory_order_relaxed) noexcept
        {
            return std::atomic_ref{_state}.load(order);
        }

        void set_state(state_e s, std::memory_order order = std::memory_order_relaxed) noexcept
        {
            std::atomic_ref{_state}.store(s, order);
        }

        void async_resume_coro()
        {
            _cl.ioc_post([this](){ awaiter_coro().resume(); });
        }

        // should already inside critical section when get called
        void try_remove_and_schedule_resume(aerror_code const& ec)
        {
            BOOST_ASSERT(get_state() == state_running);

            if(! awaiter_ec())
                awaiter_ec() = ec;

            _cl._multi.remove(_easy).throw_on_error();

            set_state(state_finished);

            async_resume_coro();
        }

        template<pause_type PauseType> requires(PauseType != pause_type_none)
        void try_mark_pause_and_schedule_resume(size_t used)
        {
            BOOST_ASSERT(is_runnning_state(get_state()));
            _pauseType = PauseType;
            _writeCbDataUsed = used;
            set_state(state_paused);

            BOOST_ASSERT(! awaiter_ec());

            _cl.ioc_post([this](){
                std::lock_guard lg{_cl._mtx}; // must be serialized, as curl may be still running when this get invoked.
                async_resume_coro();
            });
        }

        template<pause_type PauseType> requires(PauseType != pause_type_none)
        size_t release_data_used_on_pause() noexcept
        {
            BOOST_ASSERT(is_runnning_state(get_state()) || is_paused_state(get_state()));
            BOOST_ASSERT(_pauseType == PauseType || (_pauseType == pause_type_none && _writeCbDataUsed == 0));
            return std::exchange(_writeCbDataUsed, 0);
        }

        template<pause_type PauseType> requires(PauseType != pause_type_none)
        void clear_pause_if_not() noexcept
        {
            if(_pauseType != PauseType)
            {
                _pauseType = pause_type_none;
                _writeCbDataUsed = 0;
            }
        }

        void reset_all()
        {
            BOOST_ASSERT(is_removed_state(get_state()));

            reset_fields();

            _easy.reset_opts();

            _easy.setopts(
                curlopt::nosignal,
                curlopt::priv_data(this),
                //curlopt::errbuf(errBuf),
                curlopt::followlocation,
                curlopt::suppress_connect_headers,
                curlopt::disbale_write_cb
                //,curlopt::disbale_header_cb
            );

            _pauseType = pause_type_none;
            _writeCbDataUsed = 0;
            set_state(state_reseted);

            _cl.apply_default_opts(*this);
        }

        void on_recycle()
        {
            if(is_paused_state(get_state()))
            {
                std::lock_guard lg{_cl._mtx};
                _cl._multi.remove(_easy).throw_on_error();
                set_state(state_finished);
            }

            reset_all();
        }

        enum read_type
        {
            // NOTE: hd here means bytes passed to CURLOPT_HEADERFUNCTION, for http protocol, it's header and trailer
            //       wd are bytes passed to CURLOPT_WRITEFUNCTION, for http protocol, it's body
            read_type_hd_wd,
            read_type_hd,
            read_type_wd,
            read_type_hd_until_wd,
            read_type_wd_until_hd,
        };


        // curl will pass multiple responses to write_cb/header_cb when following redirects or tunneling.
        // tunneling headers can be suppressed with CURLOPT_SUPPRESS_CONNECT_HEADERS.
        // in other cases, we suppose there could be header only responses before the last response.
        // we'll skip intermediate responses and return only the last response.
        template<read_type RT, _byte_buf_ B, class B2, unsigned ReadSomeBits, bool EnableStop>
        class data_awaiter : private data_awaiter_base
        {
            cr_buf_wrapper<ReadSomeBits & 1, B> _bw;
            [[no_unique_address]] cr_buf_wrapper<(ReadSomeBits & 2) != 0, B2> _bw2;

            JKL_DEF_MEMBER_IF(EnableStop, optional_stop_callback<>, _stopCb       );
            JKL_DEF_MEMBER_IF(EnableStop, bool                    , _stopRequested) = false;

            constexpr bool exam_stop()
            {
                if constexpr(EnableStop)
                {
                    return false;
                }
                else
                {
                    if(std::atomic_ref{_stopRequested}.load(std::memory_order_relaxed))
                    {
                        _ec = asio::error::operation_aborted;
                        return true;
                    }
                    return false;
                }
            }

            template<size_t I> requires(I < 3)
            auto& get_bw() noexcept
            {
                if constexpr(I == 1)
                    return _bw;
                else
                    return _bw2;
            }

            template<size_t Bw, pause_type PauseType> requires((Bw == 1 || Bw == 2) && PauseType != pause_type_none)
            static size_t resumed_write_to_bw(char* d, size_t size, size_t nmemb, void* wd)
            {
                data_awaiter* w = reinterpret_cast<data_awaiter*>(wd);

                if(w->exam_stop())
                    return -1;

                size_t n = size * nmemb;

                if(n == 0)
                    return n;

                size_t prevUsed = w->_cr.template release_data_used_on_pause<PauseType>();

                if(prevUsed > n) // something wrong
                    return 0;

                if constexpr(ReadSomeBits & Bw)
                {
                    size_t m = n - prevUsed;

                    size_t copied = w->get_bw<Bw>().append(d + prevUsed, m);

                    BOOST_ASSERT(copied <= m);

                    if(copied < m)
                    {
                        w->_cr.template try_mark_pause_and_schedule_resume<PauseType>(copied);
                        return CURL_WRITEFUNC_PAUSE;
                    }
                }
                else
                {
                    if(! w->get_bw<Bw>().append(d + prevUsed, n - prevUsed))
                    {
                        w->_ec = asio::error::no_buffer_space;
                        return 0;
                    }
                }

                return n;
            }

            template<pause_type PauseType> requires(PauseType != pause_type_none)
            static size_t pause_when_called(char* /*d*/, size_t size, size_t nmemb, void* wd)
            {
                data_awaiter* w = reinterpret_cast<data_awaiter*>(wd);

                if(w->exam_stop())
                    return -1;

                size_t n = size * nmemb;

                if(n == 0)
                    return n;

                w->_cr.template try_mark_pause_and_schedule_resume<PauseType>(0);
                return CURL_WRITEFUNC_PAUSE;
            }

            bool init()
            {
                _bw.clear();

                if constexpr(RT == read_type_hd_wd)
                {
                    if constexpr(decltype(_bw2)::is_buf())
                    {
                        _bw2.clear();
                        _cr._easy.setopts(
                            curlopt::header_cb(resumed_write_to_bw<1, pause_type_hd>, this),
                            curlopt::write_cb (resumed_write_to_bw<2, pause_type_wd>, this)
                        );
                    }
                    else
                    {
                        _cr._easy.setopts(
                            curlopt::header_cb(resumed_write_to_bw<1, pause_type_hd>, this),
                            curlopt::write_cb (resumed_write_to_bw<1, pause_type_wd>, this)
                        );
                        // NOTE: don't use curlopt::header_to_write_cb. It would break pause/resume with other read_type.
                    }
                }
                else if constexpr(RT == read_type_hd)
                {
                    _cr.clear_pause_if_not<pause_type_hd>();
                    _cr._easy.setopts(
                        curlopt::header_cb(resumed_write_to_bw<1, pause_type_hd>, this),
                        curlopt::disbale_write_cb
                    );
                }
                else if constexpr(RT == read_type_wd)
                {
                    _cr.clear_pause_if_not<pause_type_wd>();
                    _cr._easy.setopts(
                        curlopt::disbale_header_cb,
                        curlopt::write_cb(resumed_write_to_bw<1, pause_type_wd>, this)
                    );
                }
                else if constexpr(RT == read_type_hd_until_wd)
                {
                    _cr._easy.setopts(
                        curlopt::header_cb(
                            [](char* d, size_t size, size_t nmemb, void* wd) -> size_t
                            {
                                // to skip paused wd before first hd, we set the write_cb when header_cb is called.
                                reinterpret_cast<data_awaiter*>(wd)->_cr.opts(curlopt::write_cb(pause_when_called<pause_type_hd>, wd));
                                return resumed_write_to_bw<1, pause_type_hd>(d, size, nmemb, wd);
                            },
                            this),
                        curlopt::disbale_write_cb
                    );
                }
                else if constexpr(RT == read_type_wd_until_hd)
                {
                    _cr._easy.setopts(
                        curlopt::disbale_header_cb,
                        curlopt::write_cb(
                            [](char* d, size_t size, size_t nmemb, void* wd) -> size_t
                            {
                                // to skip paused hd before first wd, we set the header_cb when write_cb is called.
                                reinterpret_cast<data_awaiter*>(wd)->_cr.opts(curlopt::header_cb(pause_when_called<pause_type_wd>, wd));
                                return resumed_write_to_bw<1, pause_type_wd>(d, size, nmemb, wd);
                            },
                            this)
                    );
                }

                return true;
            }

            auto get_result()
            {
                if constexpr(decltype(_bw2)::is_buf())
                {
                    return aresult<std::pair<size_t, size_t>>(_ec, _bw.size(), _bw2.size());
                }
                else
                {
                    return aresult<size_t>(_ec, _bw.size());
                }
            }

        public:
            data_awaiter(curl_request& cr, B&& b, B2&& b2)
                : data_awaiter_base{cr}, _bw{JKL_FORWARD(b)}, _bw2{JKL_FORWARD(b2)}
            {}

            bool await_ready() const noexcept { return false; }

            template<class P>
            bool await_suspend(std::coroutine_handle<P> c)
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

                BOOST_ASSERT(is_removed_state(st) || is_paused_state(st));
                //if(! (is_removed_state(st) || is_paused_state(st)))
                //{
                //    _ec = aerrc::operation_not_permitted;
                //    return false;
                //}

                //std::lock_guard lg{_cr._cl._mtx};

                if(! init())
                    return false;

                _coro = c;
                BOOST_ASSERT(! _cr._aw);
                _cr._aw = this; // don't put this in ctor, awaiter may be moved

                // start or unpause the request
                {
                    std::lock_guard lg{_cr._cl._mtx};

                    _cr.set_state(state_running);

                    if(is_removed_state(st))
                        _ec = _cr._cl._multi.add(_cr._easy).error(); // will set a time-out to trigger very soon
                    else // if(is_paused_state(st))
                        _ec = _cr._easy.pause(CURLPAUSE_CONT).error(); // unpause all, NOTE: header_cb/write_cb will be called in this if there are remain data
                }                    // ^^^^^: will also use multi, so lock it.

                if(_ec)
                {
                    _cr.set_state(st);
                    _cr.async_resume_coro();
                    return false;
                }

                if constexpr(EnableStop)
                {
                    BOOST_ASSERT(! _stopCb);
                    BOOST_ASSERT(! _stopRequested);

                    _stopCb.emplace(c.promise().get_stop_token(),
                        [this]()
                        {
                            std::atomic_ref{_stopRequested}.store(true, std::memory_order_relaxed);
                        }
                    );
                }

                return true;
            }

            auto await_resume()
            {
                _cr._aw = nullptr; // don't put this in dtor, awaiter may be moved
                //BOOST_ASSERT(is_removed_state(_cr.get_state()));
                return get_result();
            }
        };

        template<read_type RT>
        auto do_read_data2(_byte_buf_ auto&& b1, auto&& b2, auto... p)
        {
            auto params = make_params(p..., p_read_some_none, p_disable_stop);

            return data_awaiter<
                RT, decltype(b1), decltype(b2), params(t_read_some), params(t_stop_enabled)
            >
            (*this, JKL_FORWARD(b1), JKL_FORWARD(b2));
        }

        template<read_type RT>
        auto do_read_data(_byte_buf_ auto&& b, auto... p)
        {
            return do_read_data2<RT>(JKL_FORWARD(b), null_op, p...);
        }

        // this only insert name value into fields, while beast.http.parser will also set other members in header
        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        static aresult<> parse_fields(_byte_buf_ auto const& in, auto& fields)
        {
            aerror_code ec;
            boost::string_view name;
            boost::string_view value;
            beast::detail::char_buffer<beast::http::detail::basic_parser_base::max_obs_fold> buf;

            for(auto p = buf_begin(in), last = buf_end(in);;)
            {
                if(p + 2 > last)
                    return beast::http::error::need_more;

                if(p[0] == '\r')
                {
                    if(p[1] == '\n')
                        return no_err;
                    return beast::http::error::bad_line_ending;
                }

                beast::http::detail::basic_parser_base::parse_field(p, last, name, value, buf, ec);
                if(ec)
                    return ec;

                fields.insert(name, value);
            }
            return no_err;
        }

        // assumes b contains one or more headers and an optional trailer at the end:
        //      HTTP/...\r\n
        //      [header fields] \r\n
        //      \r\n
        //      ...
        //      [trailer header fields] \r\n
        //      \r\n
        template<bool IsRequest, class Fields>
        static aresult<> parse_header_trailer(_byte_buf_ auto const& b,  beast::http::header<IsRequest, Fields>& h)
        {
            string_view bv{buf_data(b), buf_size(b)};

            if(bv.size() < 16)
                return beast::http::error::need_more;

            string_view header = bv;
            string_view trailer;

            if(auto q = bv.rfind("\r\n\r\n", bv.size() - 5); q != npos)
            {
                if(string_view t = bv.substr(q + 4); t.starts_with("HTTP/"))
                {
                    header = t;
                }
                else
                {
                    trailer = t;

                    if(auto j = bv.rfind("\r\n\r\n", q - 1); j != npos)
                        header = bv.substr(j + 4, q - j);
                    else
                        header = bv.substr(0, q + 4);
                }
            }

            beast::http::response_parser<beast::http::empty_body, typename Fields::allocator_type> parser;

            parser.skip(true); // complete when the header is done

            aerror_code ec;

            parser.put(asio_buf(header), ec);

            if(ec)
                return ec;

            BOOST_ASSERT(parser.is_header_done());

            if(trailer.size())
            {
                JKL_TRY(parse_fields(trailer, parser.get()));
            }

            h = parser.release();
            return ec;
        }

        template<class Body, class Fields>
        static aresult<> fill_response_body(_byte_buf_ auto const& b, beast::http::response<Body, Fields>& m)
        {
            typename Body::reader r(m.base(), m.body());

            aerror_code ec;

            r.init(ec, b.size());
            if(ec)
                return ec;

            r.put(asio_buf(b), ec);
            if(ec)
                return ec;

            r.finish(ec);
            return ec;
        }

    public:
        explicit curl_request(curl_client& cl)
            : _cl{cl}
        {
            // reset_all(); should called when creating and recycling in res_pool
        }

    #ifndef NDEBUG
        ~curl_request()
        {
            BOOST_ASSERT(! _state || ! is_runnning_state(get_state()));
        }

        // since we write out dtor, move control won't be implicitly-declared
        curl_request(curl_request&&) = default;
        //curl_request& operator=(curl_request&&) = default;
    #endif

        // still need to check returned error from co_awaited result
        bool finished() noexcept { return is_finished_state(get_state()); }

        // curlinfo::
        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto info(auto const& t) { return _easy.info(t); }

        auto last_response_code() { return _easy.info(curlinfo::response_code); }

        // curlopt::
        // NOTE: some opts are preserved by this class
        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto& opts(auto&&... t)
        {
            _easy.setopts(JKL_FORWARD(t)...);
            return *this;
        }

        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto& proxy(_str_ auto const& u)
        {
            return opts(curlopt::proxy(u));
        }

        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto& no_proxy_list(auto const& us) // should be a range of str
        {
            string l;
            for(auto const& u : us)
                append_str(l, u, ',');
            return opts(curlopt::noproxy(l));
        }

        // hd: data from CURLOPT_HEADERFUNCTION
        // wd: data from CURLOPT_WRITEFUNCTION
        //
        // if buf is lvalue-ref, the ref will be used, otherwise a copy will be used.
        // So, both container and span/view like buf can be used.
        //
        // if p_read_some_xxx is used, it will fill the corresponding buf for at most buf_size(buf),
        //      and transfer completes when no error is returned and (read size < buf_size(b) || finished())
        // otherwise:
        // if buf is resizable, it will be filled with all the data left and the buf will be resized to the exact size.
        // if buf is not resizable, and the left data cannot fit in it, asio::error::no_buffer_space will be returned.
        //
        // NOTE: the data returned is already decoded by curl.
        //       decoding will be applied to original content received with Content-Encoding/Transfer-Encoding header fields.

        // co_await result is aresult<size_t>, for actual read size
        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto read_hd         (_byte_buf_ auto&& b, auto... p) { return do_read_data<read_type_hd         >(JKL_FORWARD(b), p...); }
        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto read_wd         (_byte_buf_ auto&& b, auto... p) { return do_read_data<read_type_wd         >(JKL_FORWARD(b), p...); }
        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto read_hd_until_wd(_byte_buf_ auto&& b, auto... p) { return do_read_data<read_type_hd_until_wd>(JKL_FORWARD(b), p...); }
        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto read_wd_until_hd(_byte_buf_ auto&& b, auto... p) { return do_read_data<read_type_wd_until_hd>(JKL_FORWARD(b), p...); }
        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto read_hd_wd      (_byte_buf_ auto&& b, auto... p) { return do_read_data<read_type_hd_wd      >(JKL_FORWARD(b), p...); }

        // co_await result is aresult<std::pair<size_t, size_t>>, for actual read size
        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto read_hd_wd(_byte_buf_ auto&& hd, _byte_buf_ auto&& wd, auto... p)
        {
            return do_read_data2<read_type_hd_wd>(JKL_FORWARD(hd), JKL_FORWARD(wd), p...);
        }

        /// http specific method

        // an error code of CURLE_HTTP_RETURNED_ERROR will returned when HTTP response code >= 400
        // NOTE: This option is not fail-safe and there are occasions where non-successful response codes
        //       will slip through, especially when authentication is involved (response codes 401 and 407)
        auto& fail_on_http_error(bool on = true)
        {
            _easy.setopt(curlopt::failonerror(on));
            return *this;
        }

        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto& method(_str_ auto const& s) { return opts(curlopt::customrequest(s)); }
        auto& method(beast::http::verb v) { return method(beast::http::to_string(v)); }

        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto& get (auto const& u) { return method("GET" ).target(u); }
        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto& put (auto const& u) { return method("PUT" ).target(u); }
        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto& post(auto const& u) { return method("POST").target(u); }

        // can be string or curlu
        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto& target(auto const& u) { return opts(curlopt::url(u)); }

        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto& field_line(auto&&... lines) // string or curlist
        {
            _fields.append(JKL_FORWARD(lines)...);
            return *this;
        }

        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto& field(auto const&... kvs)
        {
            _fields.append_field(kvs...);
            return *this;
        }

        curl_request& reset_fields(curl_fields&& h = curl_fields())
        {
            _fields = std::move(h);

            // you can disable curl to add some headers by adding field lines like "<name>:"
            // e.g.: "Host:", "Accept:", "User-Agent:"

            _fields.append("Expect:"); // disable "Expect: 100-continue" set by curl, see NOTE below
            _fields.append("Accept:");
            _fields.append("User-Agent:");

            _easy.setopt(curlopt::httpheader(_fields));

            return *this;
        }

        // NOTE: if body is used, "Content-Type: application/x-www-form-urlencoded",
        //       and "Expect: 100-continue"(when body size > 1kb for HTTP 1.1)
        //       will be set by curl,
        //       "Content-Type" can only be overridden with other value
        //       "Expect" can be removed with a header line "Expect:", which has been done in reset_headers();

        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto& body_ref(_byte_buf_ auto      & b) { return opts(curlopt::postfields(b)); }
        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto& body    (_byte_buf_ auto const& b) { return opts(curlopt::copypostfields(b)); }

        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto& str_body_ref(_str_ auto      & s) { return body_ref(as_str_class(s)); }
        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto& str_body    (_str_ auto const& s) { return body(as_str_class(s)); }


        // suppose: all intermediate responses have no body, and only read last response.
        //          all wd belong to last response.
        //          a trailer may follow. After the trailer, the transfer should complete.

        // you can keep on read body, if any.
        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        aresult_task<> read_header(auto& h, auto... p)
        {
            string hd;
            JKL_CO_TRY(co_await read_hd_until_wd(hd, p...));
            co_return parse_header_trailer(hd, h);
        }

        // b can be used with p_read_some
        // you can directly read_body(), without read_header()
        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto read_body(_byte_buf_ auto&& b, auto... p)
        {
            return read_wd(JKL_FORWARD(b), p...);
        }

        // if there is trailer after read_body().
        template<class Msg>
        aresult_task<> read_trailer(Msg& m, auto... p)
        {
            string b;
            JKL_CO_TRY(co_await read_hd(b, p...));
            co_return parse_fields(b, m);
        }

        // b can be used with p_read_some
        // trailer is allowed(hd after wd).
        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        aresult_task<> read_response(auto& h, _byte_buf_ auto&& b, auto... p)
        {
            string hd;
            JKL_CO_TRY(co_await read_hd_wd(hd, JKL_FORWARD(b), p...));
            co_return parse_header_trailer(hd, h);
        }

        _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
        auto read_response(auto& msg, auto... p)
        {
            return read_response(msg.base(), msg.body(), p...);
        }

        ///

        template<class Header = http_response_header>
        aresult_task<Header> return_header(auto... p)
        {
            Header h;
            JKL_CO_TRY(co_await read_header(h, p...));
            co_return h;
        }

        template<_resizable_byte_buf_ B = string>
        aresult_task<B> return_body(auto... p)
        {
            B b;
            JKL_CO_TRY(co_await read_body(b, p...));
            co_return b;
        }

        template<class Fields = http_fields>
        aresult_task<Fields> return_trailer(auto... p)
        {
            Fields f;
            JKL_CO_TRY(co_await read_trailer(f, p...));
            co_return f;
        }

        template<class Msg = http_response>
        aresult_task<Msg> return_response(auto... p)
        {
            Msg m;
            JKL_CO_TRY(co_await read_response(m, p...));
            co_return m;
        }
    };

private:
    // NOTE: all curl callbacks, anything involving _multi, state_running _easy should inside critical section
    std::mutex         _mtx;
    asio::io_context&  _ioc;
    asio::steady_timer _timer{_ioc};

    curl_multi             _multi;
    res_pool<curl_request> _pool;
    unordered_auto_map<curl_socket_t, socket_data> _sds;
    size_t _idSeq = 0;

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    void ioc_post(auto&& f) { asio::post(_ioc   , JKL_FORWARD(f)); }

    static curl_request& get_curl_request(CURL* e)
    {
        BOOST_ASSERT(e);
        curl_easy ce{e};
        curl_request* cr = ce.priv_data<curl_request*>();
        ce.release();
        BOOST_ASSERT(cr);
        return *cr;
    }

    int process_socket_action(curl_socket_t cskt, int acts)
    {
        std::lock_guard lg{_mtx};
        return process_socket_action_nolock(cskt, acts);
    }

    int process_socket_action_nolock(curl_socket_t cskt, int acts)
    {
        //                        vvvvvvvvvvvvv invokes other curl callbacks
        int stillRunning = _multi.socket_action(cskt, acts).value_or_throw();

        while(auto msg = _multi.next_done())
        {
            get_curl_request(msg.easy)
                .try_remove_and_schedule_resume(msg.err);
        }

        if(stillRunning <= 0)
            _timer.cancel();

        return stillRunning;
    }

    // single action (read/in or write/out) process chain
    void event_cb(size_t id, aerror_code const& ec, curl_socket_t cskt, int act)
    {
        BOOST_ASSERT(act == CURL_POLL_IN || act == CURL_POLL_OUT);

        if(ec == asio::error::operation_aborted)
            return;

        std::lock_guard lg{_mtx};

        int stillRunning = process_socket_action_nolock(cskt, ec ? CURL_CSELECT_ERR : act); // -> _multi.socket_action() -> multi_sock_cb()

        if(ec || ! stillRunning)
            return;

        // sd may be removed(and possibly the same file descriptor is immediataly reused) indirectly by process_socket_action_nolock()
        
        // check if still the same sd

        auto it = _sds.find(cskt);

        if(it == _sds.end() || it->second.id != id)
            return;

        socket_data& sd = it->second;

        if(sd.acts & act)
        {
            if(act == CURL_POLL_IN)
                sd.skt.async_wait(asio::socket_base::wait_read, [&, id = sd.id, cskt](auto&& ec){
                    event_cb(id, ec, cskt, CURL_POLL_IN);
                });
            else // if(act == CURL_POLL_OUT)
                sd.skt.async_wait(asio::socket_base::wait_write, [&, id = sd.id, cskt](auto&& ec){
                    event_cb(id, ec, cskt, CURL_POLL_OUT);
                });
        }
    }

    // should already inside critical section when get called
    static int multi_sock_cb(CURL* e, curl_socket_t cskt, int acts, void* cbp, void* /*sockp*/)
    {
        BOOST_ASSERT(cbp);
        curl_client&  cl = *reinterpret_cast<curl_client*>(cbp);
        curl_request& cr = get_curl_request(e);

        switch(acts)
        {
            case CURL_POLL_IN:
            case CURL_POLL_OUT:
            case CURL_POLL_INOUT:
                {
                    auto it = cl._sds.find(cskt);

                    if(it == cl._sds.end())
                    {
                        socket_data sd{cl._ioc};

                        if(aerror_code ec = sd.assign(cskt))
                        {
                            cr.try_remove_and_schedule_resume(ec);
                            return CURLM_OK;
                        }

                        sd.id = ++ cl._idSeq;
                        it = cl._sds.try_emplace(cskt, std::move(sd)).first;
                    }

                    socket_data& sd = it->second;

                    if((acts & CURL_POLL_IN) && !(sd.acts & CURL_POLL_IN))
                        sd.skt.async_wait(asio::socket_base::wait_read, [&, id = sd.id, cskt](auto&& ec){
                            cl.event_cb(id, ec, cskt, CURL_POLL_IN);
                        });

                    if((acts & CURL_POLL_OUT) && !(sd.acts & CURL_POLL_OUT))
                        sd.skt.async_wait(asio::socket_base::wait_write, [&, id = sd.id, cskt](auto&& ec){
                            cl.event_cb(id, ec, cskt, CURL_POLL_OUT);
                        });

                    sd.acts = acts;
                }
                break;
            case CURL_POLL_REMOVE:
                BOOST_ASSERT(cl._sds.contains(cskt));
                cl._sds.erase(cskt);
                break;
            default:
                BOOST_ASSERT(false);
                throw std::runtime_error("multi_sock_cb: unknown action");
        }

        return CURLM_OK;
    }

    // should already inside critical section when get called
    static int multi_timer_cb(CURLM*, long ms, void* cbp)
    {
        BOOST_ASSERT(cbp);
        curl_client& cl = *reinterpret_cast<curl_client*>(cbp);

        cl._timer.cancel();

        if(ms > 0)
        {
            cl._timer.expires_after(std::chrono::milliseconds(ms));
            cl._timer.async_wait([&](auto&& ec){
                if(! ec)
                    cl.process_socket_action(CURL_SOCKET_TIMEOUT, 0);
            });
        }
        else if(ms == 0)
        {
            cl.process_socket_action_nolock(CURL_SOCKET_TIMEOUT, 0);
        }

        return CURLM_OK;
    }

    std::function<void(curl_request&)> _defaultOptsFunc;
    string _defaultProxy;
    std::deque<string> _noProxyList;

    // called in curl_request.reset_all()
    void apply_default_opts(curl_request& cr)
    {
        cr.proxy(_defaultProxy);
        cr.no_proxy_list(_noProxyList);

        if(_defaultOptsFunc)
            _defaultOptsFunc(cr);
    }

public:
    explicit curl_client(size_t cap, asio::io_context& ctx = default_ioc())
        : _ioc{ctx}, _pool{cap, [this](auto&& emplace){ emplace(*this).reset_all(); }}
    {
        _pool.on_recycle([](curl_request& cr){
            cr.on_recycle();
        });

        _multi.setopts(
            curlmopt::socket_cb(multi_sock_cb, this),
            curlmopt::timer_cb(multi_timer_cb, this)
        );
    }

    curl_client(curl_client const&) = delete;
    curl_client& operator=(curl_client const&) = delete;
    curl_client(curl_client&&) = delete;
    curl_client& operator=(curl_client&&) = delete;

    auto& io_ctx() noexcept { return _ioc; }

    // do not call clear_unused() when running
    auto& pool() noexcept { return _pool; }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    auto acquire_request(auto... p)
    {
        return _pool.acquire(p...);
    }

    /// settings
    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    auto& opts(auto&&... t)
    {
        std::lock_guard lg{_mtx};
        _multi.setopts(JKL_FORWARD(t)...);
        return *this;
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    void default_opts_func(auto&& f)
    {
        std::lock_guard lg{_mtx};
        _defaultOptsFunc = JKL_FORWARD(f);
    }

    void clear_default_opts_func()
    {
        std::lock_guard lg{_mtx};
        _defaultOptsFunc = nullptr;
    }

    auto& default_proxy()
    {
        std::lock_guard lg{_mtx};
        return _defaultProxy;
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    void default_proxy(_str_ auto const& u)
    {
        std::lock_guard lg{_mtx};
        _defaultProxy = u;
    }

    void clear_default_proxy()
    {
        std::lock_guard lg{_mtx};
        _defaultProxy.clear();
    }

    auto& no_proxy_list()
    {
        std::lock_guard lg{_mtx};
        return _noProxyList;
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    void add_no_proxy(_str_ auto&&... u)
    {
        std::lock_guard lg{_mtx};
        (... , _noProxyList.emplace_back(JKL_FORWARD(u)));
    }

    void clear_no_proxy_list()
    {
        std::lock_guard lg{_mtx};
        _noProxyList.clear();
    }

    ///
    template<_resizable_byte_buf_ B>
    aresult_task<> read_body(auto method, auto target, curl_fields fields, B&& b, auto... p)
    {
        lref_or_val_t<B> bh = std::forward<B>(b);

        JKL_CO_TRY(auto&& req, co_await acquire_request(p...));

        co_return co_await req->fail_on_http_error() // otherwise user may only get an invalid body with no ec
                            .method(method).target(target).reset_fields(std::move(fields))
                            .read_body(bh, p...);
    }

    template<_resizable_byte_buf_ B = string>
    aresult_task<B> return_body(auto method, auto target, curl_fields fields, auto... p)
    {
        B b;
        JKL_CO_TRY(co_await read_body(method, target, fields, b, p...));
        co_return b;
    }

    template<_resizable_byte_buf_ B = string>
    auto get_body(auto&& target, curl_fields fields, auto... p)
    {
        return return_body("GET", JKL_FORWARD(target), std::move(fields), p...);
    }

    template<_resizable_byte_buf_ B = string>
    auto get_body(auto&& target, auto... p)
    {
        return return_body("GET", JKL_FORWARD(target), {}, p...);
    }

    ///
    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    aresult_task<> read_response(auto method, auto target, curl_fields fields, auto& msg, auto... p)
    {
        JKL_CO_TRY(auto&& req, co_await acquire_request(p...));

        co_return co_await req->method(method).target(target).reset_fields(std::move(fields))
                                .read_response(msg, p...);
    }

    template<_resizable_byte_buf_ B>
    aresult_task<> read_response(auto method, auto target, curl_fields fields, auto& msg, B&& b, auto... p)
    {
        lref_or_val_t<B> bh = std::forward<B>(b);

        JKL_CO_TRY(auto&& req, co_await acquire_request(p...));

        co_return co_await req->method(method).target(target).reset_fields(std::move(fields))
                                .read_response(msg, bh, p...);
    }

    template<class Msg = http_response>
    aresult_task<Msg> return_response(auto method, auto target, curl_fields fields, auto... p)
    {
        Msg m;
        JKL_CO_TRY(co_await read_response(method, target, std::move(fields), m, p...));
        co_return m;
    }

    template<class Msg = http_response>
    auto get_response(auto&& target, curl_fields fields, auto... p)
    {
        return return_response("GET", JKL_FORWARD(target), std::move(fields), p...);
    }

    template<class Msg = http_response>
    auto get_response(auto&& target, auto... p)
    {
        return return_response("GET", JKL_FORWARD(target), {}, p...);
    }
};


} // namespace jkl
