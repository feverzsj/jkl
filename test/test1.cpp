#if 0

// #define BOOST_ASIO_ENABLE_HANDLER_TRACKING
#include <jkl/client.hpp>
#include <jkl/uri.hpp>
#include <jkl/html.hpp>
#include <jkl/file.hpp>
#include <jkl/res_pool.hpp>
#include <jkl/charset/icu.hpp>
#include <jkl/uri/data_url.hpp>
#include <jkl/util/log.hpp>
#include <boost/beast.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/name_generator_md5.hpp>


using namespace jkl;


size_t g_thread_cnt        = 10;
int    g_max_depth         = 10;
size_t g_max_conn          = 100;
size_t g_per_size_max_conn = 20;
string g_save_dir          = "downloads";

curl_client                g_client{g_max_conn};
res_pool_map<string, void> g_perSiteConnPermit{g_per_size_max_conn};

std:mutex                  g_visited_urls_mtx;
unordered_node_set<string> g_visited_urls; // should be decoded url without fragment

std::atomic_uint g_crawl_id = 0;


// return if unvisited/added
bool add_unvisited_url(string url) // should not contain fragment part
{
    if(auto r = uri_normalize_ret<url_decoded>(uri_uri<url_encoded>(url)))
        url = std::move(r.value());
        
    std::lock_guard lg{g_visited_urls_mtx};
    return g_visited_urls.emplace(JKL_FORWARD(url)).second;
}


aresult_task<> do_crawl(string url, int depth, auto crawId);

