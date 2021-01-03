#pragma once

#include <jkl/config.hpp>
#include <jkl/error.hpp>
#include <jkl/result.hpp>
#include <jkl/util/str.hpp>
#include <jkl/util/handle.hpp>
#include <unicode/ucnv.h>


namespace jkl{


class icu_err_category : public aerror_category
{
public:
    char const* name() const noexcept override { return "icu"; }

    std::string message(int rc) const override
    {
        switch(static_cast<UErrorCode>(rc))
        {
            case U_ILLEGAL_ARGUMENT_ERROR          : return "Illegal argument";
            case U_MISSING_RESOURCE_ERROR          : return "The requested resource cannot be found";
            case U_INVALID_FORMAT_ERROR            : return "Data format is not what is expected";
            case U_FILE_ACCESS_ERROR               : return "The requested file cannot be found";
            case U_INTERNAL_PROGRAM_ERROR          : return "Indicates a bug in the library code";
            case U_MESSAGE_PARSE_ERROR             : return "Unable to parse a message (message format)";
            case U_MEMORY_ALLOCATION_ERROR         : return "Memory allocation error";
            case U_INDEX_OUTOFBOUNDS_ERROR         : return "Trying to access the index that is out of bounds";
            case U_PARSE_ERROR                     : return "Equivalent to Java ParseException";
            case U_INVALID_CHAR_FOUND              : return "Character conversion: Unmappable input sequence. In other APIs: Invalid character.";
            case U_TRUNCATED_CHAR_FOUND            : return "Character conversion: Incomplete input sequence.";
            case U_ILLEGAL_CHAR_FOUND              : return "Character conversion: Illegal input sequence/combination of input units.";
            case U_INVALID_TABLE_FORMAT            : return "Conversion table file found, but corrupted";
            case U_INVALID_TABLE_FILE              : return "Conversion table file not found";
            case U_BUFFER_OVERFLOW_ERROR           : return "A result would not fit in the supplied buffer";
            case U_UNSUPPORTED_ERROR               : return "Requested operation not supported in current context";
            case U_RESOURCE_TYPE_MISMATCH          : return "an operation is requested over a resource that does not support it";
            case U_ILLEGAL_ESCAPE_SEQUENCE         : return "ISO-2022 illegal escape sequence";
            case U_UNSUPPORTED_ESCAPE_SEQUENCE     : return "ISO-2022 unsupported escape sequence";
            case U_NO_SPACE_AVAILABLE              : return "No space available for in-buffer expansion for Arabic shaping";
            case U_CE_NOT_FOUND_ERROR              : return "Currently used only while setting variable top, but can be used generally";
            case U_PRIMARY_TOO_LONG_ERROR          : return "User tried to set variable top to a primary that is longer than two bytes";
            case U_STATE_TOO_OLD_ERROR             : return "ICU cannot construct a service from this state, as it is no longer supported";
            case U_TOO_MANY_ALIASES_ERROR          : return "There are too many aliases in the path to the requested resource. It is very possible that a circular alias definition has occurred";
            case U_ENUM_OUT_OF_SYNC_ERROR          : return "UEnumeration out of sync with underlying collection";
            case U_INVARIANT_CONVERSION_ERROR      : return "Unable to convert a UChar* string to char* with the invariant converter.";
            case U_INVALID_STATE_ERROR             : return "Requested operation can not be completed with ICU in its current state";
            case U_COLLATOR_VERSION_MISMATCH       : return "Collator version is not compatible with the base version";
            case U_USELESS_COLLATOR_ERROR          : return "Collator is options only and no base is specified";
            case U_NO_WRITE_PERMISSION             : return "Attempt to modify read-only or constant data.";
            case U_BAD_VARIABLE_DEFINITION         : return "Missing '$' or duplicate variable name";
            case U_MALFORMED_RULE                  : return "Elements of a rule are misplaced";
            case U_MALFORMED_SET                   : return "A UnicodeSet pattern is invalid";
            case U_MALFORMED_SYMBOL_REFERENCE      : return "UNUSED as of ICU 2.4";
            case U_MALFORMED_UNICODE_ESCAPE        : return "A Unicode escape pattern is invalid";
            case U_MALFORMED_VARIABLE_DEFINITION   : return "A variable definition is invalid";
            case U_MALFORMED_VARIABLE_REFERENCE    : return "A variable reference is invalid";
            case U_MISMATCHED_SEGMENT_DELIMITERS   : return "UNUSED as of ICU 2.4";
            case U_MISPLACED_ANCHOR_START          : return "A start anchor appears at an illegal position";
            case U_MISPLACED_CURSOR_OFFSET         : return "A cursor offset occurs at an illegal position";
            case U_MISPLACED_QUANTIFIER            : return "A quantifier appears after a segment close delimiter";
            case U_MISSING_OPERATOR                : return "A rule contains no operator";
            case U_MISSING_SEGMENT_CLOSE           : return "UNUSED as of ICU 2.4";
            case U_MULTIPLE_ANTE_CONTEXTS          : return "More than one ante context";
            case U_MULTIPLE_CURSORS                : return "More than one cursor";
            case U_MULTIPLE_POST_CONTEXTS          : return "More than one post context";
            case U_TRAILING_BACKSLASH              : return "A dangling backslash";
            case U_UNDEFINED_SEGMENT_REFERENCE     : return "A segment reference does not correspond to a defined segment";
            case U_UNDEFINED_VARIABLE              : return "A variable reference does not correspond to a defined variable";
            case U_UNQUOTED_SPECIAL                : return "A special character was not quoted or escaped";
            case U_UNTERMINATED_QUOTE              : return "A closing single quote is missing";
            case U_RULE_MASK_ERROR                 : return "A rule is hidden by an earlier more general rule";
            case U_MISPLACED_COMPOUND_FILTER       : return "A compound filter is in an invalid location";
            case U_MULTIPLE_COMPOUND_FILTERS       : return "More than one compound filter";
            case U_INVALID_RBT_SYNTAX              : return "A '::id' rule was passed to the RuleBasedTransliterator parser";
            case U_INVALID_PROPERTY_PATTERN        : return "UNUSED as of ICU 2.4";
            case U_MALFORMED_PRAGMA                : return "A 'use' pragma is invalid";
            case U_UNCLOSED_SEGMENT                : return "A closing ')' is missing";
            case U_ILLEGAL_CHAR_IN_SEGMENT         : return "UNUSED as of ICU 2.4";
            case U_VARIABLE_RANGE_EXHAUSTED        : return "Too many stand-ins generated for the given variable range";
            case U_VARIABLE_RANGE_OVERLAP          : return "The variable range overlaps characters used in rules";
            case U_ILLEGAL_CHARACTER               : return "A special character is outside its allowed context";
            case U_INTERNAL_TRANSLITERATOR_ERROR   : return "Internal transliterator system error";
            case U_INVALID_ID                      : return "A '::id' rule specifies an unknown transliterator";
            case U_INVALID_FUNCTION                : return "A '&fn()' rule specifies an unknown transliterator";
            case U_UNEXPECTED_TOKEN                : return "Syntax error in format pattern";
            case U_MULTIPLE_DECIMAL_SEPARATORS     : return "More than one decimal separator in number pattern";
            case U_MULTIPLE_EXPONENTIAL_SYMBOLS    : return "More than one exponent symbol in number pattern";
            case U_MALFORMED_EXPONENTIAL_PATTERN   : return "Grouping symbol in exponent pattern";
            case U_MULTIPLE_PERCENT_SYMBOLS        : return "More than one percent symbol in number pattern";
            case U_MULTIPLE_PERMILL_SYMBOLS        : return "More than one permill symbol in number pattern";
            case U_MULTIPLE_PAD_SPECIFIERS         : return "More than one pad symbol in number pattern";
            case U_PATTERN_SYNTAX_ERROR            : return "Syntax error in format pattern";
            case U_ILLEGAL_PAD_POSITION            : return "Pad symbol misplaced in number pattern";
            case U_UNMATCHED_BRACES                : return "Braces do not match in message pattern";
            case U_UNSUPPORTED_PROPERTY            : return "UNUSED as of ICU 2.4";
            case U_UNSUPPORTED_ATTRIBUTE           : return "UNUSED as of ICU 2.4";
            case U_ARGUMENT_TYPE_MISMATCH          : return "Argument name and argument index mismatch in MessageFormat functions";
            case U_DUPLICATE_KEYWORD               : return "Duplicate keyword in PluralFormat";
            case U_UNDEFINED_KEYWORD               : return "Undefined Plural keyword";
            case U_DEFAULT_KEYWORD_MISSING         : return "Missing DEFAULT rule in plural rules";
            case U_DECIMAL_NUMBER_SYNTAX_ERROR     : return "Decimal number syntax error";
            case U_FORMAT_INEXACT_ERROR            : return "Cannot format a number exactly and rounding mode is ROUND_UNNECESSARY @stable ICU 4.8";
            case U_BRK_INTERNAL_ERROR              : return "An internal error (bug) was detected.";
            case U_BRK_HEX_DIGITS_EXPECTED         : return "Hex digits expected as part of a escaped char in a rule.";
            case U_BRK_SEMICOLON_EXPECTED          : return "Missing ';' at the end of a RBBI rule.";
            case U_BRK_RULE_SYNTAX                 : return "Syntax error in RBBI rule.";
            case U_BRK_UNCLOSED_SET                : return "UnicodeSet writing an RBBI rule missing a closing ']'.";
            case U_BRK_ASSIGN_ERROR                : return "Syntax error in RBBI rule assignment statement.";
            case U_BRK_VARIABLE_REDFINITION        : return "RBBI rule $Variable redefined.";
            case U_BRK_MISMATCHED_PAREN            : return "Mis-matched parentheses in an RBBI rule.";
            case U_BRK_NEW_LINE_IN_QUOTED_STRING   : return "Missing closing quote in an RBBI rule.";
            case U_BRK_UNDEFINED_VARIABLE          : return "Use of an undefined $Variable in an RBBI rule.";
            case U_BRK_INIT_ERROR                  : return "Initialization failure.  Probable missing ICU Data.";
            case U_BRK_RULE_EMPTY_SET              : return "Rule contains an empty Unicode Set.";
            case U_BRK_UNRECOGNIZED_OPTION         : return "!!option in RBBI rules not recognized.";
            case U_BRK_MALFORMED_RULE_TAG          : return "The {nnn} tag on a rule is malformed";
            case U_REGEX_INTERNAL_ERROR            : return "An internal error (bug) was detected.";
            case U_REGEX_RULE_SYNTAX               : return "Syntax error in regexp pattern.";
            case U_REGEX_INVALID_STATE             : return "RegexMatcher in invalid state for requested operation";
            case U_REGEX_BAD_ESCAPE_SEQUENCE       : return "Unrecognized backslash escape sequence in pattern";
            case U_REGEX_PROPERTY_SYNTAX           : return "Incorrect Unicode property";
            case U_REGEX_UNIMPLEMENTED             : return "Use of regexp feature that is not yet implemented.";
            case U_REGEX_MISMATCHED_PAREN          : return "Incorrectly nested parentheses in regexp pattern.";
            case U_REGEX_NUMBER_TOO_BIG            : return "Decimal number is too large.";
            case U_REGEX_BAD_INTERVAL              : return "Error in {minmax} interval";
            case U_REGEX_MAX_LT_MIN                : return "In {minmax} max is less than min.";
            case U_REGEX_INVALID_BACK_REF          : return "Back-reference to a non-existent capture group.";
            case U_REGEX_INVALID_FLAG              : return "Invalid value for match mode flags.";
            case U_REGEX_LOOK_BEHIND_LIMIT         : return "Look-Behind pattern matches must have a bounded maximum length.";
            case U_REGEX_SET_CONTAINS_STRING       : return "Regexps cannot have UnicodeSets containing strings.";
            case U_REGEX_MISSING_CLOSE_BRACKET     : return "Missing closing bracket on a bracket expression.";
            case U_REGEX_INVALID_RANGE             : return "In a character range [x-y] x is greater than y.";
            case U_REGEX_STACK_OVERFLOW            : return "Regular expression backtrack stack overflow.";
            case U_REGEX_TIME_OUT                  : return "Maximum allowed match time exceeded";
            case U_REGEX_STOPPED_BY_CALLER         : return "Matching operation aborted by user callback fn.";
            case U_REGEX_PATTERN_TOO_BIG           : return "Pattern exceeds limits on size or complexity. @stable ICU 55";
            case U_REGEX_INVALID_CAPTURE_GROUP_NAME: return "Invalid capture group name. @stable ICU 55";
            case U_IDNA_PROHIBITED_ERROR           : return "idna prohibited";
            case U_IDNA_UNASSIGNED_ERROR           : return "idna unassigned";
            case U_IDNA_CHECK_BIDI_ERROR           : return "idna check bidi";
            case U_IDNA_STD3_ASCII_RULES_ERROR     : return "idna std3 ascii rules";
            case U_IDNA_ACE_PREFIX_ERROR           : return "idna ace prefix";
            case U_IDNA_VERIFICATION_ERROR         : return "idna verification";
            case U_IDNA_LABEL_TOO_LONG_ERROR       : return "idna label too long";
            case U_IDNA_ZERO_LENGTH_LABEL_ERROR    : return "idna zero length label";
            case U_IDNA_DOMAIN_NAME_TOO_LONG_ERROR : return "idna domain name too long error";
            case U_PLUGIN_TOO_HIGH                 : return "The plugin's level is too high to be loaded right now.";
            case U_PLUGIN_DIDNT_SET_LEVEL          : return "The plugin didn't call uplug_setPlugLevel in response to a QUERY";
            default                                 : return "undefined";
        }
    }
};

inline constexpr icu_err_category g_icu_error_cat;
inline auto const& icu_err_cat() noexcept { return g_icu_error_cat; }

} // namespace jkl{

