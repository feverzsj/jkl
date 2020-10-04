#pragma once

#include <jkl/config.hpp>
#include <jkl/util/str.hpp>


namespace jkl{


constexpr unsigned short to_well_known_port(string_view const& scheme) noexcept
{
    if(scheme == "https" ) return 443;
    if(scheme == "http"  ) return 80;
    if(scheme == "wss"   ) return 443;
    if(scheme == "ws"    ) return 80;
    if(scheme == "ftp"   ) return 21;
    if(scheme == "ssh"   ) return 22;
    if(scheme == "telnet") return 23;
    if(scheme == "smtp"  ) return 25;
    if(scheme == "dns"   ) return 53;
    if(scheme == "nntp"  ) return 119;
    if(scheme == "imap"  ) return 143;
    if(scheme == "ldap"  ) return 389;
    if(scheme == "smtps" ) return 465;
    if(scheme == "rtsp"  ) return 554;
    if(scheme == "ldaps" ) return 636;
    if(scheme == "dnss"  ) return 853;
    if(scheme == "imaps" ) return 993;
    if(scheme == "sip"   ) return 5060;
    if(scheme == "sips"  ) return 5061;
    if(scheme == "xmpp"  ) return 5222;

    return 0;
}

constexpr unsigned short prefix_to_well_known_port(string_view const& s) noexcept
{
    if(s.starts_with("https:" )) return 443;
    if(s.starts_with("http:"  )) return 80;
    if(s.starts_with("wss:"   )) return 443;
    if(s.starts_with("ws:"    )) return 80;
    if(s.starts_with("ftp:"   )) return 21;
    if(s.starts_with("ssh:"   )) return 22;
    if(s.starts_with("telnet:")) return 23;
    if(s.starts_with("smtp:"  )) return 25;
    if(s.starts_with("dns:"   )) return 53;
    if(s.starts_with("nntp:"  )) return 119;
    if(s.starts_with("imap:"  )) return 143;
    if(s.starts_with("ldap:"  )) return 389;
    if(s.starts_with("smtps:" )) return 465;
    if(s.starts_with("rtsp:"  )) return 554;
    if(s.starts_with("ldaps:" )) return 636;
    if(s.starts_with("dnss:"  )) return 853;
    if(s.starts_with("imaps:" )) return 993;
    if(s.starts_with("sip:"   )) return 5060;
    if(s.starts_with("sips:"  )) return 5061;
    if(s.starts_with("xmpp:"  )) return 5222;

    return 0;
}


} // namespace jkl
