#if 0
//#define JKL_DISABLE_CORO_RECYCLING
#include <jkl/gen.hpp>
#include <jkl/util/log.hpp>

using namespace jkl;


int main()
{
    int r[] = {1,2,6};

    while_next([&r]()->agen<int>{
                   for(int e : r)
                       co_yield e;
                   //co_return;
               }(),
               [](int v)->atask<>{
                  std::cout << v;
                  //co_return;
    }).start_join();
}

#elif 0

// #define BOOST_ASIO_ENABLE_HANDLER_TRACKING
#include <jkl/client.hpp>
#include <jkl/gen.hpp>
#include <jkl/uri.hpp>
#include <jkl/html.hpp>
#include <jkl/mime.hpp>
#include <jkl/file.hpp>
#include <jkl/res_pool.hpp>
#include <jkl/charset/icu.hpp>
#include <jkl/uri/data_url.hpp>
#include <jkl/util/log.hpp>
#include <boost/beast.hpp>
// #include <boost/filesystem.hpp>
#include <filesystem>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/name_generator_sha1.hpp>


using namespace jkl;


size_t g_thread_cnt       = 6;
int    g_max_depth        = 6;
size_t g_max_req          = 126;
size_t g_per_host_max_req = 6;
string g_save_dir         = "downloads";

curl_client                g_client{g_max_req};
res_pool_map<string, void> g_perHostReqPermit{g_per_host_max_req};

std::mutex                 g_visited_urls_mtx;
unordered_node_set<string> g_visited_urls; // should be decoded url without fragment

std::atomic_uint g_crawl_id = 0;