namespace boost::system{

template<> struct is_error_code_enum<UErrorCode> : public true_type {};

} // namespace boost::system

inline jkl::aerror_code make_error_code(UErrorCode e) noexcept { return {static_cast<int>(e), jkl::icu_err_cat()}; }



namespace jkl{


_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
int icu_compare_name(_char_str_ auto const& n1, _char_str_ auto const& n2)
{
    return ucnv_compareNames(as_cstr(n1).data(), as_cstr(n2).data());
}

_JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
bool icu_equal_name(_char_str_ auto const& n1, _char_str_ auto const& n2)
{
    return icu_compare_name(n1, n2) == 0;
}


class icu_charset
{
    struct deleter{ void operator()(UConverter* h) noexcept { ucnv_close(h); } };
    struct copier
    {
        UConverter* operator()(UConverter* h)
        {
            UErrorCode status = U_ZERO_ERROR;
            UConverter* t = ucnv_safeClone(h, nullptr, nullptr, &status);
            if(! t)
                throw asystem_error{status};
            return t;
        }
    };
    copyable_handle<UConverter*, deleter, copier> _h;

    icu_charset(UConverter* h) noexcept : _h{h} {}

public:
    icu_charset() = default;

    explicit operator bool() const noexcept { return _h.valid(); }
    UConverter* handle() const noexcept { return _h.get(); }