// this function is just for crawId to track whole execution of do_crawl(), one can use do_crawl() directly
atask<> start_crawl(string url, int depth)
{
    auto crawId = g_crawl_id.fetch_add(1, std::memory_order_relaxed);

    JKL_LOG << crawId << ". >>>>>>>>>>>>>>>>>>>>><crawl begin>";

    try
    {
        auto r = co_await do_crawl(std::move(url), depth, crawId);
        if(! r)
            JKL_ERR << r.error().message();
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


aresult_task<> do_crawl(string url, int depth, auto crawId)
{
    JKL_LOG << crawId << ". url: " << url.subview(66) << (url.size() > 66 ? "..." : ":");
    JKL_LOG << crawId << ". depth: " << depth;

    if(depth < 0)
        co_return no_err;

    // data url
    if(u.starts_with("data:"))
    {
        if(auto r = parse_data_url(url))
        {
            auto& ud = r.value();

            if(ud.mime.starts_with("image/") || ud.mime.starts_with("video/") || ud.mime.starts_with("audio/"))
            {
                auto id   = boost::uuids::name_generator_md5{boost::uuids::ns::url()}(url);
                auto name = boost::uuids::to_string(id);
                auto ext  = preferred_ext_for_mime(ud.mime, "unknown_type");
                auto path = cat_str(g_save_dir, '/', name, '.', ext);

                if(boost::filesystem::exists(path))
                    co_return aerrc::file_exists;

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
    JKL_CO_TRY(auto permit, co_await g_perSiteConnPermit.acquire(schemeAuth, p_enable_stop));

    JKL_CO_TRY(auto req, co_await g_client.acquire_request(p_enable_stop));

    JKL_CO_TRY(auto header, co_await req->get(url).return_header(p_enable_stop));

    http_field_content_type ct = header.parse_content_type();

    // multimedia
    if(ct.mime.starts_with("image/") || ct.mime.starts_with("video/") || ct.mime.starts_with("audio/"))
    {
        auto id   = boost::uuids::name_generator_md5{boost::uuids::ns::url()}(url);
        auto name = boost::uuids::to_string(id);
        auto ext  = preferred_ext_for_mime(ct.mime);

        if(ext.empty())
        {
            if(! read_uri(uri_uri<url_encoded>(url), uri_file_ext<url_decoded>(ext)))
                ext = "unknown_type";
        }

        auto path = cat_str(g_save_dir, '/', name, '.', ext);

        JKL_LOG << crawId << ". write to: " << path;

        if(boost::filesystem::exists(path))
            co_return aerrc::file_exists;

        synced_file file;
        JKL_CO_TRY(file.open(path, file_mode::append));

        char buf[64 * 1024];

        for(;;)
        {
            JKL_CO_TRY(auto size, co_await req->read_body(buf, p_read_some, p_enable_stop));

            JKL_CO_TRY(file.write(buf, size));

            if(size < sizeof(buf))
                break;
            if(req->finished())
                break;
        }

        co_return no_err;
    }

    // html
    if(ct.mime == "text/html")
    {
        JKL_CO_TRY(auto body, co_await req->return_body(url, p_enable_stop));

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
                else
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
            co_return co_await while_next(
                gen_moved_range_elems(urls),
                [leftDepth = depth - 1](auto&& u)
                {
                    return start_crawl(JKL_FORWARD(u), leftDepth);
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
    g_client.default_proxy("http://192.168.0.6:1080");

    jkl::default_ioc_src().start(g_thread_cnt);

    try{
        auto task = start_crawl("https://www.google.com/search?q=jav+ecchi&tbm=isch", g_max_depth);
        auto g = jkl::default_signal_set_request_stop_guard(task);
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
#include "uri.hpp"
#include "http_msg.hpp"


// #include <jkl/uri.hpp>
// #include <iostream>
//
//
// int main()
// {
//     using namespace jkl;
//
// //     static_assert(tagged_elem_idx<uri_auto_port_tag, std::tuple<tagged_obj<uri_auto_port_tag, int>>>() == 0);
//
//     string_view scheme, schemeColon, user, password, userinfo, host, port, hostPort,
//                 auth, ssAuth, schemeAuth, path, query, qmQuery, frag, sharpFrag, qmQueryFrag;
//     int intPort, autoPort;
//
//     uri_reader rd{uri_scheme(scheme), uri_scheme_colon(schemeColon), uri_user(user), uri_password(password), uri_userinfo(userinfo)
//                 , uri_host(host), uri_port(port), uri_port(intPort), uri_auto_port(autoPort), uri_host_port(hostPort)
//                 , uri_authority(auth), uri_ss_authority(ssAuth), uri_scheme_authority(schemeAuth), uri_path(path)
//                 , uri_query(query), uri_qm_query(qmQuery), uri_frag(frag), uri_sharp_frag(sharpFrag), uri_qm_query_frag(qmQueryFrag)
//     };
//
//     auto&& r = rd.read<as_is_codec, false>("\t\r \n \v\f   https://abc:123@9gag.com/gag/abGW9pv?ref=w.upvotenoti#qeqwe  \t\r  \n\v\f");
//
//     if(! r)
//     {
//         std::cout << r.error().message();
//     }
//     else
//     {
//         std::cout << "scheme     : " << scheme      << std::endl;
//         std::cout << "schemeColon: " << schemeColon << std::endl;
//         std::cout << "user       : " << user        << std::endl;
//         std::cout << "password   : " << password    << std::endl;
//         std::cout << "userinfo   : " << userinfo    << std::endl;
//         std::cout << "host       : " << host        << std::endl;
//         std::cout << "port       : " << port        << std::endl;
//         std::cout << "intPort    : " << intPort     << std::endl;
//         std::cout << "autoPort   : " << autoPort    << std::endl;
//         std::cout << "hostPort   : " << hostPort    << std::endl;
//         std::cout << "auth       : " << auth        << std::endl;
//         std::cout << "ssAuth     : " << ssAuth      << std::endl;
//         std::cout << "schemeAuth : " << schemeAuth  << std::endl;
//         std::cout << "path       : " << path        << std::endl;
//         std::cout << "query      : " << query       << std::endl;
//         std::cout << "qmQuery    : " << qmQuery     << std::endl;
//         std::cout << "frag       : " << frag        << std::endl;
//         std::cout << "sharpFrag  : " << sharpFrag   << std::endl;
//         std::cout << "qmQueryFrag: " << qmQueryFrag << std::endl;
//     }
// }


#endif
