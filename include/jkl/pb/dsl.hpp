#pragma once

#include <jkl/config.hpp>
#include <jkl/result.hpp>
#include <jkl/util/tuple.hpp>
#include <jkl/util/params.hpp>
#include <jkl/util/endian.hpp>
#include <jkl/util/strlit.hpp>
#include <jkl/util/stringify.hpp>
#include <jkl/pb/type.hpp>
#include <jkl/pb/error.hpp>
#include <jkl/pb/varint.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <bitset>


namespace jkl{

// modules proto2
//
// limitations:
//      enum, sub message not supported;
//      repeated field cannot be interleaved;
//      merging message(which contains required sub fields) is not supported, so a non repeated message field can only present on wire once;
//      by default strings are not validated as utf8.


using mp11::mp_list, mp11::mp_append, mp11::mp_first, mp11::mp_second, mp11::mp_repeat, mp11::mp_size, mp11::mp_plus,
      mp11::mp_fold, mp11::mp_transform, mp11::mp_transform_q, mp11::mp_unique, mp11::mp_unique_if, mp11::mp_at_c,
      mp11::mp_all_of,
      mp11::mp_bool, mp11::mp_size_t, mp11::mp_iota;


template<class Msg1, class Msg2>
using __equal_type_name = mp_bool<Msg1::f_type == Msg2::f_type>;

// msgs must be passed in define order.
template<_resizable_byte_buf_ S = string, strlit... ExtraHeaders>
constexpr S pb_gen_def(auto const&... msgs)
{
    // make sure there is no duplicated type name
    static_assert(
        sizeof...(msgs) == mp_size<mp_unique_if<
            mp_list<JKL_DECL_NO_CVREF_T(msgs)...>, __equal_type_name>>::value);

    S def;

    append_str(def,  "syntax = \"proto2\"\n\n");

    if constexpr(sizeof...(ExtraHeaders))
    {
        (..., append_str(def, ExtraHeaders, "\n"));
        append_str(def,  "\n");
    }

    (... , append_str(def, msgs.template msg_def<S>(), "\n\n"));

    return def;
}


constexpr bool is_pb_reserved_word(_str_ auto const& s) noexcept
{
    return is_oneof(s, "int32", "int64", "sint32", "sint64", "uint32", "uint64", "bool",
                       "fixed32", "fixed64", "sfixed32", "sfixed64", "float", "double",
                       "string", "bytes", "message", "optional", "repeated", "group");
}


inline constexpr struct t_validate_t {} t_validate;
inline constexpr struct t_val_t      {} t_val;
inline constexpr struct t_optional_t {} t_optional;
inline constexpr struct t_default_t  {} t_default;
inline constexpr struct t_has_val_t  {} t_has_val;
inline constexpr struct t_clear_val_t{} t_clear_val;
inline constexpr struct t_get_member_t       {} t_get_member;
inline constexpr struct t_activate_member_t  {} t_activate_member;
inline constexpr struct t_active_member_idx_t{} t_active_member_idx;

inline constexpr auto p_validate  = p_forward<t_validate_t>; // on successful reading, corresponding validator will be called, it may return boolean, error_code or aresult<.

inline constexpr auto p_optional  = [](t_optional_t){ return  true; }; // value is std::optional or _buf_(for bytes/string fields) like, implies optional field
inline constexpr auto p_required  = [](t_optional_t){ return false; };
inline constexpr auto p_val       = p_forward<t_val_t      >; // the value itself or how to access the value, e.g.: data/function member ptr or callable;
inline constexpr auto p_has_val   = p_forward<t_has_val_t  >; // for write only: check if value exists, implies optional field.
inline constexpr auto p_clear_val = p_forward<t_clear_val_t>; // for read only: clear value, so that has_val(d) == false, implies optional field.
inline constexpr auto p_default   = p_forward<t_default_t  >; // same as: has_val(auto& d){ d.m != _default; }, clear_val(auto& d){ d.m = _default; }, implies optional field.

// oneof field member access
inline constexpr auto p_get_member        = p_forward<t_get_member_t       >;
inline constexpr auto p_activate_member   = p_forward<t_activate_member_t  >;
inline constexpr auto p_active_member_idx = p_forward<t_active_member_idx_t>;


#define JKL_P_VALIDATE(Expr) p_validate([](auto& d) { return Expr; })
// e.g.: JKL_P_VAL(d.m)
#define JKL_P_VAL(Expr) p_val([](auto& d) -> auto& { return Expr; })
// e.g.: JKL_P_HAS_VAL(d.m.has_value())
#define JKL_P_HAS_VAL(Expr) p_has_val([](auto& d) -> bool { return Expr; })
// e.g.: JKL_P_CLEAR_VAL(d.m.reset())
#define JKL_P_CLEAR_VAL(Expr) p_clear_val([](auto& d) { Expr; })

#define JKL_P_GET_MEMBER(Expr) p_get_member([](auto& d, _integral_constant_ auto I) -> auto& { return Expr; })
#define JKL_P_ACTIVATE_MEMBER(Expr) p_activate_member([](auto& d, _integral_constant_ auto I) { Expr; })
#define JKL_P_ACTIVE_MEMBER_IDX(Expr) p_active_member_idx([](auto& d) { return Expr; })


// MSVC_WORKAROUND
template<class U, class D>
concept __do_uval_cond1 = requires(U& u, D& d){ d.u(); };
template<class U, class D>
concept __do_uval_cond2 = requires(U& u, D& d){ d.u; };
template<class U, class D>
concept __do_uval_cond3 = requires(U& u, D& d){ u(d); };
template<class U>
concept __has_resize = requires(U& u){ u.resize(226); };
template<class U>
concept __has_clear = requires(U& u){ u.clear(); };
template<class U, class D>
concept __has_map_insert_or_assign = requires(U& u, D& d){ u.insert_or_assign(std::move(d.first), std::move(d.second)); };
template<class U, class D>
concept __has_set_insert = requires(U& u, D& d){ u.insert(std::move(d)); };


// d                  : whole data passed to write/read
// u  = uval(d)       : extracted fld value from d
// cv = const_val  (d): const value from u, u must contain a value
// mv = mutable_val(d): mutable value from u, if u doesn't already contain a value, a new value should be constructed in u.
template<bool IsOptionalUval,
         class Uval, class Default, class HasVal, class ClearVal,
         class GetMem, class ActMem, class ActMemIdx, class Validate>
struct pb_params
{
    static constexpr bool is_optional_uval = IsOptionalUval;

    static constexpr bool has_uval        = ! is_null_op_v<Uval     >;
    static constexpr bool has_default     = ! is_null_op_v<Default  >;
    static constexpr bool has_has_val     = ! is_null_op_v<HasVal   >;
    static constexpr bool has_clear_val   = ! is_null_op_v<ClearVal >;
    static constexpr bool has_get_mem     = ! is_null_op_v<GetMem   >;
    static constexpr bool has_act_mem     = ! is_null_op_v<ActMem   >;
    static constexpr bool has_act_mem_idx = ! is_null_op_v<ActMemIdx>;
    static constexpr bool has_validate    = ! is_null_op_v<Validate >;

    // NOTE: is_optional are overridden in pb_repeated_fld and pb_oneof_fld to be always true
    static constexpr bool is_optional = is_optional_uval || has_default || (has_has_val && has_clear_val);

    [[no_unique_address]] Uval      _uval;
    [[no_unique_address]] Default   _default;
    [[no_unique_address]] HasVal    _hasVal;
    [[no_unique_address]] ClearVal  _clearVal;
    [[no_unique_address]] GetMem    _getMem;
    [[no_unique_address]] ActMem    _actMem;
    [[no_unique_address]] ActMemIdx _actMemIdx;
    [[no_unique_address]] Validate  _validate;

