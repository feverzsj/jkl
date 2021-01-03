#pragma once

#include <jkl/util/log.hpp>
#include <jkl/uri.hpp>

#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>


TEST_SUITE("uri"){

using namespace jkl;

TEST_CASE("percent codec"){
    char const* cases[][2] = {
        {"hello, world",
         "hello, world"},
        {"%01%02%03%04%05%06%07%08%09%0a%0B%0C%0D%0e%0f/",
         "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0B\x0C\x0D\x0e\x0f/"},
        {"%10%11%12%13%14%15%16%17%18%19%1a%1B%1C%1D%1e%1f/",
         "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1B\x1C\x1D\x1e\x1f/"},
        {"%20%21%22%23%24%25%26%27%28%29%2a%2B%2C%2D%2e%2f/",
         " !\"#$%&'()*+,-.//"},
        {"%30%31%32%33%34%35%36%37%38%39%3a%3B%3C%3D%3e%3f/",
         "0123456789:;<=>?/"},
        {"%40%41%42%43%44%45%46%47%48%49%4a%4B%4C%4D%4e%4f/",
         "@ABCDEFGHIJKLMNO/"},
        {"%50%51%52%53%54%55%56%57%58%59%5a%5B%5C%5D%5e%5f/",
         "PQRSTUVWXYZ[\\]^_/"},
        {"%60%61%62%63%64%65%66%67%68%69%6a%6B%6C%6D%6e%6f/",
         "`abcdefghijklmno/"},
        {"%70%71%72%73%74%75%76%77%78%79%7a%7B%7C%7D%7e%7f/",
         "pqrstuvwxyz{|}~\x7f/"},
        {"%e4%bd%a0%e5%a5%bd",
         "\xe4\xbd\xa0\xe5\xa5\xbd"},
    };

    constexpr auto unescape = [](auto const& s){ return url_decode_ret<uri_user_tag>(s).value_or_throw(); };
    //constexpr auto   escape = [](auto const& s){ return url_encode_ret<uri_user_tag>(s); };

    for(auto [escaped, unescaped] : cases)
    {
        CHECK(unescape(escaped) == unescaped);
        //CHECK(escape(unescaped) == escaped);
    }
} // TEST_CASE("percent codec")


TEST_CASE("uri writer"){

    // NOTE: IRI unsupported yet

    char const* cases[][2] = {
        //{"http://www.w%33.org", "http://www.w3.org"},
        {"http://r%C3%A4ksm%C3%B6rg%C3%A5s.josefsson.org",
         "http://räksmörgås.josefsson.org"}, // IRI: "http://xn--rksmrgs-5wao1o.josefsson.org"
        {"http://%E7%B4%8D%E8%B1%86.w3.mag.keio.ac.jp",
         "http://納豆.w3.mag.keio.ac.jp"}, // IRI: "http://xn--99zt52a.w3.mag.keio.ac.jp"
        {"http://www.%E3%81%BB%E3%82%93%E3%81%A8%E3%81%86%E3%81%AB%E3%81%AA%E3%81%8C%E3%81%84%E3%82%8F%E3%81%91%E3%81%AE%E3%82%8F%E3%81%8B%E3%82%89%E3%81%AA%E3%81%84%E3%81%A9%E3%82%81%E3%81%84%E3%82%93%E3%82%81%E3%81%84%E3%81%AE%E3%82%89%E3%81%B9%E3%82%8B%E3%81%BE%E3%81%A0%E3%81%AA%E3%81%8C%E3%81%8F%E3%81%97%E3%81%AA%E3%81%84%E3%81%A8%E3%81%9F%E3%82%8A%E3%81%AA%E3%81%84.w3.mag.keio.ac.jp/",
         "http://www.ほんとうにながいわけのわからないどめいんめいのらべるまだながくしないとたりない.w3.mag.keio.ac.jp/"}, // IRI: "http://www.xn--n8jaaaaai5bhf7as8fsfk3jnknefdde3fg11amb5gzdb4wi9bya3kc6lra.w3.mag.keio.ac.jp/"
        {"http://%E3%81%BB%E3%82%93%E3%81%A8%E3%81%86%E3%81%AB%E3%81%AA%E3%81%8C%E3%81%84%E3%82%8F%E3%81%91%E3%81%AE%E3%82%8F%E3%81%8B%E3%82%89%E3%81%AA%E3%81%84%E3%81%A9%E3%82%81%E3%81%84%E3%82%93%E3%82%81%E3%81%84%E3%81%AE%E3%82%89%E3%81%B9%E3%82%8B%E3%81%BE%E3%81%A0%E3%81%AA%E3%81%8C%E3%81%8F%E3%81%97%E3%81%AA%E3%81%84%E3%81%A8%E3%81%9F%E3%82%8A%E3%81%AA%E3%81%84.%E3%81%BB%E3%82%93%E3%81%A8%E3%81%86%E3%81%AB%E3%81%AA%E3%81%8C%E3%81%84%E3%82%8F%E3%81%91%E3%81%AE%E3%82%8F%E3%81%8B%E3%82%89%E3%81%AA%E3%81%84%E3%81%A9%E3%82%81%E3%81%84%E3%82%93%E3%82%81%E3%81%84%E3%81%AE%E3%82%89%E3%81%B9%E3%82%8B%E3%81%BE%E3%81%A0%E3%81%AA%E3%81%8C%E3%81%8F%E3%81%97%E3%81%AA%E3%81%84%E3%81%A8%E3%81%9F%E3%82%8A%E3%81%AA%E3%81%84.%E3%81%BB%E3%82%93%E3%81%A8%E3%81%86%E3%81%AB%E3%81%AA%E3%81%8C%E3%81%84%E3%82%8F%E3%81%91%E3%81%AE%E3%82%8F%E3%81%8B%E3%82%89%E3%81%AA%E3%81%84%E3%81%A9%E3%82%81%E3%81%84%E3%82%93%E3%82%81%E3%81%84%E3%81%AE%E3%82%89%E3%81%B9%E3%82%8B%E3%81%BE%E3%81%A0%E3%81%AA%E3%81%8C%E3%81%8F%E3%81%97%E3%81%AA%E3%81%84%E3%81%A8%E3%81%9F%E3%82%8A%E3%81%AA%E3%81%84.w3.mag.keio.ac.jp/",
         "http://ほんとうにながいわけのわからないどめいんめいのらべるまだながくしないとたりない.ほんとうにながいわけのわからないどめいんめいのらべるまだながくしないとたりない.ほんとうにながいわけのわからないどめいんめいのらべるまだながくしないとたりない.w3.mag.keio.ac.jp/"}, // IRI: "http://xn--n8jaaaaai5bhf7as8fsfk3jnknefdde3fg11amb5gzdb4wi9bya3kc6lra.xn--n8jaaaaai5bhf7as8fsfk3jnknefdde3fg11amb5gzdb4wi9bya3kc6lra.xn--n8jaaaaai5bhf7as8fsfk3jnknefdde3fg11amb5gzdb4wi9bya3kc6lra.w3.mag.keio.ac.jp/"
        {"https://github.com/search?l=&p=3&q=url+encode+test+language:C%2B%2B&ref=advsearch&type=Code",
         "https://github.com/search?l=&p=3&q=url encode test language:C++&ref=advsearch&type=Code"}
    };

    constexpr auto decode = [](auto const& s){ return make_uri<url_decoded>(uri_uri<url_encoded>(s)).value_or_throw(); };
    constexpr auto encode = [](auto const& s){ return make_uri<url_encoded>(uri_uri<url_decoded>(s)).value_or_throw(); };

    for(auto [encoded, decoded] : cases)
    {
        CHECK(decode(encoded) == decoded);
        CHECK(encode(decoded) == encoded);
    }
}

TEST_CASE("verify uri"){
    constexpr auto verify_uri = [](_char_str_ auto const& u, int /*errorOffset*/ = -1) -> bool
    {
        auto scheme = uri_scheme(string{});
        auto user = uri_user(string{});
        auto password = uri_password(string{});
        auto host = uri_host(string{});
        auto port = uri_port(string{});
        auto intPort = uri_port(0);
        auto autoPort = uri_auto_port(0);
        auto query = uri_query(string{});
        auto frag = uri_frag(string{});
        return read_uri(uri_uri<url_encoded>(u), scheme, user, password, host, port, intPort, autoPort, query, frag)
                .no_error();
    };

    CHECK(verify_uri("file:///foo/bar"));
    CHECK(verify_uri("mailto:user@host?subject=blah"));
    CHECK(verify_uri("dav:")); // empty opaque part / rel-path allowed by RFC 2396bis
    CHECK(verify_uri("about:")); // empty opaque part / rel-path allowed by RFC 2396bis

    // the following test cases are from a Perl script by David A. Wheeler
    // at http://www.dwheeler.com/secure-programs/url.pl
    CHECK(verify_uri("http://www.yahoo.com"));
    CHECK(verify_uri("http://www.yahoo.com/"));
    CHECK(verify_uri("http://1.2.3.4/"));
    CHECK(verify_uri("http://www.yahoo.com/stuff"));
    CHECK(verify_uri("http://www.yahoo.com/stuff/"));
    CHECK(verify_uri("http://www.yahoo.com/hello%20world/"));
    CHECK(verify_uri("http://www.yahoo.com?name=obi"));
    CHECK(verify_uri("http://www.yahoo.com?name=obi+wan&status=jedi"));
    CHECK(verify_uri("http://www.yahoo.com?onery"));
    CHECK(verify_uri("http://www.yahoo.com#bottom"));
    CHECK(verify_uri("http://www.yahoo.com/yelp.html#bottom"));
    CHECK(verify_uri("https://www.yahoo.com/"));
    CHECK(verify_uri("ftp://www.yahoo.com/"));
    CHECK(verify_uri("ftp://www.yahoo.com/hello"));
    CHECK(verify_uri("demo.txt"));
    CHECK(verify_uri("demo/hello.txt"));
    CHECK(verify_uri("demo/hello.txt?query=hello#fragment"));
    CHECK(verify_uri("/cgi-bin/query?query=hello#fragment"));
    CHECK(verify_uri("/demo.txt"));
    CHECK(verify_uri("/hello/demo.txt"));
    CHECK(verify_uri("hello/demo.txt"));
    CHECK(verify_uri("/"));
    CHECK(verify_uri(""));
    CHECK(verify_uri("#"));
    CHECK(verify_uri("#here"));

    // Wheeler's script says these are invalid, but they aren't
    CHECK(verify_uri("http://www.yahoo.com?name=%00%01"));
    CHECK(verify_uri("http://www.yaho%6f.com"));
    CHECK(verify_uri("http://www.yahoo.com/hello%00world/"));
    CHECK(verify_uri("http://www.yahoo.com/hello+world/"));
    CHECK(verify_uri("http://www.yahoo.com?name=obi&"));
    CHECK(verify_uri("http://www.yahoo.com?name=obi&type="));
    CHECK(verify_uri("http://www.yahoo.com/yelp.html#"));
    CHECK(verify_uri("//"));

    // the following test cases are from a Haskell program by Graham Klyne
    // at http://www.ninebynine.org/Software/HaskellUtils/Network/URITest.hs
    CHECK(verify_uri("http://example.org/aaa/bbb#ccc"));
    CHECK(verify_uri("mailto:local@domain.org"));
    CHECK(verify_uri("mailto:local@domain.org#frag"));
    CHECK(verify_uri("HTTP://EXAMPLE.ORG/AAA/BBB#CCC"));
    CHECK(verify_uri("//example.org/aaa/bbb#ccc"));
    CHECK(verify_uri("/aaa/bbb#ccc"));
    CHECK(verify_uri("bbb#ccc"));
    CHECK(verify_uri("#ccc"));
    CHECK(verify_uri("#"));
    CHECK(verify_uri("A'C"));

    // escapes
    CHECK(verify_uri("http://example.org/aaa%2fbbb#ccc"));
    CHECK(verify_uri("http://example.org/aaa%2Fbbb#ccc"));
    CHECK(verify_uri("%2F"));
    CHECK(verify_uri("aaa%2Fbbb"));

    // ports
    CHECK(verify_uri("http://example.org:80/aaa/bbb#ccc"));
    CHECK(verify_uri("http://example.org:/aaa/bbb#ccc"));
    CHECK(verify_uri("http://example.org./aaa/bbb#ccc"));
    CHECK(verify_uri("http://example.123./aaa/bbb#ccc"));

    // bare authority
    CHECK(verify_uri("http://example.org"));

    // IPv6 literals (from RFC2732):
    CHECK(verify_uri("http://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:80/index.html"));
    CHECK(verify_uri("http://[1080:0:0:0:8:800:200C:417A]/index.html"));
    CHECK(verify_uri("http://[3ffe:2a00:100:7031::1]"));
    CHECK(verify_uri("http://[1080::8:800:200C:417A]/foo"));
    CHECK(verify_uri("http://[::192.9.5.5]/ipng"));
    CHECK(verify_uri("http://[::FFFF:129.144.52.38]:80/index.html"));
    CHECK(verify_uri("http://[2010:836B:4179::836B:4179]"));
    CHECK(verify_uri("//[2010:836B:4179::836B:4179]"));

    // Random other things that crop up
    CHECK(verify_uri("http://example/Andr&#567;"));
    CHECK(verify_uri("file:///C:/DEV/Haskell/lib/HXmlToolbox-3.01/examples/"));

#if 0 // to support this we need exam charset of each component, and path format
    /// bad uri

    CHECK_FALSE(verify_uri("beepbeep\x07\x07", 8));
    CHECK_FALSE(verify_uri("\n", 0));
    CHECK_FALSE(verify_uri("::", 0)); // not OK, per Roy Fielding on the W3C uri list on 2004-04-01

    // the following test cases are from a Perl script by David A. Wheeler
    // at http://www.dwheeler.com/secure-programs/url.pl
    CHECK_FALSE(verify_uri("http://www yahoo.com", 10));
    CHECK_FALSE(verify_uri("http://www.yahoo.com/hello world/", 26));
    CHECK_FALSE(verify_uri("http://www.yahoo.com/yelp.html#\"", 31));

    // the following test cases are from a Haskell program by Graham Klyne
    // at http://www.ninebynine.org/Software/HaskellUtils/Network/URITest.hs
    CHECK_FALSE(verify_uri("[2010:836B:4179::836B:4179]", 0));
    CHECK_FALSE(verify_uri(" ", 0));
    CHECK_FALSE(verify_uri("%", 1));
    CHECK_FALSE(verify_uri("A%Z", 2));
    CHECK_FALSE(verify_uri("%ZZ", 1));
    CHECK_FALSE(verify_uri("%AZ", 2));
    CHECK_FALSE(verify_uri("A C", 1));
    CHECK_FALSE(verify_uri("A\\'C", 1)); // r"A\'C"
    CHECK_FALSE(verify_uri("A`C", 1));
    CHECK_FALSE(verify_uri("A<C", 1));
    CHECK_FALSE(verify_uri("A>C", 1));
    CHECK_FALSE(verify_uri("A^C", 1));
    CHECK_FALSE(verify_uri("A\\\\C", 1)); // r'A\\C'
    CHECK_FALSE(verify_uri("A{C", 1));
    CHECK_FALSE(verify_uri("A|C", 1));
    CHECK_FALSE(verify_uri("A}C", 1));
    CHECK_FALSE(verify_uri("A[C", 1));
    CHECK_FALSE(verify_uri("A]C", 1));
    CHECK_FALSE(verify_uri("A[**]C", 1));
    CHECK_FALSE(verify_uri("http://[xyz]/", 8));
    CHECK_FALSE(verify_uri("http://]/", 7));
    CHECK_FALSE(verify_uri("http://example.org/[2010:836B:4179::836B:4179]", 19));
    CHECK_FALSE(verify_uri("http://example.org/abc#[2010:836B:4179::836B:4179]", 23));
    CHECK_FALSE(verify_uri("http://example.org/xxx/[qwerty]#a[b]", 23));

    // from a post to the W3C uri list on 2004-02-17
    // breaks at 22 instead of 17 because everything up to that point is a valid userinfo
    CHECK_FALSE(verify_uri("http://w3c.org:80path1/path2", 22));
#endif
} // TEST_CASE("verify uri")


TEST_CASE("scheme host port"){
    struct TestCases {
        char const* url;
        char const* scheme;
        char const* host;
        uint16_t port;
    } cases[] = {
        {"http://192.168.9.1/"                 , "http" , "192.168.9.1"  , 80},
        {"http://[2001:db8::1]/"               , "http" , "[2001:db8::1]", 80},
        {"http://example.com/"                 , "http" , "example.com"  , 80},
        {"http://example.com:123/"             , "http" , "example.com"  , 123},
        {"https://example.com/"                , "https", "example.com"  , 443},
        {"https://example.com:123/"            , "https", "example.com"  , 123},
        {"file:///etc/passwd"                  , "file" , ""             , 0},
        {"file://example.com/etc/passwd"       , "file" , "example.com"  , 0},
        {"http://u:p@example.com/"             , "http" , "example.com"  , 80},
        {"http://u:p@example.com/path"         , "http" , "example.com"  , 80},
        {"http://u:p@example.com/path?123"     , "http" , "example.com"  , 80},
        {"http://u:p@example.com/path?123#hash", "http" , "example.com"  , 80},
    };


    for(auto const& c : cases)
    {
        string scheme, host;
        unsigned short port = 0;

        CHECK(read_uri<url_encoded>(c.url, uri_scheme<url_decoded>(scheme), uri_host<url_decoded>(host), uri_auto_port(port)));
        CHECK(scheme == c.scheme);
        CHECK(host == c.host);
        CHECK(port == c.port);
        CHECK(read_uri<url_decoded>(c.url, uri_scheme<url_encoded>(scheme), uri_host<url_encoded>(host), uri_auto_port(port)));
        CHECK(scheme == c.scheme);
        CHECK(host == c.host);
        CHECK(port == c.port);
        CHECK(read_uri<url_encoded>(c.url, uri_scheme<as_is_codec>(scheme), uri_host<url_encoded>(host), uri_auto_port(port)));
        CHECK(scheme == c.scheme);
        CHECK(host == c.host);
        CHECK(port == c.port);
    }
} // TEST_CASE("scheme host port")


TEST_CASE("resolve"){
    char const* cases[][3] = {
        {"scheme://Authority/"             , "../path"                         , "scheme://Authority/path"},
        {"data:/Blah:Blah/"                , "file.html"                       , "data:/Blah:Blah/file.html"},
        //{"data:/Path/../part/part2"        , "file.html"                       , "data:/Path/../part/file.html"},
        {"data://text/html,payload"        , "//user:pass@host:33////payload22", "data://user:pass@host:33////payload22"},
        {"custom://Authority/"             , "file.html"                       , "custom://Authority/file.html"},
        {"custom://Authority/"             , "other://Auth/"                   , "other://Auth/"},
        {"custom://Authority/"             , "../../file.html"                 , "custom://Authority/file.html"},
        {"custom://Authority/path/"        , "file.html"                       , "custom://Authority/path/file.html"},
        {"custom://Authority:NoCanon/path/", "file.html"                       , "custom://Authority:NoCanon/path/file.html"},
        //{"content://content.Provider/"     , "//other.Provider"                , "content://other.provider/"},
        {"about:blank"                     , "custom://Authority"              , "custom://Authority"},
        {"scheme://Authority/path"         , "#fragment"                       , "scheme://Authority/path#fragment"},
        {"scheme://Authority/"             , "#fragment"                       , "scheme://Authority/#fragment"},
        {"about:blank"                     , "#id42"                           , "about:blank#id42"},
        {"about:blank"                     , " #id42"                          , "about:blank#id42"},
        {"about:blank#oldfrag"             , "#newfrag"                        , "about:blank#newfrag"},
        //{"javascript:alert('foo#bar')"     , "#badfrag"                        , "javascript:alert('foo#badfrag"},
        //{"aaa://a\\"                       , "aaa:."                           , "aaa://a\\."},

        // http://lists.w3.org/Archives/Public/uri/2004Feb/0114.html
        {"foo:a/b"           , "../c"             , "foo:c"},
        {"foo:a"             , "foo:."            , "foo:"},
        {"zz:abc"            , "/foo/../../../bar", "zz:/bar"},
        {"zz:abc"            , "/foo/../bar"      , "zz:/bar"},
        {"zz:abc"            , "foo/../../../bar" , "zz:bar"},
        {"zz:abc"            , "foo/../bar"       , "zz:bar"},
        {"zz:abc"            , "zz:."             , "zz:"},
        {"http://a/b/c/d;p?q", "/."               , "http://a/"},
        {"http://a/b/c/d;p?q", "/.foo"            , "http://a/.foo"},
        {"http://a/b/c/d;p?q", ".foo"             , "http://a/b/c/.foo"},

        // http://gbiv.com/protocols/uri/test/rel_examples1.html
        // examples from RFC 2396
        {"http://a/b/c/d;p?q", "g:h", "g:h"},
        {"http://a/b/c/d;p?q", "g"  , "http://a/b/c/g"},
        {"http://a/b/c/d;p?q", "./g", "http://a/b/c/g"},
        {"http://a/b/c/d;p?q", "g/" , "http://a/b/c/g/"},
        {"http://a/b/c/d;p?q", "/g" , "http://a/g"},
        {"http://a/b/c/d;p?q", "//g", "http://g"},

        // changed with RFC 2396bis
        {"http://a/b/c/d;p?q", "?y" , "http://a/b/c/d;p?y"},
        {"http://a/b/c/d;p?q", "g?y", "http://a/b/c/g?y"},

        // changed with RFC 2396bis
        { "http://a/b/c/d;p?q", "#s"     , "http://a/b/c/d;p?q#s"},
        { "http://a/b/c/d;p?q", "g#s"    , "http://a/b/c/g#s"},
        { "http://a/b/c/d;p?q", "g?y#s"  , "http://a/b/c/g?y#s"},
        { "http://a/b/c/d;p?q", ";x"     , "http://a/b/c/;x"},
        { "http://a/b/c/d;p?q", "g;x"    , "http://a/b/c/g;x"},
        { "http://a/b/c/d;p?q", "g;x?y#s", "http://a/b/c/g;x?y#s"},

        // changed with RFC 2396bis
        {"http://a/b/c/d;p?q", ""             , "http://a/b/c/d;p?q"},
        {"http://a/b/c/d;p?q", "."            , "http://a/b/c/"},
        {"http://a/b/c/d;p?q", "./"           , "http://a/b/c/"},
        {"http://a/b/c/d;p?q", ".."           , "http://a/b/"},
        {"http://a/b/c/d;p?q", "../"          , "http://a/b/"},
        {"http://a/b/c/d;p?q", "../g"         , "http://a/b/g"},
        {"http://a/b/c/d;p?q", "../.."        , "http://a/"},
        {"http://a/b/c/d;p?q", "../../"       , "http://a/"},
        {"http://a/b/c/d;p?q", "../../g"      , "http://a/g"},
        {"http://a/b/c/d;p?q", "../../../g"   , "http://a/g"}, // http://a/../g
        {"http://a/b/c/d;p?q", "../../../../g", "http://a/g"}, // http://a/../../g

        // changed with RFC 2396bis
        {"http://a/b/c/d;p?q", "/./g", "http://a/g"},

        // changed with RFC 2396bis
        {"http://a/b/c/d;p?q", "/../g"     , "http://a/g"},
        {"http://a/b/c/d;p?q", "g."        , "http://a/b/c/g."},
        {"http://a/b/c/d;p?q", ".g"        , "http://a/b/c/.g"},
        {"http://a/b/c/d;p?q", "g.."       , "http://a/b/c/g.."},
        {"http://a/b/c/d;p?q", "..g"       , "http://a/b/c/..g"},
        {"http://a/b/c/d;p?q", "./../g"    , "http://a/b/g"},
        {"http://a/b/c/d;p?q", "./g/."     , "http://a/b/c/g/"},
        {"http://a/b/c/d;p?q", "g/./h"     , "http://a/b/c/g/h"},
        {"http://a/b/c/d;p?q", "g/../h"    , "http://a/b/c/h"},
        {"http://a/b/c/d;p?q", "g;x=1/./y" , "http://a/b/c/g;x=1/y"},
        {"http://a/b/c/d;p?q", "g;x=1/../y", "http://a/b/c/y"},
        {"http://a/b/c/d;p?q", "g?y/./x"   , "http://a/b/c/g?y/./x"},
        {"http://a/b/c/d;p?q", "g?y/../x"  , "http://a/b/c/g?y/../x"},
        {"http://a/b/c/d;p?q", "g#s/./x"   , "http://a/b/c/g#s/./x"},
        {"http://a/b/c/d;p?q", "g#s/../x"  , "http://a/b/c/g#s/../x"},
        {"http://a/b/c/d;p?q", "http:g"    , "http:g"}, // http://a/b/c/g
        {"http://a/b/c/d;p?q", "http:"     , "http:"}, // "http://a/b/c/d;p?q"

        // not sure where this one originated
        {"http://a/b/c/d;p?q", "/a/b/c/./../../g", "http://a/a/g"},

        // http://gbiv.com/protocols/uri/test/rel_examples2.html
        // slashes in base URI's query args
        {"http://a/b/c/d;p?q=1/2", "g"  , "http://a/b/c/g"},
        {"http://a/b/c/d;p?q=1/2", "./g", "http://a/b/c/g"},
        {"http://a/b/c/d;p?q=1/2", "g/" , "http://a/b/c/g/"},
        {"http://a/b/c/d;p?q=1/2", "/g" , "http://a/g"},
        {"http://a/b/c/d;p?q=1/2", "//g", "http://g"},

        // changed in RFC 2396bis
        {"http://a/b/c/d;p?q=1/2", "?y"      , "http://a/b/c/d;p?y"},
        {"http://a/b/c/d;p?q=1/2", "g?y"     , "http://a/b/c/g?y"},
        {"http://a/b/c/d;p?q=1/2", "g?y/./x" , "http://a/b/c/g?y/./x"},
        {"http://a/b/c/d;p?q=1/2", "g?y/../x", "http://a/b/c/g?y/../x"},
        {"http://a/b/c/d;p?q=1/2", "g#s"     , "http://a/b/c/g#s"},
        {"http://a/b/c/d;p?q=1/2", "g#s/./x" , "http://a/b/c/g#s/./x"},
        {"http://a/b/c/d;p?q=1/2", "g#s/../x", "http://a/b/c/g#s/../x"},
        {"http://a/b/c/d;p?q=1/2", "./"      , "http://a/b/c/"},
        {"http://a/b/c/d;p?q=1/2", "../"     , "http://a/b/"},
        {"http://a/b/c/d;p?q=1/2", "../g"    , "http://a/b/g"},
        {"http://a/b/c/d;p?q=1/2", "../../"  , "http://a/"},
        {"http://a/b/c/d;p?q=1/2", "../../g" , "http://a/g"},

        // http://gbiv.com/protocols/uri/test/rel_examples3.html
        // slashes in path params
        // all of these changed in RFC 2396bis
        {"http://a/b/c/d;p=1/2?q", "g"         , "http://a/b/c/d;p=1/g"},
        {"http://a/b/c/d;p=1/2?q", "./g"       , "http://a/b/c/d;p=1/g"},
        {"http://a/b/c/d;p=1/2?q", "g/"        , "http://a/b/c/d;p=1/g/"},
        {"http://a/b/c/d;p=1/2?q", "g?y"       , "http://a/b/c/d;p=1/g?y"},
        {"http://a/b/c/d;p=1/2?q", ";x"        , "http://a/b/c/d;p=1/;x"},
        {"http://a/b/c/d;p=1/2?q", "g;x"       , "http://a/b/c/d;p=1/g;x"},
        {"http://a/b/c/d;p=1/2?q", "g;x=1/./y" , "http://a/b/c/d;p=1/g;x=1/y"},
        {"http://a/b/c/d;p=1/2?q", "g;x=1/../y", "http://a/b/c/d;p=1/y"},
        {"http://a/b/c/d;p=1/2?q", "./"        , "http://a/b/c/d;p=1/"},
        {"http://a/b/c/d;p=1/2?q", "../"       , "http://a/b/c/"},
        {"http://a/b/c/d;p=1/2?q", "../g"      , "http://a/b/c/g"},
        {"http://a/b/c/d;p=1/2?q", "../../"    , "http://a/b/"},
        {"http://a/b/c/d;p=1/2?q", "../../g"   , "http://a/b/g"},

        // http://gbiv.com/protocols/uri/test/rel_examples4.html
        // double and triple slash, unknown scheme
        {"fred:///s//a/b/c", "g:h"          , "g:h"},
        {"fred:///s//a/b/c", "g"            , "fred:///s//a/b/g"},
        {"fred:///s//a/b/c", "./g"          , "fred:///s//a/b/g"},
        {"fred:///s//a/b/c", "g/"           , "fred:///s//a/b/g/"},
        {"fred:///s//a/b/c", "/g"           , "fred:///g"}, // may change to fred:///s//a/g
        {"fred:///s//a/b/c", "//g"          , "fred://g"}, // may change to fred:///s//g
        {"fred:///s//a/b/c", "//g/x"        , "fred://g/x"}, // may change to fred:///s//g/x
        {"fred:///s//a/b/c", "///g"         , "fred:///g"},
        {"fred:///s//a/b/c", "./"           , "fred:///s//a/b/"},
        {"fred:///s//a/b/c", "../"          , "fred:///s//a/"},
        {"fred:///s//a/b/c", "../g"         , "fred:///s//a/g"},
        {"fred:///s//a/b/c", "../../"       , "fred:///s//"}, // may change to fred:///s//a/../
        {"fred:///s//a/b/c", "../../g"      , "fred:///s//g"}, // may change to fred:///s//a/../g
        {"fred:///s//a/b/c", "../../../g"   , "fred:///s/g"}, // may change to fred:///s//a/../../g
        {"fred:///s//a/b/c", "../../../../g", "fred:///g"}, // may change to fred:///s//a/../../../g

        // http://gbiv.com/protocols/uri/test/rel_examples5.html
        // double and triple slash, well-known scheme
        {"http:///s//a/b/c", "g:h"          , "g:h"},
        {"http:///s//a/b/c", "g"            , "http:///s//a/b/g"},
        {"http:///s//a/b/c", "./g"          , "http:///s//a/b/g"},
        {"http:///s//a/b/c", "g/"           , "http:///s//a/b/g/"},
        {"http:///s//a/b/c", "/g"           , "http:///g"}, // may change to http:///s//a/g
        {"http:///s//a/b/c", "//g"          , "http://g"}, // may change to http:///s//g
        {"http:///s//a/b/c", "//g/x"        , "http://g/x"}, // may change to http:///s//g/x
        {"http:///s//a/b/c", "///g"         , "http:///g"},
        {"http:///s//a/b/c", "./"           , "http:///s//a/b/"},
        {"http:///s//a/b/c", "../"          , "http:///s//a/"},
        {"http:///s//a/b/c", "../g"         , "http:///s//a/g"},
        {"http:///s//a/b/c", "../../"       , "http:///s//"}, // may change to http:///s//a/../
        {"http:///s//a/b/c", "../../g"      , "http:///s//g"}, // may change to http:///s//a/../g
        {"http:///s//a/b/c", "../../../g"   , "http:///s/g"}, // may change to http:///s//a/../../g
        {"http:///s//a/b/c", "../../../../g", "http:///g"}, // may change to http:///s//a/../../../g

        // from Dan Connelly's tests in http://www.w3.org/2000/10/swap/uripath.py
        {"foo:xyz"                                                     ,"bar:abc"                           ,  "bar:abc"},
        {"http://example/x/y/z"                                        ,"../abc"                            ,  "http://example/x/abc"},
        {"http://example2/x/y/z"                                       ,"http://example/x/abc"              ,  "http://example/x/abc"},
        {"http://ex/x/y/z"                                             ,"../r"                              ,  "http://ex/x/r"},
        {"http://ex/x/y"                                               ,"q/r"                               ,  "http://ex/x/q/r"},
        {"http://ex/x/y"                                               ,"q/r#s"                             ,  "http://ex/x/q/r#s"},
        {"http://ex/x/y"                                               ,"q/r#s/t"                           ,  "http://ex/x/q/r#s/t"},
        {"http://ex/x/y"                                               ,"ftp://ex/x/q/r"                    ,  "ftp://ex/x/q/r"},
        {"http://ex/x/y"                                               ,""                                  ,  "http://ex/x/y"},
        {"http://ex/x/y/"                                              ,""                                  ,  "http://ex/x/y/"},
        {"http://ex/x/y/pdq"                                           ,""                                  ,  "http://ex/x/y/pdq"},
        {"http://ex/x/y/"                                              ,"z/"                                ,  "http://ex/x/y/z/"},
        {"file:/swap/test/animal.rdf"                                  ,"#Animal"                           ,  "file:/swap/test/animal.rdf#Animal"},
        {"file:/e/x/y/z"                                               ,"../abc"                            ,  "file:/e/x/abc"},
        {"file:/example2/x/y/z"                                        ,"/example/x/abc"                    ,  "file:/example/x/abc"},
        {"file:/ex/x/y/z"                                              ,"../r"                              ,  "file:/ex/x/r"},
        {"file:/ex/x/y/z"                                              ,"/r"                                ,  "file:/r"},
        {"file:/ex/x/y"                                                ,"q/r"                               ,  "file:/ex/x/q/r"},
        {"file:/ex/x/y"                                                ,"q/r#s"                             ,  "file:/ex/x/q/r#s"},
        {"file:/ex/x/y"                                                ,"q/r#"                              ,  "file:/ex/x/q/r#"},
        {"file:/ex/x/y"                                                ,"q/r#s/t"                           ,  "file:/ex/x/q/r#s/t"},
        {"file:/ex/x/y"                                                ,"ftp://ex/x/q/r"                    ,  "ftp://ex/x/q/r"},
        {"file:/ex/x/y"                                                ,""                                  ,  "file:/ex/x/y"},
        {"file:/ex/x/y/"                                               ,""                                  ,  "file:/ex/x/y/"},
        {"file:/ex/x/y/pdq"                                            ,""                                  ,  "file:/ex/x/y/pdq"},
        {"file:/ex/x/y/"                                               ,"z/"                                ,  "file:/ex/x/y/z/"},
        {"file:/devel/WWW/2000/10/swap/test/reluri-1.n3"               ,"file://meetings.example.com/cal#m1",  "file://meetings.example.com/cal#m1"},
        {"file:/home/connolly/w3ccvs/WWW/2000/10/swap/test/reluri-1.n3","file://meetings.example.com/cal#m1",  "file://meetings.example.com/cal#m1"},
        {"file:/some/dir/foo"                                          ,"./#blort"                          ,  "file:/some/dir/#blort"},
        {"file:/some/dir/foo"                                          ,"./#"                               ,  "file:/some/dir/#"},

        // Ryan Lee
        {"http://example/x/abc.efg", "./", "http://example/x/"},

        // Graham Klyne's tests
        // http://www.ninebynine.org/Software/HaskellUtils/Network/UriTest.xls
        // 01-31 are from Connelly's cases

        // 32-49
        {"http://ex/x/y"                 , "./q:r"                      , "http://ex/x/q:r"},
        {"http://ex/x/y"                 , "./p=q:r"                    , "http://ex/x/p=q:r"},
        {"http://ex/x/y?pp/qq"           , "?pp/rr"                     , "http://ex/x/y?pp/rr"},
        {"http://ex/x/y?pp/qq"           , "y/z"                        , "http://ex/x/y/z"},
        {"mailto:local"                  , "local/qual@domain.org#frag" , "mailto:local/qual@domain.org#frag"},
        {"mailto:local/qual1@domain1.org", "more/qual2@domain2.org#frag", "mailto:local/more/qual2@domain2.org#frag"},
        {"http://ex/x/y?q"               , "y?q"                        , "http://ex/x/y?q"},
        {"http://ex?p"                   , "/x/y?q"                     , "http://ex/x/y?q"},
        {"foo:a/b"                       , "c/d"                        , "foo:a/c/d"},
        {"foo:a/b"                       , "/c/d"                       , "foo:/c/d"},
        {"foo:a/b?c#d"                   , ""                           , "foo:a/b?c"},
        {"foo:a"                         , "b/c"                        , "foo:b/c"},
        {"foo:/a/y/z"                    , "../b/c"                     , "foo:/a/b/c"},
        {"foo:a"                         , "./b/c"                      , "foo:b/c"},
        {"foo:a"                         , "/./b/c"                     , "foo:/b/c"},
        {"foo://a//b/c"                  , "../../d"                    , "foo://a/d"},
        {"foo:a"                         , "."                          , "foo:"},
        {"foo:a"                         , ".."                         , "foo:"},

        // 50-57 (cf. TimBL comments --
        // http://lists.w3.org/Archives/Public/uri/2003Feb/0028.html,
        // http://lists.w3.org/Archives/Public/uri/2003Jan/0008.html)
        {"http://example/x/y%2Fz"  , "abc"          , "http://example/x/abc"},
        {"http://example/a/x/y/z"  , "../../x%2Fabc", "http://example/a/x%2Fabc"},
        {"http://example/a/x/y%2Fz", "../x%2Fabc"   , "http://example/a/x%2Fabc"},
        {"http://example/x%2Fy/z"  , "abc"          , "http://example/x%2Fy/abc"},
        {"http://ex/x/y"           , "q%3Ar"        , "http://ex/x/q%3Ar"},
        {"http://example/x/y%2Fz"  , "/x%2Fabc"     , "http://example/x%2Fabc"},
        {"http://example/x/y/z"    , "/x%2Fabc"     , "http://example/x%2Fabc"},
        {"http://example/x/y%2Fz"  , "/x%2Fabc"     , "http://example/x%2Fabc"},

        // 70-77
        {"mailto:local1@domain1?query1", "local2@domain2"           , "mailto:local2@domain2"},
        {"mailto:local1@domain1"       , "local2@domain2?query2"    , "mailto:local2@domain2?query2"},
        {"mailto:local1@domain1?query1", "local2@domain2?query2"    , "mailto:local2@domain2?query2"},
        {"mailto:local@domain?query1"  , "?query2"                  , "mailto:local@domain?query2"},
        {"mailto:?query1"              , "local@domain?query2"      , "mailto:local@domain?query2"},
        {"mailto:local@domain?query1"  , "?query2"                  , "mailto:local@domain?query2"},
        {"foo:bar"                     , "http://example/a/b?c/../d", "http://example/a/b?c/../d"},
        {"foo:bar"                     , "http://example/a/b#c/../d", "http://example/a/b#c/../d"},

        // 82-88
        {"http://example.org/base/uri", "http:this", "http:this"},
        {"http:base"                  , "http:this", "http:this"},
        // Whole in the URI spec, see http://lists.w3.org/Archives/Public/uri/2007Aug/0003.html
        {"f:/a"                                                 , ".//g"                         , "f://g"}, // ORIGINAL
        //{"f:/a"                                                 , ".//g"                         , "f:/.//g"}, // FIXED ONE
        {"f://example.org/base/a"                               , "b/c//d/e"                     , "f://example.org/base/b/c//d/e"},
        {"mid:m@example.ord/c@example.org"                      , "m2@example.ord/c2@example.org", "mid:m@example.ord/m2@example.ord/c2@example.org"},
        {"file:///C:/DEV/Haskell/lib/HXmlToolbox-3.01/examples/", "mini1.xml"                    , "file:///C:/DEV/Haskell/lib/HXmlToolbox-3.01/examples/mini1.xml"},
        {"foo:a/y/z"                                            , "../b/c"                       , "foo:a/b/c"}
    };

    for(auto [base, ref, tar] : cases)
    {
        //JKL_LOG << "base=" << base;
        //JKL_LOG << "ref =" << ref;
        //JKL_LOG << "tar =" << tar;
        CHECK(uri_resolve_ret<as_is_codec, std::string>(uri_uri(base), uri_uri(ref)).value_or_throw() == tar);
//         CHECK(uri_resolve_ret<url_encoded, std::string>(uri_uri<as_is_codec>(base), uri_uri<url_decoded>(ref)).value_or_throw() == tar);
//         CHECK(uri_resolve_ret<url_encoded, std::string>(uri_uri<url_decoded>(base), uri_uri<url_decoded>(ref)).value_or_throw() == tar);
//         CHECK(uri_resolve_ret<url_decoded, std::string>(uri_uri<url_encoded>(base), uri_uri<url_encoded>(ref)).value_or_throw() == tar);
//         CHECK(uri_resolve_ret<url_decoded, std::string>(uri_uri<url_decoded>(base), uri_uri<url_encoded>(ref)).value_or_throw() == tar);
//         CHECK(uri_resolve_ret<url_encoded, std::string>(uri_uri<url_decoded>(base), uri_uri<url_encoded>(ref)).value_or_throw() == tar);
//         CHECK(uri_resolve_ret<url_encoded, std::string>(uri_uri<url_encoded>(base), uri_uri<url_decoded>(ref)).value_or_throw() == tar);
    }
} // TEST_CASE("resolve")


TEST_CASE("normalize and compare"){

    constexpr auto equal = [](_char_str_ auto const& r, _char_str_ auto const& l) -> bool
    {
        auto&& nr = uri_normalize_ret<url_decoded>(uri_uri<url_encoded>(r));
        if(! nr)
            return false;
        auto&& nl = uri_normalize_ret<url_decoded>(uri_uri<url_encoded>(l));
        if(! nl)
            return false;
        return nr.value() == nl.value();
    };

    CHECK(equal("HTTP://www.EXAMPLE.com/", "http://www.example.com/"));
    CHECK(equal("example://A/b/c/%7bfoo%7d", "example://a/b/c/%7Bfoo%7D"));
    CHECK(equal("http://host/%7Euser/x/y/z", "http://host/~user/x/y/z"));
    CHECK(equal("http://host/%7euser/x/y/z", "http://host/~user/x/y/z"));
    CHECK(equal("/a/b/../../c", "/c"));
    // CHECK(equal("a/b/../../c", "a/b/../../c"));
    // Fixed:
    CHECK(equal("a/b/../../c", "c"));
    CHECK(equal("/a/b/././c", "/a/b/c"));
    // CHECK(equal("a/b/././c", "a/b/././c"));
    // Fixed:
    CHECK(equal("a/b/././c", "a/b/c"));
    CHECK(equal("/a/b/../c/././d", "/a/c/d"));
    // CHECK(equal("a/b/../c/././d", "a/b/../c/././d"));
    // Fixed:
    CHECK(equal("a/b/../c/././d", "a/c/d"));

} // TEST_CASE("scheme host port")


TEST_CASE("parse_data_url"){

    constexpr auto parse = [](_char_str_ auto const& u, bool valid,
                              string_view const& mime = "", string_view const& charset = "", string_view const& data = "",
                              bool const is_base64 = false) -> bool
    {
        auto r = parse_data_url(u);

        if(! valid)
            return r.has_error();

        if(r.has_error())
            return false;

        return r->is_base64 == is_base64
            && r->mime      == mime
            && r->charset   == charset
            && r->data      == data;
    };

    CHECK(parse("data:"                            , false));
    CHECK(parse("data:,"                           , true , "text/plain", "us-ascii"));
    CHECK(parse("data:;base64,"                    , true , "text/plain", "us-ascii", "", true));
    CHECK(parse("data:;charset=,test"              , false));
    CHECK(parse("data:TeXt/HtMl,<b>x</b>"          , true , "text/html", "", "<b>x</b>"));
    CHECK(parse("data:,foo"                        , true , "text/plain", "us-ascii", "foo"));
    CHECK(parse("data:;base64,aGVsbG8gd29ybGQ="    , true , "text/plain", "us-ascii", "hello world", true));
    CHECK(parse("data:foo,boo"                     , false));
    CHECK(parse("data:foo;charset=UTF-8,boo"       , false));
    CHECK(parse("data:foo/bar;baz=1;charset=kk,boo", true , "foo/bar", "kk", "boo"));
    CHECK(parse("data:foo/bar;charset=kk;baz=1,boo", true , "foo/bar", "kk", "boo"));
    CHECK(parse("data:text/html,%3Chtml%3E%3Cbody%3E%3Cb%3Ehello%20world%3C%2Fb%3E%3C%2Fbody%3E%3C%2Fhtml%3E",
                true, "text/html", "", "<html><body><b>hello world</b></body></html>"));
    CHECK(parse("data:text/html,<html><body><b>hello world</b></body></html>",
                true, "text/html", "", "<html><body><b>hello world</b></body></html>"));
    CHECK(parse("data:%2Cblah"                                  , false));
    CHECK(parse("data:;base64,aGVs_-_-"                         , false));
    CHECK(parse("data:text/plain;charset=utf-8;base64,SGVsbMO2" , true , "text/plain", "utf-8", "Hell\xC3\xB6", true));
    CHECK(parse("data:;charset=utf-8;base64,SGVsbMO2"           , true , "text/plain", "utf-8", "Hell\xC3\xB6", true));
    CHECK(parse("data:;base64,aGVsbG8gd29yb"                    , false));
    CHECK(parse("data:text/plain;charset=utf-8,\xE2\x80\x8Ftest", true , "text/plain", "utf-8", "\xE2\x80\x8Ftest"));
    CHECK(parse("data:text/plain;charset=utf-8,\xE2\x80\x8F\xD8\xA7\xD8\xAE\xD8\xAA\xD8\xA8\xD8\xA7\xD8\xB1",
                true, "text/plain", "utf-8", "\xE2\x80\x8F\xD8\xA7\xD8\xAE\xD8\xAA\xD8\xA8\xD8\xA7\xD8\xB1"));
    CHECK(parse("data:text/plain;charset=utf-8,%E2%80%8Ftest", true, "text/plain", "utf-8", "\xE2\x80\x8Ftest"));
    CHECK(parse("data:text/plain;charset=utf-8,%E2%80%8F\xD8\xA7\xD8\xAE\xD8\xAA\xD8\xA8\xD8\xA7\xD8\xB1",
                true, "text/plain", "utf-8", "\xE2\x80\x8F\xD8\xA7\xD8\xAE\xD8\xAA\xD8\xA8\xD8\xA7\xD8\xB1"));
    CHECK(parse("data:%00text/plain%41,foo", true, "%00text/plain%41", "", "foo"));
    CHECK(parse("data:text/plain;charset=%00US-ASCII%41,foo", true, "text/plain", "%00us-ascii%41", "foo"));
    CHECK(parse("data:text/plain;%62ase64,AA//", true, "text/plain", "", "AA//"));

} // TEST_CASE("scheme host port")


} // TEST_SUITE("uri")
