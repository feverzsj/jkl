#pragma once

#include <jkl/config.hpp>


namespace jkl{


struct uri_tag{};

struct uri_scheme_tag    : uri_tag{};
struct uri_user_tag      : uri_tag{};
struct uri_password_tag  : uri_tag{};
struct uri_host_tag      : uri_tag{};
struct uri_port_tag      : uri_tag{};
struct uri_auto_port_tag : uri_tag{}; // For reader: if port part doesn't present, assigns well known port. For writer: if the port is well known port of scheme, don't write it.
struct uri_path_tag      : uri_tag{};
struct uri_query_tag     : uri_tag{};
struct uri_frag_tag      : uri_tag{};

struct uri_scheme_colon_tag     : uri_tag{}; // 'scheme:'
struct uri_userinfo_tag         : uri_tag{};
struct uri_userinfo_at_tag      : uri_tag{}; // 'userinfo@'
struct uri_host_port_tag        : uri_tag{};
struct uri_authority_tag        : uri_tag{};
struct uri_ss_authority_tag     : uri_tag{}; // '//authority', use this to check if authority actually presents in case empty authority.
struct uri_scheme_authority_tag : uri_tag{};
struct uri_qm_query_tag         : uri_tag{}; // '?query'
struct uri_sharp_frag_tag       : uri_tag{}; // '#frag'
struct uri_qm_query_frag_tag    : uri_tag{}; // including leading '?query#frag'

struct uri_dir_path_tag     : uri_tag{};     // path without filename[.ext] part
struct uri_file_tag         : uri_tag{};         // filename[.ext]
struct uri_file_name_tag    : uri_tag{};    // filename
struct uri_file_ext_tag     : uri_tag{};     // ext
struct uri_file_dot_ext_tag : uri_tag{}; // .ext

struct uri_uri_tag : uri_tag{};


enum uri_codec_e
{
    as_is_codec,  // read/write as is
    url_encoded,
    url_decoded
};


} // namespace jkl
