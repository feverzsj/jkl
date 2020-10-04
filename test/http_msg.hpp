#pragma once

#include <jkl/util/log.hpp>
#include <jkl/http_msg.hpp>

#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>


TEST_SUITE("http_msg"){

using namespace jkl;

TEST_CASE("parse_content_type"){

    constexpr auto parse = [](_char_str_ auto const& s
                              string_view const& mime = "", string_view const& charset = "", string_view const& boundary = "") -> bool
    {
        http_field_content_type c;
        c.parse(s);

        return c.mime     == mime
            && c.charset  == charset
            && c.boundary == boundary;
    };

    CHECK(parse("text/html"                                             , "text/html"));
    CHECK(parse("text/html;"                                            , "text/html"));
    CHECK(parse("text/html; charset=utf-8"                              , "text/html", "utf-8"));
    CHECK(parse("text/html; charset =utf-8"                             , "text/html"));
    CHECK(parse("text/html; charset= utf-8"                             , "text/html", "utf-8"));
    CHECK(parse("text/html; charset=utf-8 "                             , "text/html", "utf-8"));
    CHECK(parse("text/html; boundary=\"WebKit-ada-df-dsf-adsfadsfs\""   , "text/html", "", "WebKit-ada-df-dsf-adsfadsfs"));
    CHECK(parse("text/html; boundary =\"WebKit-ada-df-dsf-adsfadsfs\""  , "text/html"));
    CHECK(parse("text/html; boundary= \"WebKit-ada-df-dsf-adsfadsfs\""  , "text/html", "", "WebKit-ada-df-dsf-adsfadsfs"));
    CHECK(parse("text/html; boundary= \"WebKit-ada-df-dsf-adsfadsfs\"  ", "text/html", "", "WebKit-ada-df-dsf-adsfadsfs"));
    CHECK(parse("text/html; boundary=\"WebKit-ada-df-dsf-adsfadsfs  \"" , "text/html", "", "WebKit-ada-df-dsf-adsfadsfs"));
    CHECK(parse("text/html; boundary=WebKit-ada-df-dsf-adsfadsfs"       , "text/html", "", "WebKit-ada-df-dsf-adsfadsfs"));
    CHECK(parse("text/html; charset"                                    , "text/html"));
    CHECK(parse("text/html; charset="                                   , "text/html"));
    CHECK(parse("text/html; charset= "                                  , "text/html"));
    CHECK(parse("text/html; charset= ;"                                 , "text/html"));
    CHECK(parse("text/html; charset=\"\""                               , "text/html"));
    CHECK(parse("text/html; charset=\" \""                              , "text/html"));
    CHECK(parse("text/html; charset=\" foo \""                          , "text/html", "foo"));
    CHECK(parse("text/html; charset=foo; charset=utf-8"                 , "text/html", "foo"));
    CHECK(parse("text/html; charset; charset=; charset=utf-8"           , "text/html", "utf-8"));
    CHECK(parse("text/html; charset=utf-8; charset=; charset"           , "text/html", "utf-8"));
    CHECK(parse("text/html; boundary=foo; boundary=bar"                 , "text/html", "", "foo"));
    CHECK(parse("text/html; \"; \"\"; charset=utf-8"                    , "text/html", "utf-8"));
    CHECK(parse("text/html; charset=u\"tf-8\""                          , "text/html", "u\"tf-8\""));
    CHECK(parse("text/html; charset=\"utf-8\""                          , "text/html", "utf-8"));
    CHECK(parse("text/html; charset=\"utf-8"                            , "text/html", "utf-8"));
    CHECK(parse("text/html; charset=\"\\utf\\-\\8\""                    , "text/html", "utf-8"));
    CHECK(parse("text/html; charset=\"\\\\\\\"\\"                       , "text/html", "\\\"\\"));
    CHECK(parse("text/html; charset=\";charset=utf-8;\""                , "text/html", ";charset=utf-8;"));
    CHECK(parse("text/html; charset= \"utf-8\""                         , "text/html", "utf-8"));
    CHECK(parse("text/html; charset='utf-8'"                            , "text/html", "'utf-8'"));

} // TEST_CASE("parse_content_type")


} // TEST_SUITE("http_msg")