// return if unvisited/added
bool add_unvisited_url(string url) // should not contain fragment part
{
    if(auto r = uri_normalize_ret<url_decoded>(uri_uri<url_encoded>(url)))
        url = std::move(r.value());

    if(url.starts_with("http://"))
        url.erase(0, 7);
    else if(url.starts_with("https://"))
        url.erase(0, 8);

    std::lock_guard lg{g_visited_urls_mtx};
    return g_visited_urls.emplace(std::move(url)).second;
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
auto gen_unique_name(_char_str_ auto const& url)
{
    auto id = boost::uuids::name_generator_sha1{boost::uuids::ns::url()}(str_data(url), str_size(url));
    return boost::uuids::to_string(id);
}


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
aresult_task<> do_crawl(string url, int depth, auto crawId);

// this function is just for crawId to track whole execution of do_crawl(), one can use do_crawl() directly
atask<> start_crawl(string url, int depth)
{
    if(depth < 0)
        co_return;

    auto crawId = g_crawl_id.fetch_add(1, std::memory_order_relaxed);
    JKL_LOG << crawId << ". >>>>>>>>>>>>>>>>>>>>><crawl begin>";

    try
    {
        auto r = co_await do_crawl(std::move(url), depth, crawId);
        if(! r)
            JKL_ERR << crawId << ". "  << r.error().message();
    }
    catch(std::exception& e)
    {
        JKL_ERR << crawId << ". " << e.what();
    }
    catch(...)
    {
        JKL_ERR << crawId << ". unknown exception";
    }

    JKL_LOG << crawId << ". <<<<<<<<<<<<<<<<<<<<<<crawl end>";
}


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
aresult_task<> do_crawl(string url, int depth, auto crawId)
{
    if(depth < 0)
        co_return no_err;

    JKL_LOG << crawId << ". url: " << url.subview(0, 66) << (url.size() > 66 ? "..." : ":");
    JKL_LOG << crawId << ". depth: " << depth;

    // data url
    if(url.starts_with("data:"))
    {
        if(auto r = parse_data_url(url))
        {
            auto& ud = r.value();

            if(ud.mime.starts_with("image/") || ud.mime.starts_with("video/") || ud.mime.starts_with("audio/"))
            {
                auto name = gen_unique_name(url);
                auto ext  = preferred_ext_for_mime(ud.mime, "unknown_type");
                auto path = cat_str(g_save_dir, '/', name, '.', ext);

                if(std::filesystem::exists(path.data()))
                    co_return gerrc::file_exists;

                synced_file file;
                JKL_CO_TRY(file.open(path, file_mode::append));
                JKL_CO_TRY(file.write(ud.data));
            }
        }
        else
        {
            JKL_LOG << crawId << ". " << r.error().message();
        }

        co_return no_err;
    }

    // get http header
    string_view schemeAuth;
    JKL_CO_TRY(read_uri(uri_uri<url_encoded>(url), uri_scheme_authority<as_is_codec>(schemeAuth)));
    JKL_CO_TRY(auto permit, co_await g_perHostReqPermit.acquire(schemeAuth, p_enable_stop));

    JKL_CO_TRY(auto req, co_await g_client.acquire_request(p_enable_stop));

    JKL_CO_TRY(auto header, co_await req->get(url).return_header(p_enable_stop));

    http_field_content_type ct = parse_http_content_type(header);

    // multimedia
    if(ct.mime.starts_with("image/") || ct.mime.starts_with("video/") || ct.mime.starts_with("audio/"))
    {
        auto name = gen_unique_name(url);
        string ext  = preferred_ext_for_mime(ct.mime);

        if(ext.empty())
        {
            if(! read_uri(uri_uri<url_encoded>(url), uri_file_ext<url_decoded>(ext)))
                ext = "unknown_type";
        }

        auto path = cat_str(g_save_dir, '/', name, '.', ext);

        JKL_LOG << crawId << ". write to: " << path;

        if(std::filesystem::exists(path.data()))
            co_return gerrc::file_exists;

        synced_file file;
        JKL_CO_TRY(file.open(path, file_mode::append));

        char buf[64 * 1024];

        for(;;)
        {
            JKL_CO_TRY(auto size, co_await req->read_body(buf, p_read_some, p_enable_stop));

            JKL_CO_TRY(file.write(buf, size));

            if(size < sizeof(buf) || req->finished())
                break;
        }

        co_return no_err;
    }

    // html
    if(ct.mime == "text/html")
    {
        if(depth <= 1)
            co_return no_err;

        JKL_CO_TRY(auto body, co_await req->return_body(url, p_enable_stop));
        req.recycle();
        permit.recycle();

        JKL_CO_TRY(auto o, jkl::parse_html<icu_charset_cvt>(body, ct.charset));

        std::deque<string> urls;

        for(gumbo_element const& e :  o.root() % (html_tag_sel{GUMBO_TAG_IMG} || GUMBO_TAG_VIDEO || GUMBO_TAG_AUDIO || GUMBO_TAG_A))
        {
            auto ref = ascii_trimed_view(e.attrv(e.tag() == GUMBO_TAG_A ? "href" : "src"));

            if(ref.size())
            {
                if(ref.starts_with("data:"))
                {
                    urls.emplace_back(std::move(ref));
                }
                else if(! ref.starts_with("javascript:"))
                {
                    if(auto r = uri_resolve_ret<as_is_codec>(uri_uri<url_encoded>(url), uri_uri<url_encoded>(ref), p_skip_frag))
                    {
                        if(add_unvisited_url(r.value()))
                            urls.emplace_back(std::move(r.value()));
                    }
                    else
                    {
                        JKL_LOG << crawId << ". uri_resolve failed: " << r.error().message() << " : <" << url << "> + <" << ref << ">";
                    }
                }
            }
        }

        if(urls.size())
        {
            co_await while_next(
                gen_moved_range_elems(urls),
                [depth = depth - 1](auto&& u) -> atask<>
                {
                    return start_crawl(JKL_FORWARD(u), depth);
                }
            );
        }

        co_return no_err;
    }

    co_return no_err;
}


int main()
try
{
    //g_client.default_proxy("http://192.168.0.6:1080");
    //g_client.default_proxy("http://192.168.159.130:1080");

    g_client.opts(
        curlmopt::max_connections(g_max_req),
        curlmopt::max_per_host_connections(g_per_host_max_req),
        curlmopt::max_connection_cache(g_max_req)
    );

    std::error_code ec;
    if(! std::filesystem::create_directory(g_save_dir.data(), ec) && ec)
    {
        JKL_ERR << "Failed to create dir: " << ec.message();
        return EXIT_FAILURE;
    }

    jkl::default_ioc_src().start(g_thread_cnt);

    try{
        //auto task = start_crawl("https://www.google.com/search?q=jav+ecchi&tbm=isch", g_max_depth);
        auto task = start_crawl("https://image.baidu.com/search/index?tn=baiduimage&ps=1&ct=201326592&lm=-1&cl=2&nc=1&ie=utf-8&word=sexy", g_max_depth);
        jkl::signal_set_request_stop_guard sg{task};
        task.start_join();
    }
    catch(std::exception& e)
    {
        JKL_ERR << "root catch: " << e.what();
    }
    catch(...)
    {
        JKL_ERR << "root catch: " << "unknown exception";
    }

    // optional
    jkl::default_ioc_src().join();

    JKL_ERR << "===================<all done>===================";
    return EXIT_SUCCESS;
}
catch(std::exception& e)
{
    JKL_ERR << "main catch: " << e.what();
    return EXIT_FAILURE;
}
catch(...)
{
    JKL_ERR << "main catch: " << "unknown exception";
    return EXIT_FAILURE;
}


#else


#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define ANKERL_NANOBENCH_IMPLEMENT

// #include "uri.hpp"
// #include "http_msg.hpp"
#include "pb.hpp"


#endif
