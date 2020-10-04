#pragma once

#include <jkl/config.hpp>
#include <jkl/util/params.hpp>


namespace jkl{


// NOTE: parameters here should be small object which passed by value.

inline constexpr struct t_stop_enabled_t{} t_stop_enabled;
inline constexpr auto p_enable_stop  = [](t_stop_enabled_t){ return  true; };
inline constexpr auto p_disable_stop = [](t_stop_enabled_t){ return false; };


inline constexpr struct t_expiry_dur_t{} t_expiry_dur;
inline constexpr auto p_expires_never = [](t_expiry_dur_t){ return null_op; };
inline constexpr auto p_expires_after = [](auto const& dur) noexcept { return [dur](t_expiry_dur_t){ return dur; }; };


inline constexpr struct t_read_some_t{} t_read_some;
template<unsigned Bits> inline constexpr auto p_read_some_bits = [](t_read_some_t){ return Bits; };
inline constexpr auto p_read_some_none = p_read_some_bits<0>;
inline constexpr auto p_read_some_all  = p_read_some_bits<UINT_MAX>;
inline constexpr auto p_read_some      = p_read_some_all;
inline constexpr auto p_read_some_1    = p_read_some_bits<1>;
inline constexpr auto p_read_some_2    = p_read_some_bits<2>;


inline constexpr struct t_skip_frag_t{} t_skip_frag;
inline constexpr auto p_skip_frag = [](t_skip_frag_t){ return  true; };
inline constexpr auto p_keep_frag = [](t_skip_frag_t){ return false; };


} // namespace jkl
