#pragma once

#include <jkl/config.hpp>
#include <jkl/charset/ascii.hpp>
#include <jkl/util/str.hpp>
#include <jkl/util/unordered_map_set.hpp>


namespace jkl{


// chromium/git/net/base/mime_util.cc

struct mime_info
{
    string_view mime;
    string_view exts; // Comma-separated list of possible extensions for the type. The first extension is considered preferred.
};

// NOTE: order matters when search by extension
inline constexpr mime_info g_mime_infos[] =
{
    /// primary

    {"video/webm"                    , "webm"               }, // Must precede audio/webm
    {"audio/mpeg"                    , "mp3"                }, // Must precede audio/mp3
    {"application/wasm"              , "wasm"               },
    {"application/x-chrome-extension", "crx"                },
    {"application/xhtml+xml"         , "xhtml,xht,xhtm"     },
    {"audio/flac"                    , "flac"               },
    {"audio/mp3"                     , "mp3"                },
    {"audio/ogg"                     , "ogg,oga,opus"       },
    {"audio/wav"                     , "wav"                },
    {"audio/webm"                    , "webm"               },
    {"audio/x-m4a"                   , "m4a"                },
    {"image/avif"                    , "avif"               },
    {"image/gif"                     , "gif"                },
    {"image/jpeg"                    , "jpeg,jpg"           },
    {"image/png"                     , "png"                },
    {"image/apng"                    , "png"                },
    {"image/webp"                    , "webp"               },
    {"multipart/related"             , "mht,mhtml"          },
    {"text/css"                      , "css"                },
    {"text/html"                     , "html,htm,shtml,shtm"},
    {"text/javascript"               , "js,mjs"             },
    {"text/xml"                      , "xml"                },
    {"video/mp4"                     , "mp4,m4v"            },
    {"video/ogg"                     , "ogv,ogm"            },

    /// secondary
    
    {"image/x-icon"                           , "ico"           }, // Must precede image/vnd.microsoft.icon
    {"application/epub+zip"                   , "epub"          },
    {"application/font-woff"                  , "woff"          },
    {"application/gzip"                       , "gz,tgz"        },
    {"application/javascript"                 , "js"            },
    {"application/json"                       , "json"          },  // Per http://www.ietf.org/rfc/rfc4627.txt.
    {"application/octet-stream"               , "bin,exe,com"   },
    {"application/pdf"                        , "pdf"           },
    {"application/pkcs7-mime"                 , "p7m,p7c,p7z"   },
    {"application/pkcs7-signature"            , "p7s"           },
    {"application/postscript"                 , "ps,eps,ai"     },
    {"application/rdf+xml"                    , "rdf"           },
    {"application/rss+xml"                    , "rss"           },
    {"application/vnd.android.package-archive", "apk"           },
    {"application/vnd.mozilla.xul+xml"        , "xul"           },
    {"application/x-gzip"                     , "gz,tgz"        },
    {"application/x-mpegurl"                  , "m3u8"          },
    {"application/x-shockwave-flash"          , "swf,swl"       },
    {"application/x-tar"                      , "tar"           },
    {"application/x-x509-ca-cert"             , "cer,crt"       },
    {"application/zip"                        , "zip"           },
    {"audio/webm"                             , "weba"          },
    {"image/bmp"                              , "bmp"           },
    {"image/jpeg"                             , "jfif,pjpeg,pjp"},
    {"image/svg+xml"                          , "svg,svgz"      },
    {"image/tiff"                             , "tiff,tif"      },
    {"image/vnd.microsoft.icon"               , "ico"           },
    {"image/x-png"                            , "png"           },
    {"image/x-xbitmap"                        , "xbm"           },
    {"message/rfc822"                         , "eml"           },
    {"text/calendar"                          , "ics"           },
    {"text/html"                              , "ehtml"         },
    {"text/plain"                             , "txt,text"      },
    {"text/x-sh"                              , "sh"            },
    {"text/xml"                               , "xsl,xbl,xslt"  },
    {"video/mpeg"                             , "mpeg,mpg"      }
};



inline string_view* wellknown_mime_for_ext(string_view ext) noexcept
{
    if(ext.size() > 65536)
        return nullptr; // Avoids crash when unable to handle a long file path. See crbug.com/48733.
    
    if(ext.find('\0') != npos)
        return nullptr; // Reject a string which contains null character.

    if(ext.starts_with('.'))
        ext.remove_prefix(1);
    if(ext.empty())
        return nullptr;
    
    for(mime_info const& info : g_mime_infos)
    {
        string_view const& exts = info.exts;

        for(auto b = 0;;)
        {
            auto e = exts.find(',', b);

            if(ascii_iequal(exts.substr(b, e - b), ext))
                return & info.mime;

            if(e == npos)
                break;

            b = e + 1;
        }
    }

    return nullptr;
}


string_view preferred_ext_for_mime(string_view const& mime, string_view const& def = {}) noexcept
{
    if(mime == "application/octet-stream")
        return def; // There is no preferred extension for "application/octet-stream".

    for(mime_info const& info : g_mime_infos)
    {
        if(info.mime == mime)
            return info.mime.substr(0, info.mime.find(','));
    }

    return def;
}


} // namespace jkl