    void close() { _h.reset(); }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    static aresult<icu_charset> open(_char_str_ auto const& name)
    {
        UErrorCode e = U_ZERO_ERROR;
        if(UConverter* h = ucnv_open(as_cstr(name).data(), &e))
            return icu_charset{h};
        return e;
        // return {e, h}; don't use this, as UErrorCode may also represent warnings.
    }

    char const* name() const
    {
        UErrorCode e = U_ZERO_ERROR;
        return ucnv_getName(handle(), &e);
    }

    int32_t max_char_size() const noexcept { return ucnv_getMaxCharSize(handle()); }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    int32_t max_from_u16_result_size(_u16_str_ auto const& s)
    {
        return UCNV_GET_MAX_BYTES_FOR_STRING(str_size(s), max_char_size());
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    static constexpr int32_t max_to_u16_result_size(_byte_str_ auto const& s)
    {
        return 2 * str_size(s);
    }

    template<bool SkipMaxResize = false>
    aresult<> from_u16(_u16_str_ auto const& s, _resizable_byte_buf_ auto& d)
    {
        if constexpr(! SkipMaxResize)
            resize_buf(d, max_from_u16_result_size(s));

        auto sd = reinterpret_cast<UChar const*>(str_data(s));
        auto sn = static_cast     <int32_t     >(str_size(s));
        auto dd = reinterpret_cast<char*       >(buf_data(d));
        auto dn = static_cast     <int32_t     >(buf_size(d));
        UErrorCode e = U_ZERO_ERROR;

        int32_t n = ucnv_fromUChars(handle(), dd, dn, sd, sn, &e);

        if constexpr(SkipMaxResize)
        {
            if(n > dn)
            {
                resize_buf(d, n);
                dd = reinterpret_cast<char*>(buf_data(d));
                dn = n;
                n = ucnv_fromUChars(handle(), dd, dn, sd, sn, &e);
            }
        }

        if(e > 0)
            return e;

        BOOST_ASSERT(n <= dn);

        resize_buf(d, n);

        return no_err;
    }

    template<bool SkipMaxResize = false>
    aresult<> to_u16(_byte_str_ auto const& s, _resizable_16bits_buf_ auto& d)
    {
        if constexpr(! SkipMaxResize)
            resize_buf(d, max_to_u16_result_size(s));

        auto sd = reinterpret_cast<char const*>(str_data(s));
        auto sn = static_cast     <int32_t    >(str_size(s));
        auto dd = reinterpret_cast<UChar*     >(buf_data(d));
        auto dn = static_cast     <int32_t    >(buf_size(d));
        UErrorCode e = U_ZERO_ERROR;

        int32_t n = ucnv_toUChars(handle(), dd, dn, sd, sn, &e);

        if constexpr(SkipMaxResize)
        {
            if(n > dn)
            {
                resize_buf(d, n);
                dd = reinterpret_cast<UChar*>(buf_data(d));
                dn = n;
                n = ucnv_toUChars(handle(), dd, dn, sd, sn, &e);
            }
        }

        if(e > 0)
            return e;

        BOOST_ASSERT(n <= dn);

        resize_buf(d, n);

        return no_err;
    }


    template<_resizable_byte_buf_ B = string>
    aresult<B> from_u16(_u16_str_ auto const& s)
    {
        B d;
        JKL_TRY(from_u16(s, d));
        return d;
    }

    template<_resizable_16bits_buf_ B = u16string>
    aresult<B> to_u16(_byte_str_ auto const& s)
    {
        B d;
        JKL_TRY(to_u16(s, d));
        return d;
    }
};


template<bool SkipMaxResize = false>
aresult<> cvt_charset(icu_charset& srcCodec, icu_charset& dstCodec,
                      _byte_str_ auto const& s, _resizable_byte_buf_ auto& d)
{
    JKL_TRY(auto&& t, srcCodec.to_u16(s));
    return dstCodec.template from_u16<SkipMaxResize>(t, d);
}


class icu_charset_cvt
{
public:
    icu_charset src;
    icu_charset dst;

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    static aresult<icu_charset_cvt> open(_char_str_ auto const& srcName, _char_str_ auto const& dstName)
    {
        icu_charset_cvt cvt;
        JKL_TRY(cvt.src, icu_charset::open(srcName));
        JKL_TRY(cvt.dst, icu_charset::open(dstName));
        return cvt;
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    static aresult<> to_dst(_char_str_ auto const& srcCs, _byte_str_ auto const& s,
                            _char_str_ auto const& dstCs, _resizable_byte_buf_ auto& d)
    {
        JKL_TRY(auto&& cvt, open(srcCs, dstCs));
        return cvt.forward(s, d);
    }

    template<_resizable_byte_buf_ B = string>
    static aresult<B> to_dst(_char_str_ auto const& srcCs, _byte_str_ auto const& s,
                             _char_str_ auto const& dstCs)
    {
        B d;
        JKL_TRY(to_dst(srcCs, s, dstCs, d));
        return d;
    }

    // src => dst
    template<bool SkipMaxResize = false>
    aresult<> forward(_byte_str_ auto const& s, _resizable_byte_buf_ auto& d)
    {
        return cvt_charset<SkipMaxResize>(src, dst, s, d);
    }

    // dst => src
    template<bool SkipMaxResize = false>
    aresult<> inverse(_byte_str_ auto const& s, _resizable_byte_buf_ auto& d)
    {
        return cvt_charset<SkipMaxResize>(dst, src, s, d);
    }
};


} // namespace jkl