    constexpr auto& default_val() const noexcept
    {
        static_assert(has_default);
        return _default;
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr auto rebind_uval(auto&& v) const
    {
        return pb_params<IsOptionalUval, JKL_DECL_NO_CVREF_T(v),  Default,  HasVal,  ClearVal,  GetMem,  ActMem,  ActMemIdx, Validate>{
                                                 JKL_FORWARD(v), _default, _hasVal, _clearVal, _getMem, _actMem, _actMemIdx, _validate};
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr auto& uval(auto& d) const
    {
        if constexpr(! has_uval)
            return d;
        //else if constexpr(requires{ d._uval(); })
        else if constexpr(__do_uval_cond1<decltype(_uval), decltype(d)>)
            return d._uval();
        //else if constexpr(requires{ d._uval; })
        else if constexpr(__do_uval_cond2<decltype(_uval), decltype(d)>)
            return d._uval;
        //else if constexpr(requires{ _uval(d); })
        else if constexpr(__do_uval_cond3<decltype(_uval), decltype(d)>)
            return _uval(d);
        else
            return _uval;

    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr bool has_val(auto const& d) const noexcept
    {
        if constexpr(is_optional_uval)
            return uval(d).has_value();
        else if constexpr(has_has_val)
            return _hasVal(d);
        else if constexpr(has_default)
            return uval(d) != _default;
        else
            return true;
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr void clear_val(auto& d) const
    {
        if constexpr(is_optional_uval)
            uval(d).reset();
        else if constexpr(has_clear_val)
            _clearVal(d);
        else if constexpr(has_default)
            uval(d) = _default;
        else
            JKL_DEPENDENT_FAIL(decltype(d), "unsupported type");
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    auto const& const_val(auto const& d) const noexcept
    {
        auto const& u = uval(d);

        if constexpr(is_optional_uval)
        {
            BOOST_ASSERT(u.has_value());
            return *u;
        }
        else
        {
            return u;
        }
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    auto& mutable_val(auto& d) const noexcept
    {
        auto& u = uval(d);

        if constexpr(is_optional_uval)
        {
            if(! u)
                u.emplace();
            return *u;
        }
        else
        {
            return u;
        }
    }

    // variant access for oneof
    // NOTE: 0 index implies no active member

    template<size_t I>
    constexpr auto& get_member(auto& d) const noexcept
    {
        static_assert(I != 0);

        if constexpr(has_get_mem)
            return _getMem(d, size_c<I>);
        else
            return std::get<I>(uval(d));
    }

    template<size_t I>
    constexpr void activate_member(auto& d) const
    {
        if(active_member_idx(d) != I)
        {
            if constexpr(has_act_mem)
                _actMem(d, size_c<I>);
            else
                uval(d).template emplace<I>();
        }
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr size_t active_member_idx(auto const& d) const noexcept
    {
        if constexpr(has_act_mem_idx)
            return _actMemIdx(d);
        else
            return uval(d).index();
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr aresult<> validate(auto& d) const noexcept
    {
        if constexpr(has_validate)
        {
            if constexpr(std::is_convertible_v<decltype(_validate(d)), aresult<>>)
                return _validate(d);
            else
                return _validate(d) ? aresult<>{} : pb_err::validation_failed;
        }
        else
        {
            return no_err;
        }
    }
};


using pb_null_params = pb_params<false, null_op_t, null_op_t, null_op_t, null_op_t, null_op_t, null_op_t, null_op_t, null_op_t>;


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr auto make_pb_params(auto&&... p)
{
    auto m = make_params(JKL_FORWARD(p)..., p_required, p_val(null_op), p_default(null_op), p_has_val(null_op), p_clear_val(null_op),
                                            p_get_member(null_op), p_activate_member(null_op), p_active_member_idx(null_op),
                                            p_validate(null_op));

    return pb_params<m(t_optional),
        JKL_DECL_NO_CVREF_T(m(t_val)),        JKL_DECL_NO_CVREF_T(m(t_default)),         JKL_DECL_NO_CVREF_T(m(t_has_val)),
        JKL_DECL_NO_CVREF_T(m(t_clear_val)),
        JKL_DECL_NO_CVREF_T(m(t_get_member)), JKL_DECL_NO_CVREF_T(m(t_activate_member)), JKL_DECL_NO_CVREF_T(m(t_active_member_idx)),
        JKL_DECL_NO_CVREF_T(m(t_validate))>{
                            m(t_val),                             m(t_default),                              m(t_has_val),
                            m(t_clear_val),
                            m(t_get_member),                      m(t_activate_member),                      m(t_active_member_idx),
                            m(t_validate)};
}

static_assert(std::is_same_v<pb_null_params, decltype(make_pb_params())>);


inline constexpr size_t pb_default_len_cache_init_size = 256;


template<strlit Type, strlit Name, uint32_t Id, pb_wire_type Wt, class Derived, class Params>
struct pb_fld : Params
{
    static_assert(! Type.empty());
    static_assert(! is_pb_reserved_word(Name));

    static constexpr auto         f_type          = Type;
    static constexpr auto         f_name          = Name;
    static constexpr uint32_t     f_id            = Id;
    static constexpr pb_wire_type f_wire_type     = Wt;
    static constexpr uint32_t     f_tag           = (Id << 3) | Wt;
    static constexpr pb_size_t    f_tag_wire_size = varint_wire_size(f_tag);

    // fields injected to the enclosing message by this field, mainly for pb_oneof_fld
    using injected_fld_list = mp_list<Derived>;

    template<size_t I, _byte_ B>
    constexpr aresult<B const*> injected_fld_read(B const* beg, B const* end, auto& d) const noexcept
    {
        static_assert(I == 0);
        return derived()->template read_impl<false>(beg, end, d);
    }

    constexpr Derived      * derived()       noexcept { return static_cast<Derived*>(this); }
    constexpr Derived const* derived() const noexcept { return static_cast<Derived const*>(this); }

    template<_resizable_byte_buf_ S = string>
    constexpr S fld_def() const
    {
        static_assert(Id != 0);

        S def;
        append_as_str(def, (Derived::is_optional ? "optional " : "required "), Type, " ", Name, "=", Id);

        if constexpr(Derived::has_default)
            append_as_str(def, " [default = ", derived()->default_val(), "]");

        append_str(def, ";");
        return def;
    }

    template<_byte_ B>
    static constexpr B* write_tag(B* b)
    {
        static_assert(Id != 0);
    #ifdef _JKL_PB_USE_RUNTIME_TAG_CODEC
        return append_varint(b, f_tag);
    #else
        return append_varint_bytes<f_tag>(b);
    #endif
    }

    template<_byte_ B>
    static constexpr aresult<B const*> exam_tag(B const* beg, B const* end) noexcept
    {
        static_assert(Id != 0);
    #ifdef _JKL_PB_USE_RUNTIME_TAG_CODEC
        uint32_t tag;
        auto r = read_varint(beg, end, tag);
        if(BOOST_UNLIKELY(r && (tag != f_tag)))
            return pb_err::tag_mismatch;
        return r;
    #else
        if(auto* e = starts_with_varint_bytes<f_tag>(beg, end))
            return e;
        return pb_err::tag_mismatch;
    #endif
    }

    template<bool SkipTag, bool SkipLen>
    static constexpr pb_size_t len_dlm_wire_size(pb_size_t len) noexcept
    {
        static_assert(Wt == pb_wt_len_dlm);
        static_assert(SkipTag ||  !( SkipTag || SkipLen));

        pb_size_t s = 0;

        if constexpr(! SkipTag)
            s += f_tag_wire_size;

        if constexpr(! SkipLen)
            s += varint_wire_size(len);

        return s + len;
    }


    template<bool SkipLen = (Id == 0)>
    constexpr auto len_cache_cnt(auto const& /*d*/) const noexcept
    {
        return size_c<0>;
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr auto is_static_len(auto const& /*d*/) const noexcept
    {
        return false_c;
    }


    template<bool SkipTag = (Id == 0), bool SkipLen = (Id == 0), _byte_ B>
    constexpr B* write(B* b, auto const& d, pb_size_t const*& lc) const noexcept
    {
        return derived()->template write_impl<SkipTag, SkipLen>(b, d, lc);
    }

    template<bool SkipTag = (Id == 0), bool SkipLen = (Id == 0), _byte_ B>
    constexpr aresult<B const*> read(B const* beg, B const* end, auto& d) const
    {
        if constexpr(! SkipTag)
        {
            JKL_TRY(beg, exam_tag(beg, end));
        }

        if constexpr(Derived::has_validate)
        {
            JKL_TRY(beg, derived()->template read_impl<SkipLen>(beg, end, d));
            JKL_TRY(derived()->validate(d));
            return beg;
        }
        else
        {
            return derived()->template read_impl<SkipLen>(beg, end, d);
        }
    }


    template<bool SkipTag, bool SkipLen>
    constexpr void do_write(_resizable_byte_buf_ auto& b, auto const& d, pb_size_t* lenCache, size_t lenCacheCnt) const
    {
        pb_size_t* lc = lenCache;
        pb_size_t const total = derived()->template wire_size<SkipTag, SkipLen>(d, lc);
        BOOST_ASSERT(lc == lenCache + lenCacheCnt);

        resize_buf(b, total);
        pb_size_t const* clc = const_cast<pb_size_t const*>(lenCache);
        [[maybe_unused]] auto* e = write<SkipTag, SkipLen>(buf_data(b), d, clc);
        BOOST_ASSERT(e == buf_end(b));
        BOOST_ASSERT(clc == lenCache + lenCacheCnt);
    }

    template<size_t LenCacheInitSize = pb_default_len_cache_init_size,
             bool SkipTag = (Id == 0), bool SkipLen = (Id == 0), class D = null_op_t>
    constexpr void write(_resizable_byte_buf_ auto& b, D const& d = D{}) const
    {
        auto const lenCacheCnt = derived()->template len_cache_cnt<SkipLen>(d);

        if constexpr(is_integral_constant_v<decltype(lenCacheCnt)>)
        {
            pb_size_t lenCache[lenCacheCnt + 1];
            do_write<SkipTag, SkipLen>(b, d, lenCache, lenCacheCnt);
        }
        else
        {
            if(lenCacheCnt <= LenCacheInitSize)
            {
                pb_size_t lenCache[LenCacheInitSize];
                do_write<SkipTag, SkipLen>(b, d, lenCache, lenCacheCnt);
            }
            else
            {
                auto lenCache = std::make_unique<pb_size_t[]>(lenCacheCnt);
                do_write<SkipTag, SkipLen>(b, d, lenCache.get(), lenCacheCnt);
            }
        }
    }

    template<size_t LenCacheInitSize = pb_default_len_cache_init_size, class D = null_op_t>
    constexpr void write_len_prefixed(_resizable_byte_buf_ auto& b, D const& d = D{}) const
    {
        write<LenCacheInitSize, true, false>(b, d);
    }

    template<_byte_ B>
    constexpr aresult<B const*> read_len_prefixed(B const* beg, B const* end, auto& d) const
    {
        return read<true, false>(beg, end, d);
    }


    template<bool SkipTag = (Id == 0), bool SkipLen = (Id == 0)>
    constexpr aresult<> full_read(_byte_buf_ auto const& b, auto& d) const
    {
        JKL_TRY(auto e, read<SkipTag, SkipLen>(buf_begin(b), buf_end(b), d));
        if(e != buf_end(b))
            return pb_err::more_data_than_required;
        return no_err;
    }

    template<bool SkipTag = (Id == 0), bool SkipLen = (Id == 0)>
    constexpr aresult<> full_read(_byte_buf_ auto const& b) const
    {
        return full_read<SkipTag, SkipLen>(b, null_op);
    }

    constexpr aresult<> full_read_len_prefixed(_byte_buf_ auto const& b, auto& d) const
    {
        return full_read<true, false>(b, d);
    }

    constexpr aresult<> full_read_len_prefixed(_byte_buf_ auto const& b) const
    {
        return full_read_len_prefixed(b, null_op);
    }
};


// wire fmt: tag|varint
// expected value type: _arithmetic_
// available params: p_val, p_optional, p_has_val, p_clear_val, p_default, p_validate.
template<strlit Type, strlit Name, uint32_t Id, class Params>
struct pb_varint_fld : pb_fld<Type, Name, Id, pb_wt_varint, pb_varint_fld<Type, Name, Id, Params>, Params>
{
    using base = pb_fld<Type, Name, Id, pb_wt_varint, pb_varint_fld<Type, Name, Id, Params>, Params>;

    using Tar = std::conditional_t<
                    Type == "bool"  , bool    , std::conditional_t<
                    Type == "int32" , int32_t , std::conditional_t<
                    Type == "int64" , int64_t , std::conditional_t<
                    Type == "uint32", uint32_t, std::conditional_t<
                    Type == "uint64", uint64_t, std::conditional_t<
                    Type == "sint32", int32_t , std::conditional_t<
                    Type == "sint64", int64_t , void>>>>>>>;

    static_assert(! std::is_void_v<Tar>);

    static constexpr bool zigzag = (Type == "sint32" || Type == "sint64");
    
    static_assert(! Params::has_get_mem
               && ! Params::has_act_mem
               && ! Params::has_act_mem_idx);

    static constexpr bool is_optional = Params::is_optional;


    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr explicit pb_varint_fld(auto&& p)
        : base{JKL_FORWARD(p)}
    {}

    template<strlit RName, uint32_t RId>
    constexpr auto rebind_name_id() const noexcept
    {
        return pb_varint_fld<Type, RName, RId, Params>{*this};
    }

    template<strlit RName, uint32_t RId>
    constexpr auto rebind_name_id_u(auto&& v) const noexcept
    {
        return pb_varint_fld<Type, RName, RId, decltype(Params::rebind_uval(v))>{Params::rebind_uval(v)};
    }

    // rebind_name_id_accessor
    template<strlit RName, uint32_t RId>
    constexpr auto _(auto&&... p) const& noexcept
    {
        return pb_varint_fld<Type, RName, RId, decltype(make_pb_params(JKL_FORWARD(p)...))>{
                                                        make_pb_params(JKL_FORWARD(p)...)};
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr auto is_static_len(auto const& /*d*/) const noexcept
    {
        return bool_c<! is_optional && std::is_same_v<Tar, bool>>;
    }

    template<bool SkipTag = (Id == 0), bool SkipLen = (Id == 0)>
    constexpr auto wire_size(auto const& d, pb_size_t*&) const noexcept
    {
        static_assert(! (SkipTag && is_optional));

        constexpr pb_size_t tagSize = (SkipTag ? 0 : base::f_tag_wire_size);

        if constexpr(JKL_CEVL(is_static_len(d)))
        {
            return pb_size_c<tagSize + 1>;
        }
        else
        {
            if(Params::has_val(d))
                return tagSize + varint_wire_size<zigzag>(Params::const_val(d));
            return static_cast<pb_size_t>(0);
        }
    }

    template<bool SkipTag, bool SkipLen, _byte_ B>
    constexpr B* write_impl(B* b, auto const& d, pb_size_t const*&) const noexcept
    {
        static_assert(! SkipLen);
        static_assert(! (SkipTag && is_optional));

        if constexpr(! SkipTag)
        {
            if(Params::has_val(d))
                b = base::write_tag(b);
            else
                return b;
        }

        return append_varint<zigzag>(b, static_cast<Tar>(Params::const_val(d)));
    }

    template<bool SkipLen, _byte_ B>
    constexpr aresult<B const*> read_impl(B const* beg, B const* end, auto& d) const
    {
        static_assert(! SkipLen);

        Tar t;
        auto r = read_varint<zigzag>(beg, end, t);
        if(r)
            Params::mutable_val(d) = t;
        return r;
    }
};

template<strlit Type, strlit Name = "", uint32_t Id = 0>
constexpr auto pb_varint(auto&&... p) noexcept
{
    return pb_varint_fld<Type, Name, Id, decltype(make_pb_params(JKL_FORWARD(p)...))>{
                                                  make_pb_params(JKL_FORWARD(p)...)};
}

template<strlit Name = "", uint32_t Id = 0>
constexpr auto pb_bool(auto&&... p) noexcept
{
    return pb_varint<"bool", Name, Id>(JKL_FORWARD(p)...);
}

template<strlit Name = "", uint32_t Id = 0>
constexpr auto pb_int32(auto&&... p) noexcept
{
    return pb_varint<"int32", Name, Id>(JKL_FORWARD(p)...);
}

template<strlit Name = "", uint32_t Id = 0>
constexpr auto pb_int64(auto&&... p) noexcept
{
    return pb_varint<"int64", Name, Id>(JKL_FORWARD(p)...);
}

template<strlit Name = "", uint32_t Id = 0>
constexpr auto pb_uint32(auto&&... p) noexcept
{
    return pb_varint<"uint32", Name, Id>(JKL_FORWARD(p)...);
}

template<strlit Name = "", uint32_t Id = 0>
constexpr auto pb_uint64(auto&&... p) noexcept
{
    return pb_varint<"uint64", Name, Id>(JKL_FORWARD(p)...);
}

template<strlit Name = "", uint32_t Id = 0>
constexpr auto pb_sint32(auto&&... p) noexcept
{
    return pb_varint<"sint32", Name, Id>(JKL_FORWARD(p)...);
}

template<strlit Name = "", uint32_t Id = 0>
constexpr auto pb_sint64(auto&&... p) noexcept
{
    return pb_varint<"sint64", Name, Id>(JKL_FORWARD(p)...);
}


// wire fmt: tag | fixed size bytes in little-endian
// expected value type: _arithmetic_
// available params: p_val, p_optional, p_has_val, p_clear_val, p_default, p_validate.
template<strlit Type, strlit Name, uint32_t Id, class Params>
struct pb_fixed_fld : pb_fld<Type, Name, Id, (is_oneof(Type, "sfixed32", "fixed32", "float") ? pb_wt_fix32 : pb_wt_fix64),
                             pb_fixed_fld<Type, Name, Id, Params>, Params>
{
    using base = pb_fld<Type, Name, Id, (is_oneof(Type, "sfixed32", "fixed32", "float") ? pb_wt_fix32 : pb_wt_fix64),
                        pb_fixed_fld<Type, Name, Id, Params>, Params>;

    using Tar = std::conditional_t<
                    Type == "sfixed32", int32_t , std::conditional_t<
                    Type == "sfixed64", int64_t , std::conditional_t<
                    Type == "fixed32" , uint32_t, std::conditional_t<
                    Type == "fixed64" , uint64_t, std::conditional_t<
                    Type == "float"   , float   , std::conditional_t<
                    Type == "double"  , double  , void>>>>>>;

    static_assert(! std::is_void_v<Tar>);
    static_assert(sizeof(float ) == sizeof(uint32_t));
    static_assert(sizeof(double) == sizeof(uint64_t));

    static_assert(! Params::has_get_mem
               && ! Params::has_act_mem
               && ! Params::has_act_mem_idx);

    static constexpr bool is_optional = Params::is_optional;


    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr explicit pb_fixed_fld(auto&& p)
        : base{JKL_FORWARD(p)}
    {}

    template<strlit RName, uint32_t RId>
    constexpr auto rebind_name_id() const noexcept
    {
        return pb_fixed_fld<Type, RName, RId, Params>{*this};
    }

    template<strlit RName, uint32_t RId>
    constexpr auto rebind_name_id_u(auto&& v) const noexcept
    {
        return pb_fixed_fld<Type, RName, RId, decltype(Params::rebind_uval(v))>{Params::rebind_uval(v)};
    }

    // rebind_name_id_accessor
    template<strlit RName, uint32_t RId>
    constexpr auto _(auto&&... p) const& noexcept
    {
        return pb_fixed_fld<Type, RName, RId, decltype(make_pb_params(JKL_FORWARD(p)...))>{
                                                       make_pb_params(JKL_FORWARD(p)...)};
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr auto is_static_len(auto const& /*d*/) const noexcept
    {
        return bool_c<! is_optional>;
    }

    template<bool SkipTag = (Id == 0), bool SkipLen = (Id == 0)>
    constexpr auto wire_size(auto const& d, pb_size_t*&) const noexcept
    {
        static_assert(! (SkipTag && is_optional));

        constexpr pb_size_t tagSize = (SkipTag ? 0 : base::f_tag_wire_size);

        if constexpr(JKL_CEVL(is_static_len(d)))
        {
            return pb_size_c<tagSize + static_cast<pb_size_t>(sizeof(Tar))>;
        }
        else
        {
            if(Params::has_val(d))
                return tagSize + static_cast<pb_size_t>(sizeof(Tar));
            return static_cast<pb_size_t>(0);
        }
    }

    template<bool SkipTag, bool SkipLen, _byte_ B>
    constexpr B* write_impl(B* b, auto const& d, pb_size_t const*&) const noexcept
    {
        static_assert(! SkipLen);
        static_assert(! (SkipTag && is_optional));

        if constexpr(! SkipTag)
        {
            if(Params::has_val(d))
                b = base::write_tag(b);
            else
                return b;
        }

        store_le(b, static_cast<Tar>(Params::const_val(d)));
        return b + sizeof(Tar);
    }

    template<bool SkipLen, _byte_ B>
    constexpr aresult<B const*> read_impl(B const* beg, B const* end, auto& d) const
    {
        static_assert(! SkipLen);

        if(beg + sizeof(Tar) <= end)
        {
            Params::mutable_val(d) = load_le<Tar>(beg);
            return beg + sizeof(Tar);
        }

        return pb_err::fixed_incomplete;
    }
};

template<strlit Type, strlit Name = "", uint32_t Id = 0>
constexpr auto pb_fixed(auto&&... p) noexcept
{
    return pb_fixed_fld<Type, Name, Id, decltype(make_pb_params(JKL_FORWARD(p)...))>{
                                                 make_pb_params(JKL_FORWARD(p)...)};
}

template<strlit Name = "", uint32_t Id = 0>
constexpr auto pb_fixed32(auto&&... p) noexcept
{
    return pb_fixed<"fixed32", Name, Id>(JKL_FORWARD(p)...);
}

template<strlit Name = "", uint32_t Id = 0>
constexpr auto pb_fixed64(auto&&... p) noexcept
{
    return pb_fixed<"fixed64", Name, Id>(JKL_FORWARD(p)...);
}

template<strlit Name = "", uint32_t Id = 0>
constexpr auto pb_sfixed32(auto&&... p) noexcept
{
    return pb_fixed<"sfixed32", Name, Id>(JKL_FORWARD(p)...);
}

template<strlit Name = "", uint32_t Id = 0>
constexpr auto pb_sfixed64(auto&&... p) noexcept
{
    return pb_fixed<"sfixed64", Name, Id>(JKL_FORWARD(p)...);
}

template<strlit Name = "", uint32_t Id = 0>
constexpr auto pb_float(auto&&... p) noexcept
{
    return pb_fixed<"float", Name, Id>(JKL_FORWARD(p)...);
}

template<strlit Name = "", uint32_t Id = 0>
constexpr auto pb_double(auto&&... p) noexcept
{
    return pb_fixed<"double", Name, Id>(JKL_FORWARD(p)...);
}


// wire fmt: tag|len|bytes
// expected value type: _buf_, _str_
// available params: p_val, p_optional, p_has_val, p_clear_val, p_default, p_validate.
// NOTE: you can use p_optional without using a std::optional<...> value,
//       in that case, if value is fixed size(like array, read only range),
//       you should specify p_clear_val, and p_has_val(if buf_size()/str_size() are not proper).
template<strlit Type, strlit Name, uint32_t Id, class Params>
struct pb_bytes_fld : pb_fld<Type, Name, Id, pb_wt_len_dlm,
                             pb_bytes_fld<Type, Name, Id, Params>, Params>
{
    using base = pb_fld<Type, Name, Id, pb_wt_len_dlm,
                        pb_bytes_fld<Type, Name, Id, Params>, Params>;

    static_assert(! Params::has_get_mem
               && ! Params::has_act_mem
               && ! Params::has_act_mem_idx);

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr bool has_val(auto const& d) const noexcept
    {
        if constexpr(Params::is_optional_uval && _buf_<decltype(Params::uval(d))>)
            return buf_size(as_buf(Params::uval(d))) > 0;
        else
            return Params::has_val(d);
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr void clear_val(auto& d) const
    {
        if constexpr(Params::is_optional_uval && _resizable_buf_<decltype(Params::uval(d))>)
            clear_buf(Params::uval(d));
        else
            Params::clear_val(d);
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    auto const& const_val(auto const& d) const noexcept
    {
        if constexpr(Params::is_optional_uval && _buf_<decltype(Params::uval(d))>)
            return Params::uval(d);
        else
            return Params::const_val(d);
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    auto& mutable_val(auto& d) const noexcept
    {
        if constexpr(Params::is_optional_uval && _buf_<decltype(Params::uval(d))>)
            return Params::uval(d);
        else
            return Params::mutable_val(d);
    }


    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr explicit pb_bytes_fld(auto&& p)
        : base{JKL_FORWARD(p)}
    {}

    template<strlit RName, uint32_t RId>
    constexpr auto rebind_name_id() const noexcept
    {
        return pb_bytes_fld<Type, RName, RId, Params>{*this};
    }

    template<strlit RName, uint32_t RId>
    constexpr auto rebind_name_id_u(auto&& v) const noexcept
    {
        return pb_bytes_fld<Type, RName, RId, decltype(Params::rebind_uval(v))>{Params::rebind_uval(v)};
    }

    // rebind_name_id_accessor
    template<strlit RName, uint32_t RId>
    constexpr auto _(auto&&... p) const& noexcept
    {
        return pb_bytes_fld<Type, RName, RId, decltype(make_pb_params(JKL_FORWARD(p)...))>{
                                                       make_pb_params(JKL_FORWARD(p)...)};
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    static constexpr decltype(auto) as_buf(auto const& v) noexcept
    {
        if constexpr(Type == "string")
        {
            static_assert(_byte_str_<decltype(v)>);
            return as_str_class(v);
        }
        else
        {
            return v;
        }
    }

    template<bool SkipTag = (Id == 0), bool SkipLen = (Id == 0)>
    constexpr pb_size_t wire_size(auto const& d, pb_size_t*&) const noexcept
    {
        if(has_val(d))
        {
            auto const len = static_cast<pb_size_t>(buf_byte_size(as_buf(const_val(d))));
            return base::len_dlm_wire_size<SkipTag, SkipLen>(len);
        }

        return 0;
    }

    template<bool SkipTag, bool SkipLen, _byte_ B>
    constexpr B* write_impl(B* b, auto const& d, pb_size_t const*&) const noexcept
    {
        static_assert(! SkipTag && ! SkipLen);

        if(has_val(d))
        {
            b = base::write_tag(b);

            decltype(auto) v = as_buf(const_val(d));

            auto len = static_cast<pb_size_t>(buf_byte_size(v));

            b = append_varint(b, len);

            memcpy(b, buf_data(v), len);

            return b + len;
        }

        return b;
    }

    template<bool SkipLen, _byte_ B>
    constexpr aresult<B const*> read_impl(B const* beg, B const* end, auto& d) const
    {
        static_assert(! SkipLen);

        pb_size_t len = 0;
        JKL_TRY(beg, read_varint(beg, end, len));

        auto& v = mutable_val(d);

        if constexpr(_resizable_buf_<decltype(v)>)
        {
            constexpr auto elmSize = sizeof(*buf_data(v));

            if(BOOST_UNLIKELY(len % elmSize != 0))
                return pb_err::invalid_length;

            resize_buf(v, len / elmSize);
        }
        else
        {
            if(BOOST_UNLIKELY(len != buf_byte_size(v)))
                return pb_err::invalid_length;
        }

        memcpy(buf_data(v), beg, len);

        return beg + len;
    }
};

template<strlit Name = "", uint32_t Id = 0>
constexpr auto pb_bytes(auto&&... p) noexcept
{
    return pb_bytes_fld<"bytes", Name, Id, decltype(make_pb_params(JKL_FORWARD(p)...))>{
                                                    make_pb_params(JKL_FORWARD(p)...)};
}

template<strlit Name = "", uint32_t Id = 0>
constexpr auto pb_string(auto&&... p) noexcept
{
    return pb_bytes_fld<"string", Name, Id, decltype(make_pb_params(JKL_FORWARD(p)...))>{
                                                     make_pb_params(JKL_FORWARD(p)...)};
}


// wire fmt:
//      for scalar fld, always uses packed fmt: tag(fld.id, pb_wt_len_dlm) | len | encoded_scalars...
//      for other repeated fld: fld | fld | ... | fld
// expected value type: _range_
// available params: p_val, p_clear_val, p_validate.
// NOTE: repeated cannot be optional or required;
//       it uses fld's name and id.
template<class Fld, class Params>
struct pb_repeated_fld : pb_fld<"repeated " + Fld::f_type, Fld::f_name, Fld::f_id, pb_wt_len_dlm,
                                 pb_repeated_fld<Fld, Params>, Params>
{
    using base = pb_fld<"repeated " + Fld::f_type, Fld::f_name, Fld::f_id, pb_wt_len_dlm,
                        pb_repeated_fld<Fld, Params>, Params>;

    static constexpr auto fld_type      = Fld::f_type;
    static constexpr auto fld_wire_type = Fld::f_wire_type;
    static constexpr bool is_packed     = is_oneof(fld_wire_type, pb_wt_fix32, pb_wt_fix64, pb_wt_varint);

    static_assert(! Fld::is_optional
               && ! fld_type.starts_with("repeated ")
               && ! fld_type.starts_with("oneof ")
               && ! fld_type.starts_with("map<"));

    static_assert(! Params::is_optional_uval
               && ! Params::has_has_val
               && ! Params::has_default
               && ! Params::has_get_mem
               && ! Params::has_act_mem
               && ! Params::has_act_mem_idx);

    static constexpr bool is_optional = true;

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr void clear_val(auto& d) const
    {
        if constexpr(Params::has_clear_val) // for value like fixed range, user should provide p_clear_val for read
            Params::_clearVal(d);
        else if constexpr(__has_clear<decltype(Params::uval(d))>)
            Params::uval(d).clear();
        else
            JKL_DEPENDENT_FAIL(decltype(d), "unsupported type");
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    static constexpr auto same_as_wire_layout(auto const& u) noexcept
    {
        using elem_t = JKL_DECL_NO_CVREF_T(*std::begin(u));

        return bool_c<
                (bendian::order::native == bendian::order::little)
                && _buf_<decltype(u)> && _arithmetic_<elem_t>
                && ((sizeof(elem_t) == 4 && fld_wire_type == pb_wt_fix32)
                    || (sizeof(elem_t) == 8 && fld_wire_type == pb_wt_fix64)
                    || (sizeof(elem_t) == 1 && fld_type == "bool"))
        >;
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr auto is_static_len_fld(auto const& u) const noexcept
    {
        return _fld.is_static_len(tr_get<0>(u));
    }

    template<bool SkipTag>
    constexpr auto static_len_fld_wire_size(auto const& u) const noexcept
    {
        static_assert(JKL_CEVL(is_static_len_fld(u)));
        pb_size_t* lc = nullptr;
        return _fld.template wire_size<SkipTag, false>(tr_get<0>(u), lc);
    }

    Fld _fld;

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr explicit pb_repeated_fld(auto&& fld, auto&& p)
        : base{JKL_FORWARD(p)}, _fld{JKL_FORWARD(fld)}
    {}

    constexpr auto const& elem_fld() const noexcept { return _fld; }

    template<strlit RName, uint32_t RId>
    constexpr auto rebind_name_id() const noexcept
    {
        return pb_repeated_fld<decltype(_fld.template rebind_name_id<RName, RId>()), Params>{
                                        _fld.template rebind_name_id<RName, RId>(),  *this};
    }

    template<strlit RName, uint32_t RId>
    constexpr auto rebind_name_id_u(auto&& v) const noexcept
    {
        return pb_repeated_fld<decltype(_fld.template rebind_name_id<RName, RId>()), decltype(Params::rebind_uval(v))>{
                                        _fld.template rebind_name_id<RName, RId>(),           Params::rebind_uval(v)};
    }

    // rebind_name_id_accessor
    template<strlit RName, uint32_t RId>
    constexpr auto _(auto&&... p) const& noexcept
    {
        return pb_repeated_fld<decltype(_fld.template rebind_name_id<RName, RId>()), decltype(make_pb_params(JKL_FORWARD(p)...))>{
                                        _fld.template rebind_name_id<RName, RId>(),           make_pb_params(JKL_FORWARD(p)...)};
    }

    template<_resizable_byte_buf_ S = string>
    constexpr auto fld_def() const
    {
        static_assert(base::f_id != 0);
        return cat_as_str<S>(base::f_type, " ", base::f_name, "=", base::f_id, (is_packed ? " [packed=true];" : ";"));
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr auto is_static_len(auto const& d) const noexcept
    {
        auto const& u = Params::uval(d);
        return bool_c<_array_<decltype(u)> || _std_array_<decltype(u)>> && is_static_len_fld(u);
    }

    template<bool SkipLen = (base::f_id == 0)>
    constexpr auto len_cache_cnt(auto const& d) const noexcept
    {
        static_assert(is_packed || ! SkipLen);

        auto const& u = Params::uval(d);

        if constexpr(JKL_CEVL(is_static_len_fld(u)))
        {
            return size_c<0>;
        }
        else if constexpr(is_packed)
        {
            return size_c<SkipLen ? 0 : 1>;
        }
        else
        {
            size_t n = 0;
            for(auto& e : u)
                n += _fld.template len_cache_cnt<false>(e);
            return n;
        }
    }

    template<bool SkipTag = (base::f_id == 0), bool SkipLen = (base::f_id == 0)>
    constexpr auto wire_size(auto const& d, pb_size_t*& lc) const noexcept
    {
        static_assert((is_packed && SkipTag) || !( SkipTag || SkipLen));

        auto const& u = Params::uval(d);

        if constexpr(JKL_CEVL(is_static_len(d)))
        {
            constexpr pb_size_t len = JKL_CEVL(tuple_size<JKL_DECL_NO_CVREF_T(u)>{}
                                                * static_len_fld_wire_size<is_packed>(u));
            if constexpr(! is_packed)
                return pb_size_c<len>;
            else
                return pb_size_c<(base::len_dlm_wire_size<SkipTag, SkipLen>(len))>;
        }
        else
        {
            pb_size_t len = 0;

            if constexpr(JKL_CEVL(is_static_len_fld(u)))
            {
                len = static_cast<pb_size_t>(std::size(u) * JKL_CEVL(static_len_fld_wire_size<is_packed>(u)));
            }
            else
            {
                [[maybe_unused]]pb_size_t* lp = lc;
                if constexpr(! SkipLen && is_packed)
                    ++lc;

                for(auto& e : u)
                    len += _fld.template wire_size<is_packed, false>(e, lc);

                BOOST_ASSERT(len > 0);
                if constexpr(! SkipLen && is_packed)
                    *lp = len;
            }

            if constexpr(! is_packed)
                return len;
            else
                return base::len_dlm_wire_size<SkipTag, SkipLen>(len);
        }
    }

    template<bool SkipTag, bool SkipLen, _byte_ B>
    constexpr B* write_impl(B* b, auto const& d, pb_size_t const*& lc) const noexcept
    {
        static_assert(! SkipTag && ! SkipLen);

        auto const& u = Params::uval(d);

        [[maybe_unused]] pb_size_t packedLen = 0;

        if constexpr(is_packed)
        {
            b = base::write_tag(b);

            if constexpr(JKL_CEVL(is_static_len_fld(u)))
            {
                pb_size_t* mlc = nullptr;
                packedLen = wire_size<true, true>(d, mlc);
            }
            else
            {
                packedLen = *lc++;
            }

            b = append_varint(b, packedLen);
        }

        if constexpr(JKL_CEVL(same_as_wire_layout(u)))
        {
            BOOST_ASSERT(buf_byte_size(u) == packedLen);
            memcpy(b, buf_data(u), packedLen);
            return b + packedLen;
        }
        else
        {
            for(auto& e : u)
                b = _fld.template write_impl<is_packed, false>(b, e, lc);
            return b;
        }
    }

    template<bool SkipLen, _byte_ B>
    constexpr aresult<B const*> read_impl(B const* beg, B const* end, auto& d) const
    {
        static_assert(! SkipLen);

        auto& u = Params::uval(d);

        [[maybe_unused]] pb_size_t packedLen = 0;
        [[maybe_unused]] size_t staticLenFldCnt = 0;

        if constexpr(is_packed)
        {
            JKL_TRY(beg, read_varint(beg, end, packedLen));

            if(BOOST_UNLIKELY(beg + packedLen > end))
                return pb_err::repeated_incomplete;

            if constexpr(JKL_CEVL(is_static_len_fld(u)))
            {
                constexpr auto staticFldSize = JKL_CEVL(static_len_fld_wire_size<is_packed>(u));
                if(BOOST_UNLIKELY(packedLen % staticFldSize))
                    return pb_err::invalid_length;
                staticLenFldCnt = packedLen / staticFldSize;
            }

            end = beg + packedLen;
        }

        using elem_t = JKL_DECL_NO_CVREF_T(*std::begin(u));

        if constexpr(! (is_packed && JKL_CEVL(is_static_len_fld(u)))
                     || __has_map_insert_or_assign<decltype(u), elem_t>
                     || __has_set_insert<decltype(u), elem_t>)
        {
            if constexpr(__has_clear<decltype(u)>)
            {
                u.clear();

                while(beg < end)
                {
                    if constexpr(! is_packed)
                    {
                        if(u.size()) // skip first tag, as it has been checked
                        {
                            if(auto r = _fld.exam_tag(beg, end))
                                beg = r.value();
                            else
                                break;
                        }
                    }

                    if constexpr(__has_map_insert_or_assign<decltype(u), elem_t>)
                    {
                        mp_transform<std::remove_const_t, elem_t> e;
                        JKL_TRY(beg, _fld.template read_impl<false>(beg, end, e));
                        u.insert_or_assign(std::move(std::get<0>(e)), std::move(std::get<1>(e)));
                    }
                    else
                    {
                        elem_t e;
                        JKL_TRY(beg, _fld.template read_impl<false>(beg, end, e));

                        if constexpr(__has_set_insert<decltype(u), elem_t>)
                            u.insert(std::move(e));
                        else
                            u.emplace_back(std::move(e));
                    }
                }

                if constexpr(is_packed)
                {
                    if(BOOST_UNLIKELY(beg != end))
                        return pb_err::invalid_length;

                    if constexpr(JKL_CEVL(is_static_len_fld(u)))
                    {
                        if(BOOST_UNLIKELY(std::size(u) != staticLenFldCnt))
                            return pb_err::invalid_length;
                    }
                }
            }
            else // u is a fixed size range
            {
                if constexpr(is_packed && JKL_CEVL(is_static_len_fld(u)))
                {
                    if(BOOST_UNLIKELY(std::size(u) != staticLenFldCnt))
                        return pb_err::invalid_length;
                }

                [[maybe_unused]] bool not1st = false;

                for(auto& e : u)
                {
                    if constexpr(! is_packed)
                    {
                        if(not1st) // skip first tag, as it has been checked
                        {
                            JKL_TRY(beg, _fld.exam_tag(beg, end));
                        }
                        else
                        {
                            not1st = true;
                        }
                    }

                    JKL_TRY(beg, _fld.template read_impl<false>(beg, end, e));
                }

                if constexpr(is_packed && ! JKL_CEVL(is_static_len_fld(u)))
                {
                    if(BOOST_UNLIKELY(beg != end))
                        return pb_err::invalid_length;
                }
            }
        }
        else // is_packed && is_static_len_fld(u) && u is not a map or set
        {
            if constexpr(_resizable_buf_<decltype(u)>)
            {
                resize_buf(u, staticLenFldCnt);
            }
            else if constexpr(__has_resize<decltype(u)>)
            {
                u.resize(staticLenFldCnt);
            }
            else // u is a fixed size range
            {
                if(BOOST_UNLIKELY(staticLenFldCnt != std::size(u)))
                    return pb_err::invalid_length;
            }

            if constexpr(JKL_CEVL(same_as_wire_layout(u)))
            {
                memcpy(buf_data(u), beg, packedLen);
                beg = end;
            }
            else
            {
                for(auto& e : u)
                {
                    JKL_TRY(beg, _fld.template read_impl<false>(beg, end, e));
                }
            }
        }

        if constexpr(is_packed)
        {
            BOOST_ASSERT(beg == end);
        }

        return beg;
    }
};

// NOTE: fld's accessor will be applied to the repeated element
_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
constexpr auto pb_repeated(auto&& fld, auto&&... p) noexcept
{
    return pb_repeated_fld<JKL_DECL_NO_CVREF_T(fld), decltype(make_pb_params(JKL_FORWARD(p)...))>{
                                   JKL_FORWARD(fld),          make_pb_params(JKL_FORWARD(p)...)};
}


// wire fmt: tag|len|sub_flds...
// expected value type: struct/class
// available params: p_val, p_optional, p_has_val, p_clear_val, p_default, p_validate.
template<strlit Type, strlit Name, uint32_t Id, class Params, class... Flds>
struct pb_message_fld : pb_fld<Type, Name, Id, pb_wt_len_dlm,
                               pb_message_fld<Type, Name, Id, Params, Flds...>, Params>
{
    using base = pb_fld<Type, Name, Id, pb_wt_len_dlm,
                        pb_message_fld<Type, Name, Id, Params, Flds...>, Params>;

    static_assert(! Params::has_get_mem
               && ! Params::has_act_mem
               && ! Params::has_act_mem_idx);

    static constexpr bool is_optional = Params::is_optional;

    using expanded_fld_list = mp_append<typename Flds::injected_fld_list...>;
    static constexpr auto expended_fld_list_cnt = mp_size<expanded_fld_list>::value;

    // make sure there is no duplicated fld name and id
    template<class Fld1, class Fld2>
    using equal_name_or_id = mp_bool<(Fld1::f_id == Fld2::f_id) || (Fld1::f_name == Fld2::f_name)>;
    static_assert(expended_fld_list_cnt == mp_size<mp_unique_if<expanded_fld_list, equal_name_or_id>>::value);

    static_assert(! is_pb_reserved_word(Type));

    std::tuple<Flds...> _flds;

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr explicit pb_message_fld(auto&& p, auto&&... flds)
        : base{JKL_FORWARD(p)}, _flds{JKL_FORWARD(flds)...}
    {}

    constexpr auto const& sub_flds() const noexcept { return _flds; }
    template<size_t I>
    constexpr auto const& sub_fld() const noexcept { return std::get<I>(_flds); }

    template<strlit RName, uint32_t RId>
    constexpr auto rebind_name_id() const noexcept
    {
        return pb_message_fld<Type, RName, RId, Params, Flds...>{*this, _flds};
    }

    template<strlit RName, uint32_t RId>
    constexpr auto rebind_name_id_u(auto&& v) const noexcept
    {
        return pb_message_fld<Type, RName, RId, decltype(Params::rebind_uval(v)), Flds...>{
                                                         Params::rebind_uval(v),  _flds};
    }

    // rebind_name_id_accessor
    template<strlit RName, uint32_t RId>
    constexpr auto _(auto&&... p) const& noexcept
    {
        return pb_message_fld<Type, RName, RId, decltype(make_pb_params(JKL_FORWARD(p)...)), Flds...>{
                                                         make_pb_params(JKL_FORWARD(p)...),  _flds};
    }

    template<_resizable_byte_buf_ S = string>
    constexpr S msg_def() const
    {
        S def;

        append_str(def, "message ", Type, "{\n");

        std::apply(
            [&def](auto&... fld)
            {
                (... , append_str(def, "    ", fld.fld_def<S>(), "\n"));
            },
            _flds);

        append_str(def, "}");

        return def;
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr auto is_static_len([[maybe_unused]] auto const& d) const noexcept
    {
        if constexpr(! is_optional)
        {
            return std::apply([&v = Params::uval(d)](auto&... fld){
                return (... && fld.is_static_len(v));
            }, _flds);
        }
        else
        {
            return false_c;
        }
    }

    template<bool SkipLen = (Id == 0)>
    constexpr auto len_cache_cnt(auto const& d) const noexcept
    {
        if constexpr(JKL_CEVL(is_static_len(d)))
        {
            return size_c<0>;
        }
        else
        {
            if(Params::has_val(d))
            {
                return (SkipLen ? 0 : 1)
                    + std::apply([&v = Params::const_val(d)](auto&... fld){
                          static_assert(sizeof...(fld));
                          return (... + fld.template len_cache_cnt<false>(v));
                      }, _flds);
            }

            return static_cast<size_t>(0);
        }
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr auto sub_flds_len(auto const& d, pb_size_t*& lc) const noexcept
    {
        return std::apply([&lc, &v = Params::const_val(d)](auto&... fld){
            static_assert(sizeof...(fld));
            return (... + fld.template wire_size<false, false>(v, lc));
        }, _flds);
    }

    template<bool SkipTag = (Id == 0), bool SkipLen = (Id == 0)>
    constexpr auto wire_size(auto const& d, pb_size_t*& lc) const noexcept
    {
        static_assert(! (SkipTag && is_optional));

        if constexpr(JKL_CEVL(is_static_len(d)))
        {
            return pb_size_c<(base::len_dlm_wire_size<SkipTag, SkipLen>(JKL_CEVL(sub_flds_len(d, lc))))>;
        }
        else
        {
            if(Params::has_val(d))
            {
                if constexpr(SkipLen)
                {
                    return base::len_dlm_wire_size<SkipTag, SkipLen>(sub_flds_len(d, lc));
                }
                else
                {
                    pb_size_t* lp = lc++;
                    pb_size_t const len = sub_flds_len(d, lc);
                    *lp = len;
                    return base::len_dlm_wire_size<SkipTag, SkipLen>(len);
                }
            }

            return static_cast<pb_size_t>(0);
        }
    }

    template<bool SkipTag, bool SkipLen, _byte_ B>
    constexpr B* write_impl(B* b, auto const& d, pb_size_t const*& lc) const noexcept
    {
        static_assert(SkipTag ||  !( SkipTag || SkipLen));
        static_assert(! (SkipTag && is_optional));

        if constexpr(! SkipTag)
        {
            if(Params::has_val(d))
                b = base::write_tag(b);
            else
                return b;
        }

        if constexpr(! SkipLen)
        {
            if constexpr(JKL_CEVL(is_static_len(d)))
                b = append_varint(b, JKL_CEVL(sub_flds_len(d, std::declval<pb_size_t*&>())));
            else
                b = append_varint(b, *lc++);
        }

        std::apply([&, &v = Params::const_val(d)](auto&... fld) {
                (... , (b = fld.template write_impl<false, false>(b, v, lc)));
            }, _flds);

        return b;
    }

    template<bool SkipLen, _byte_ B>
    constexpr aresult<B const*> read_impl(B const* beg, B const* end, auto& d) const
    {
        if constexpr(! SkipLen)
        {
            pb_size_t len;
            JKL_TRY(beg, read_varint(beg, end, len)); // len

            if(BOOST_UNLIKELY(beg + len > end))
                return pb_err::msg_incomplete;

            end = beg + len;
        }

        auto& v = Params::mutable_val(d);
        std::bitset<sizeof...(Flds)> valBits;

        while(beg < end)
        {
            uint32_t tag;
            JKL_TRY(beg, read_varint(beg, end, tag));

            if(auto r = read_sub_flds(beg, end, tag, v, valBits))
            {
                beg = r.value();
            }
            else if(r.error() == pb_err::tag_mismatch)
            {
                // no matching tag, skip this field
                switch(tag & 0x7)
                {
                    case pb_wt_varint : { JKL_TRY(beg, skip_varint(beg, end   )); break; }
                    case pb_wt_fix32  : { JKL_TRY(beg, skip_bytes (beg, end, 4)); break; }
                    case pb_wt_fix64  : { JKL_TRY(beg, skip_bytes (beg, end, 8)); break; }
                    case pb_wt_len_dlm:
                        {
                            pb_size_t len;
                            JKL_TRY(beg, read_varint(beg, end, len));
                            JKL_TRY(beg, skip_bytes(beg, end, len));
                            break;
                        }
                    default:
                        BOOST_ASSERT(false);
                }
            }
            else
            {
                return r;
            }
        }

        if constexpr(! SkipLen)
        {
            BOOST_ASSERT(beg == end);
        }

        JKL_TRY(exam_sub_flds(v, valBits));

        return beg;
    }

    template<_byte_ B>
    static constexpr aresult<B const*> skip_bytes(B const* beg, B const* end, pb_size_t n) noexcept
    {
        beg += n;
        if(beg <= end)
            return beg;
        return pb_err::invalid_length;
    }

    template<size_t I>
    static consteval uint32_t expended_fld_case_tag()
    {
        if constexpr(I < expended_fld_list_cnt)
            return mp_at_c<expanded_fld_list, I>::f_tag;
        else
            return 0xff'ffff + I - expended_fld_list_cnt;
    }

    // expended_val_bits_map is to map from expanded flds to valBits, while valBits is for Flds,

    template<class V, class Fld>
    using gen_val_bits_map = mp_list<
                                 mp_append<
                                     mp_first<V>,
                                     mp_repeat<
                                         mp_list<mp_second<V>>,
                                         mp_size<typename Fld::injected_fld_list>>>,
                                 mp_plus<mp_second<V>, mp_size_t<1>>>;

    using expended_val_bits_map = mp_first<
                                      mp_fold<
                                          mp_list<Flds...>,
                                          mp_list<mp_list<>, mp_size_t<0>>,
                                          gen_val_bits_map>>;

    static_assert(expended_fld_list_cnt == mp_size<expended_val_bits_map>::value);

    // each element contains 2 idx, the first is Flds idx, second is injected fld idx.

    template<class V1>
    struct combine_with
    {
        template<class V2>
        using fn = mp_list<V1, V2>;
    };

    template<class V, class Fld>
    using gen_expanded_fld_indice = mp_list<
                                        mp_append<
                                            mp_first<V>,
                                            mp_transform_q<
                                                combine_with<mp_second<V>>,
                                                mp_iota<mp_size<typename Fld::injected_fld_list>>>>,
                                        mp_plus<mp_second<V>, mp_size_t<1>>>;

    using expanded_fld_indice =  mp_first<
                                     mp_fold<
                                         mp_list<Flds...>,
                                         mp_list<mp_list<>, mp_size_t<0>>,
                                         gen_expanded_fld_indice>>;

    static_assert(expended_fld_list_cnt == mp_size<expanded_fld_indice>::value);


    template<size_t I = 0, _byte_ B>
    BOOST_FORCEINLINE
    constexpr aresult<B const*> read_sub_flds(B const* beg, B const* end, uint32_t tag, auto& v, auto& valBits) const
    {
    #define _READ_SUB_FLD_CASE_CNT 32
    #define _READ_SUB_FLD_CASE(Z, N, D) \
        case expended_fld_case_tag<I + N>():                                   \
        {                                                                      \
            if constexpr(I + N < expended_fld_list_cnt)                        \
            {                                                                  \
                using indice = mp_at_c<expanded_fld_indice, I + N>;            \
                                                                               \
                auto r = std::get<mp_first<indice>::value>(_flds).template     \
                    injected_fld_read<mp_second<indice>::value>(beg, end, v);  \
                                                                               \
                if(r)                                                          \
                    valBits.set(mp_at_c<expended_val_bits_map, I + N>::value); \
                return r;                                                      \
            }                                                                  \
            else                                                               \
            {                                                                  \
                BOOST_UNREACHABLE_RETURN(pb_err::tag_mismatch);                \
            }                                                                  \
        }                                                                      \
        /**/

        switch(tag)
        {
            BOOST_PP_REPEAT(
                _READ_SUB_FLD_CASE_CNT,
                _READ_SUB_FLD_CASE, _)
        }

        constexpr auto next_idx = std::min(I + _READ_SUB_FLD_CASE_CNT, expended_fld_list_cnt);

        //if constexpr(next_idx < expended_fld_list_cnt) causes some weird msvc bug
        if constexpr(next_idx + 0 < expended_fld_list_cnt)
        {
            return read_sub_flds<next_idx>(beg, end, tag, v, valBits);
        }

        return pb_err::tag_mismatch;

    #undef _READ_SUB_FLD_CASE
    #undef _READ_SUB_FLD_CASE_CNT
    }

    template<size_t I = 0>
    BOOST_FORCEINLINE constexpr aresult<> exam_sub_flds(auto& v, auto& valBits) const
    {
    #define _EXAM_SUB_FLD_CASE_CNT 32
    #define _EXAM_SUB_FLD_CASE(Z, N, D) \
        if constexpr(I + N < sizeof...(Flds))              \
        {                                                  \
            auto& fld = std::get<I + N>(_flds);            \
                                                           \
            if(valBits.test(I + N))                        \
            {                                              \
                if constexpr(fld.has_validate)             \
                {                                          \
                    auto r = fld.validate(v);              \
                    if(BOOST_UNLIKELY(! r))                \
                        return r;                          \
                }                                          \
            }                                              \
            else                                           \
            {                                              \
                if constexpr(fld.is_optional)              \
                    fld.clear_val(v);                      \
                else                                       \
                    return pb_err::required_field_missing; \
            }                                              \
        }                                                  \
        /**/

        BOOST_PP_REPEAT(
            _EXAM_SUB_FLD_CASE_CNT,
            _EXAM_SUB_FLD_CASE, _)

        constexpr auto next_idx = std::min(I + _EXAM_SUB_FLD_CASE_CNT, sizeof...(Flds));

        //if constexpr(next_idx < sizeof...(Flds)) causes some weird msvc bug
        if constexpr(next_idx + 0 < sizeof...(Flds))
        {
            return exam_sub_flds<next_idx>(v, valBits);
        }

        return no_err;

    #undef _EXAM_SUB_FLD_CASE
    #undef _EXAM_SUB_FLD_CASE_CNT
    }
};

template<strlit Type>
constexpr auto pb_message(auto&&... flds) noexcept
{
    return pb_message_fld<Type, "", 0, pb_null_params, JKL_DECL_NO_CVREF_T(flds)...>{
                                       pb_null_params{},       JKL_FORWARD(flds)...};
}



// map<key_type, value_type> fld = N; // is same as:
//
// message map_entry {
//     optional key_type key = 1;
//     optional value_type value = 2;
// }
// repeated map_entry fld = N;
//
// NOTE: map cannot be repeated, optional, or required
//       key_type can be any integral or string type (so, any scalar type except for floating point types and bytes).
//       value_type can be any type except another map.
template<strlit Name, uint32_t Id, class KeyFld, class ValueFld, class Params>
struct pb_map_fld : pb_repeated_fld<pb_message_fld<Name + "_entry", Name, Id, pb_null_params, KeyFld, ValueFld>, Params>
{
    using map_entry = pb_message_fld<Name + "_entry", Name, Id, pb_null_params, KeyFld, ValueFld>;
    using base      = pb_repeated_fld<map_entry, Params>;

    static constexpr auto f_type = "map<" + KeyFld::f_type + ", " + ValueFld::f_type + ">";

    static_assert(KeyFld::f_id == 1 && ValueFld::f_id == 2);
    static_assert(is_oneof(KeyFld::f_type, "int32", "int64", "sint32", "sint64", "uint32", "uint64", "bool",
                                           "fixed32", "fixed64", "sfixed32", "sfixed64", "string"));
    static_assert(! ValueFld::f_type.starts_with("repeated ")
               && ! ValueFld::f_type.starts_with("oneof ")
               && ! ValueFld::f_type.starts_with("map<"));


    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr explicit pb_map_fld(auto&& k, auto&& v, auto&& p)
        : base{map_entry{pb_null_params{}, JKL_FORWARD(k), JKL_FORWARD(v)}, JKL_FORWARD(p)}
    {}

    constexpr auto const&   key_field() const noexcept { return base::elem_fld().sub_fld<0>(); }
    constexpr auto const& value_field() const noexcept { return base::elem_fld().sub_fld<1>(); }

    template<strlit RName, uint32_t RId>
    constexpr auto rebind_name_id() const noexcept
    {
        return pb_map_fld<RName, RId, KeyFld,      ValueFld, Params>{
                                 key_field(), value_field(), *this};
    }

    template<strlit RName, uint32_t RId>
    constexpr auto rebind_name_id_u(auto&& v) const noexcept
    {
        return pb_map_fld<RName, RId, KeyFld,      ValueFld, decltype(Params::rebind_uval(v))>{
                                 key_field(), value_field(),          Params::rebind_uval(v)};
    }

    // rebind_name_id_accessor
    template<strlit RName, uint32_t RId>
    constexpr auto _(auto&&... p) const& noexcept
    {
        return pb_map_fld<RName, RId, KeyFld,      ValueFld, decltype(make_pb_params(JKL_FORWARD(p)...))>{
                                 key_field(), value_field(),          make_pb_params(JKL_FORWARD(p)...)};
    }

    template<_resizable_byte_buf_ S = string>
    constexpr S fld_def() const
    {
        static_assert(Id != 0);
        return cat_as_str<S>(f_type, " ", Name, "=", Id, ";");
    }
};

// NOTE: key and value flds' accessors are rebound
template<strlit Name = "", uint32_t Id = 0>
constexpr auto pb_map(auto&& key, auto&& value, auto&&... p) noexcept
{
    auto k = key  .template rebind_name_id_u<"key"  , 1>([](auto& d) -> auto& { return std::get<0>(d); });
    auto v = value.template rebind_name_id_u<"value", 2>([](auto& d) -> auto& { return std::get<1>(d); });

    return pb_map_fld<Name, Id, decltype(k),  decltype(v), decltype(make_pb_params(JKL_FORWARD(p)...))>{
                               std::move(k), std::move(v),          make_pb_params(JKL_FORWARD(p)...)};
}


// wire fmt: one of field's wire fmt
// expected value type: std::variant or any type with p_get_member, p_activate_member, p_active_member_idx.
// available params: p_val, p_get_member, p_activate_member, p_active_member_idx, p_validate.
//      NOTE: p_validate on sub fields won't be used, you should validate oneof field itself.
// oneof is like anonymous union(but with name),
// name and id of its fields should be unique in the enclosing message.
// fields cannot be required, optional, or repeated, in such case you can wrap these fields in a message.
// oneof can only present in message.
// the default target type is std::variant<std::monostate, T1, T2, ...>
// custom types(e.g. union) can be supported with p_get_member, p_activate_member and p_active_member_idx
template<strlit Name, class Params, class... Flds>
struct pb_oneof_fld : pb_fld<"oneof " + Name, "", 0, pb_wt_len_dlm,
                             pb_oneof_fld<Name, Params, Flds...>, Params>
{
    using base = pb_fld<"oneof " + Name, "", 0, pb_wt_len_dlm,
                        pb_oneof_fld<Name, Params, Flds...>, Params>;

    static_assert(! Params::is_optional_uval
               && ! Params::has_has_val
               && ! Params::has_default
               && ! Params::has_clear_val);

    static constexpr bool is_optional = true;

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr void clear_val(auto& d) const
    {
        Params::template activate_member<0>(d);
    }

    using injected_fld_list = mp_list<Flds...>;

    template<size_t I, _byte_ B>
    constexpr aresult<B const*> injected_fld_read(B const* beg, B const* end, auto& d) const noexcept
    {
        Params::activate_member<I + 1>(d);
        return std::get<I>(_flds).template read_impl<false>(beg, end, Params::get_member<I + 1>(d));
    }

    // make sure all fields are valid for oneof
    template<class Fld>
    using is_valid_field = mp_bool<! Fld::is_optional && ! Fld::f_name.empty() && Fld::f_id != 0
                                && ! Fld::f_type.starts_with("repeated ")
                                && ! Fld::f_type.starts_with("oneof ")
                                && ! Fld::f_type.starts_with("map<")>;
    static_assert(mp_all_of<injected_fld_list, is_valid_field>::value);


    std::tuple<Flds...> _flds;

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    constexpr explicit pb_oneof_fld(auto&& p, auto&&... flds)
        : base{JKL_FORWARD(p)}, _flds{JKL_FORWARD(flds)...}
    {}

    template<_resizable_byte_buf_ S = string>
    constexpr S fld_def() const
    {
        S def;

        append_str(def, "oneof ", Name, "{\n");

        std::apply(
            [&def](auto&... fld)
            {
                (... , append_as_str(def, "        ", fld.f_type, " ", fld.f_name, "=", fld.f_id, "\n"));
            },
            _flds);

        append_str(def, "    }");

        return def;
    }

    template<size_t FldIdx = 0>
    BOOST_FORCEINLINE constexpr decltype(auto) visit(size_t actIdx, auto& d, auto&& f) const
    {
        BOOST_ASSERT(actIdx > 0);

        if(BOOST_UNLIKELY(actIdx > sizeof...(Flds) + 1))
            throw std::bad_variant_access{};

    #define _VISIT_CASE_CNT 32
    #define _VISIT_CASE(Z, N, D) \
        case FldIdx + N + 1:                                                                  \
        {                                                                                     \
            if constexpr(FldIdx + N < sizeof...(Flds))                                        \
            {                                                                                 \
                return f(std::get<FldIdx + N>(_flds), Params::get_member<FldIdx + N + 1>(d)); \
            }                                                                                 \
            else                                                                              \
            {                                                                                 \
                BOOST_UNREACHABLE_RETURN(f(std::get<0>(_flds), Params::get_member<1>(d)));    \
            }                                                                                 \
        }                                                                                     \
        /**/

        switch(actIdx)
        {
            BOOST_PP_REPEAT(
                _VISIT_CASE_CNT,
                _VISIT_CASE, _)
        }

        constexpr auto next_idx = std::min(FldIdx + _VISIT_CASE_CNT, sizeof...(Flds));

        //if constexpr(next_idx < sizeof...(Flds)) causes some weird msvc bug
        if constexpr(next_idx + 0 < sizeof...(Flds))
        {
            return visit<next_idx>(actIdx, d, JKL_FORWARD(f));
        }

        BOOST_UNREACHABLE_RETURN(f(std::get<0>(_flds), Params::get_member<1>(d)));

    #undef _VISIT_CASE
    #undef _VISIT_CASE_CNT
    }

    template<bool SkipLen = false>
    constexpr size_t len_cache_cnt(auto const& d) const noexcept
    {
        static_assert(! SkipLen);

        if(size_t actIdx = Params::active_member_idx(d))
            return visit(actIdx, d, [](auto& fld, auto& m) -> size_t { return fld.template len_cache_cnt<SkipLen>(m); });

        return 0; // no fld set
    }

    template<bool SkipTag = false, bool SkipLen = false>
    constexpr pb_size_t wire_size(auto const& d, pb_size_t*& lc) const noexcept
    {
        static_assert(! SkipTag && ! SkipLen);

        if(size_t actIdx = Params::active_member_idx(d))
            return visit(actIdx, d, [&](auto& fld, auto& m) -> pb_size_t { return fld.template wire_size<SkipTag, SkipLen>(m, lc); });

        return 0; // no fld set
    }

    template<bool SkipTag, bool SkipLen, _byte_ B>
    constexpr B* write_impl(B* b, auto const& d, pb_size_t const*& lc) const noexcept
    {
        static_assert(! SkipTag && ! SkipLen);

        if(size_t actIdx = Params::active_member_idx(d))
            return visit(actIdx, d, [&](auto& fld, auto& m) -> B* { return fld.template write_impl<SkipTag, SkipLen>(b, m, lc); });

        return b; // no fld set
    }

    // you can only read oneof field from surrendering message's read()
    //template<bool SkipLen, _byte_ B>
    //constexpr aresult<B const*> read_impl(B const* beg, B const* end, auto& d) const
};

template<strlit Name>
constexpr auto pb_oneof(auto&&... flds)
{
    return [&](auto&&... p){
        return pb_oneof_fld<Name, decltype(make_pb_params(JKL_FORWARD(p)...)), JKL_DECL_NO_CVREF_T(JKL_FORWARD(flds))...>{
                                           make_pb_params(JKL_FORWARD(p)...),                      JKL_FORWARD(flds)...};
    };
}


} // namespace jkl
