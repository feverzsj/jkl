#pragma once

#include <jkl/sqlite3/config.hpp>
#include <jkl/util/str.hpp>
#include <jkl/util/assign_append.hpp>

#include <boost/numeric/conversion/cast.hpp>

#include <memory>


namespace jkl{


class sqlite3_pstmt;


void sqlite3_bind_arg(sqlite3_pstmt& s, int i, auto const& a);

void sqlite3_get_col(sqlite3_pstmt& s, int i, auto& c);


class sqlite3_ptr_arg
{
public:
    void* p;
    char const* t;

    explicit sqlite3_ptr_arg(void* p_, char const* t_ = nullptr) : p(p_), t(t_) {}
};



// suggested usages:
// 
// for:
//   sqlite3_pstmt ps = db.prepare(...);
//   
// to just execute:
//   ps.exec(a...);
// 
// to read first column of first row:
//   ps.exec<ColType>(a...);
//   
// to read Ith column of first row:
//   ps.exec<ColType, Ith>(a...);
//   
// to read N columns of first row:
//   ps.exec<TupleLike<ColTypes...>>(a...);
//
// to read N columns start at Ith of first row:
//   ps.exec<TupleLike<ColTypes...>, Ith>(a...);
//
// to read multiple rows
// {
//     // sqlite3_pstmt ps = db.prepare(...);
//     // or if sqlite3_pstmt itself is not scoped, use: 
//     auto resetter = ps.scoped_resetter();
// 
//     ps.args(...);
//     
//     while(ps.next())
//     {
//         get columns of current row use various retrieve methods
//     }
// }
class sqlite3_pstmt
{
    friend class sqlite3_conn;

    struct deleter
    {
        void operator()(sqlite3_stmt* h) noexcept
        {
            // sqlite3_finalize()/sqlite3_reset() should only return error when last sqlite3_step() fails, except SQLITE_NOMEM.
            // and the error code is same as sqlite3_step()'s. For SQLITE_NOMEM, you'll get it later anyway.
            // not throw is necessary for ~sqlite3_pstmt() and scoped_resetter.
        #ifndef NDEBUG
            sqlite3_finalize(h);
        #else
            if(sqlite3_finalize(h) != SQLITE_OK)
                JKL_ERR << "sqlite3_pstmt.finalize return error: " << sqlite3_errstr(r);
        #endif
        }
    };

    std::unique_ptr<sqlite3_stmt, deleter> _h;
    
    explicit sqlite3_pstmt(sqlite3_stmt* h) : _h(h) {}

    sqlite3* db_handle() const { return sqlite3_db_handle(handle()); }

public:
    sqlite3_pstmt() = default;

    sqlite3_stmt* handle() const { return _h.get(); }
    explicit operator bool() const { return _h.operator bool(); }

    char const* sql() { return sqlite3_sql(handle()); }
    char const* normalized_sql() { return sqlite3_normalized_sql(handle()); }
    
    std::string expanded_sql()
    {
        std::string r;
        if(char* s = sqlite3_expanded_sql(handle()))
        {
            r = s;
            sqlite3_free(s);
        }
        return r;
    }

    bool is_explain() { return sqlite3_stmt_isexplain(handle()) == 1; }
    bool is_explain_query_plain() { return sqlite3_stmt_isexplain(handle()) == 2; }

    bool readonly() { return sqlite3_stmt_readonly(handle()); }

    // NOTE: arg/param index is 1 based
    int arg_count() { return sqlite3_bind_parameter_count(handle()); }

    // NOTE: can only find idx of named parameter, name should like: :VVV, @VVV, $VVV
    // ref: https://www.sqlite.org/c3ref/bind_blob.html
    int arg_index(_cstr_ auto const& utf8name)
    {
        int i = sqlite3_bind_parameter_index(handle(), cstr_data(utf8name));
        if(i <= 0)
            throw sqlite3_err(SQLITE_NOTFOUND);
        return i;
    }

    char const* arg_name(int i) { return sqlite3_bind_parameter_name(handle(), i); }

    void finalize() { _h.reset(); }

    // https://www.sqlite.org/c3ref/reset.html
    // https://www.sqlite.org/lang_transaction.html  search "Implicit versus explicit transactions"
    // 
    // reset to initial state, ready to be re-executed, bindings are not touched.
    // if the prepared statement is in its own implicit transaction (i.e. it's not in any explicit transaction),
    // the implicit transaction will be committed if it hasn't been committed.
    // sqlite3_finalize() will also commit uncommitted implicit transaction, so no need to call this on destruction.
    void reset() noexcept
    {
        // sqlite3_finalize()/sqlite3_reset() should only return error when last sqlite3_step() fails, except SQLITE_NOMEM.
        // and the error code is same as sqlite3_step()'s. For SQLITE_NOMEM, you'll get it later anyway.
        // not throw is necessary for ~sqlite3_pstmt() and scoped_resetter.
    #ifndef NDEBUG
        sqlite3_reset(handle());
    #else
        if(sqlite3_reset(handle()) != SQLITE_OK)
            JKL_ERR << "sqlite3_pstmt.reset return error: " << sqlite3_errstr(r);
    #endif

    }

    auto scoped_resetter()
    {
        struct scoped_resetter_t
        {
            sqlite3_pstmt& stmt;
            scoped_resetter_t(sqlite3_pstmt& s) : stmt(s) {}
            ~scoped_resetter_t() { stmt.reset(); }
        };

        return scoped_resetter_t(*this);
    }

    // execute the statement with args.
    // if T is not void, return the Ith column of first row;
    // if T is tuple like, return columns start at Ith;
    // NOTE: don't use none ownership type like string_view,
    //       as the statement will be reset when this method return.
    template<class T = void, unsigned Ith = 0>
    T exec(auto const&... a)
    {
        auto resetter = scoped_resetter();

        args(a...);

        if constexpr(std::is_void_v<T>)
        {
            next();
        }
        else
        {
            if(! next())
                throw sqlite3_err("no row to retrieve");

            if constexpr(_tuple_like_<T>)
            {
                T t;
                std::apply([this](auto&... c){ cols<Ith>(c...); }, t);
                return t;
            }
            else
            {
                return col<T>(Ith);
            }
        }
    }

// argument binding

    void null_arg(int i)
    {
        sqlite3_throw_on_err(sqlite3_bind_null(handle(), i));
    }
    
    void num_arg(int i, std::integral auto v)
    {
        sqlite3_throw_on_err(sqlite3_bind_int64(handle(), i, boost::numeric_cast<int64_t>(v)));
    }

    void num_arg(int i, std::floating_point auto v)
    {
        sqlite3_throw_on_err(sqlite3_bind_double(handle(), i, boost::numeric_cast<double>(v)));
    }

    void blob_arg(int i, _byte_buf auto const& b)
    {
        sqlite3_throw_on_err(sqlite3_bind_blob64(handle(), i, buf_data(b), (buf_byte_size)(b), SQLITE_STATIC));
    }

    void text_arg(int i, _str_ auto const& s, unsigned char encoding)
    {
        sqlite3_throw_on_err(
            sqlite3_bind_text64(handle(), i,
                reinterpret_cast<char const*>(str_data(s)), str_byte_size(s), SQLITE_STATIC, encoding));
    }

    void utf8_arg(int i, _str_ auto const& s)
    {
        static_assert(sizeof(str_value(s)) == 1);
        text_arg(i, s, SQLITE_UTF8);
    }

    void utf16_arg(int i, _str_ auto const& s)
    {
        static_assert(sizeof(str_value(s)) == 2);
        text_arg(i, s, SQLITE_UTF16);
    }

    void ptr_arg(int i, auto* p, char const* type, void(*dtor)(void*) = nullptr)
    {
        sqlite3_throw_on_err(sqlite3_bind_pointer(handle(), i, p, type, dtor));
    }

    void arg(int i, auto const& a)
    {
        sqlite3_bind_arg(*this, i, a);
    }
    
    void arg(_cstr_ auto const& n, auto const& a)
    {
        arg(arg_index(n), a);
    }

    sqlite3_pstmt& args(auto const&... a)
    {
        BOOST_ASSERT(arg_count() == sizeof...(a));

        if constexpr(sizeof...(a) > 0)
        {
            int i = 1;
            (... , arg(i++, a));
        }

        return *this; 
    }

    // sqlite3_clear_bindings() is just bad and ambigous design, don't use it.
    // // bind all arguments to NULL, also release SQLITE_TRANSIENT bindings
    // void bind_all_args_to_null()
    // {
    //     sqlite3_throw_on_err(sqlite3_clear_bindings(handle()));
    // }


// result retrieve

    bool next() // also actually execute the statement on first call, args should be all binded properly
    {
        int r = sqlite3_step(handle());

        if(r == SQLITE_ROW)
            return true;

        if(r == SQLITE_DONE)
            return false;

        throw sqlite3_err(r);
    }

    bool busy() { return sqlite3_stmt_busy(handle()); }

    // column is 0 based

    int col_count() { return sqlite3_column_count(handle()); }
    int col_type(int i) { return sqlite3_column_type(handle(), i); }

    // only for SELECT statement
    char const* col_name(int i) { return sqlite3_column_name(handle(), i); }
    char const* col_ori_name(int i) { return sqlite3_column_origin_name(handle(), i); }
    char const* col_tbl_name(int i) { return sqlite3_column_table_name(handle(), i); }
    char const* col_db_name(int i) { return sqlite3_column_database_name(handle(), i); }


    template<_arithmetic_ T>
    T num(int i = 0)
    {
        switch(col_type(i))
        {
            case SQLITE_INTEGER: return boost::numeric_cast<T>(sqlite3_column_int64 (handle(), i));
            case SQLITE_FLOAT  : return boost::numeric_cast<T>(sqlite3_column_double(handle(), i));
        }

        throw sqlite3_err("cannot convert the column type to numeric type");

        // sqlite3_column_int() returns the lower 32 bits of that value without checking for overflows,
        // so it is not used here

        // sqlite will try its best to convert text/blob to numerics,
        // which cannot detect the conversion failure.
        // it's better to leave that to the user.
        // 
        // if constexpr(std::is_integral_v<T>)
        //     return std::numeric_cast<T>(sqlite3_column_int64(handle(), i));
        // else if constexpr(std::is_floating_point_v<T>)
        //     return std::numeric_cast<T>(sqlite3_column_double(handle(), i));
        // else
        //     JKL_DEPENDENT_FAIL(T, "unsupported type");
    }
 
    auto   bool_(int i = 0) { return num<bool              >(i); }
    auto  short_(int i = 0) { return num<short             >(i); }
    auto ushort_(int i = 0) { return num<unsigned short    >(i); }
    auto    int_(int i = 0) { return num<int               >(i); }
    auto   uint_(int i = 0) { return num<unsigned          >(i); }
    auto   long_(int i = 0) { return num<long              >(i); }
    auto  ulong_(int i = 0) { return num<unsigned long     >(i); }
    auto  llong_(int i = 0) { return num<long long         >(i); }
    auto ullong_(int i = 0) { return num<unsigned long long>(i); }
    auto  float_(int i = 0) { return num<float             >(i); }
    auto double_(int i = 0) { return num<double            >(i); }

    auto   int8_(int i = 0) { return num<int8_t  >(i); }
    auto  uint8_(int i = 0) { return num<uint8_t >(i); }
    auto  int16_(int i = 0) { return num<int16_t >(i); }
    auto uint16_(int i = 0) { return num<uint16_t>(i); }
    auto  int32_(int i = 0) { return num<int32_t >(i); }
    auto uint32_(int i = 0) { return num<uint32_t>(i); }
    auto  int64_(int i = 0) { return num<int64_t >(i); }
    auto uint64_(int i = 0) { return num<uint64_t>(i); }
    auto  sizet_(int i = 0) { return num<size_t  >(i); }


    // NOTE:
    // be careful with non-ownership type like string_view, it only lasts until the statement is reset or destructed;
    // https://en.cppreference.com/w/cpp/language/lifetime
    // All temporary objects are destroyed as the last step in evaluating the full-expression
    // that (lexically) contains the point where they were created ...

    // extract blob or text with same encoding directly, transcoding text if necessary
    // stringify other type
    
    template<_str_ Str = std::string_view> requires(sizeof(str_value_t<Str>) == sizeof(char8_t))
    Str utf8(int i = 0)
    {
        // evaluation order matters
        auto* s = sqlite3_column_text(handle(), i);
        auto  n = sqlite3_column_bytes(handle(), i);

        return {reinterpret_cast<char const*>(s), static_cast<size_t>(n)};
    }
    
    // UTF-16 native
    template<_str_ Str = std::u16string_view> requires(sizeof(str_value_t<Str>) == sizeof(char16_t))
    Str utf16(int i = 0)
    {
        // evaluation order matters
        auto* s = sqlite3_column_text16(handle(), i);
        auto  n = sqlite3_column_bytes16(handle(), i);

        return {reinterpret_cast<char16_t const*>(s), static_cast<size_t>(n / 2)};
    }

    // extract text/blob directly, and stringify other type
    template<_byte_buf_ Buf = std::string_view>
    Buf blob(int i = 0)
    {
        // evaluation order matters
        auto* b = sqlite3_column_blob(handle(), i);
        auto  n = sqlite3_column_bytes(handle(), i);

        return {reinterpret_cast<char const*>(b), static_cast<size_t>(n)};
    }


    void get_col(int i, auto& c)
    {
        sqlite3_get_col(*this, i, c);
    }
    
    // get cols start at Ith column
    template<unsigned Ith = 0>
    void cols(auto&... c)
    {
        BOOST_ASSERT(col_count() == sizeof...(c));
        
        if constexpr(sizeof...(c) > 0)
        {
            int i = Ith;
            (..., get_col(i++, c));
        }
    }

    template<class C>
    C col(int i)
    {
        C c;
        get_col(i, c);
        return c;
    }

// status
// https://sqlite.org/c3ref/c_stmtstatus_counter.html

    int status(int op, bool reset = false) { return sqlite3_stmt_status(handle(), op, reset ? 1 : 0); }
    int reset_status(int op, bool reset = true) { return status(op, reset); }

    int fullscan_step_cnt(bool reset = false) { return status(SQLITE_STMTSTATUS_FULLSCAN_STEP, reset); }
    int reset_fullscan_step_cnt(bool reset = true) { return fullscan_step_cnt(reset); }

    int sort_cnt(bool reset = false) { return status(SQLITE_STMTSTATUS_SORT, reset); }
    int reset_sort_cnt(bool reset = true) { return sort_cnt(reset); }

    int autoindx_cnt(bool reset = false) { return status(SQLITE_STMTSTATUS_AUTOINDEX, reset); }
    int reset_autoindx_cnt(bool reset = true) { return autoindx_cnt(reset); }

    int vm_step_cnt(bool reset = false) { return status(SQLITE_STMTSTATUS_VM_STEP, reset); }
    int reset_vm_step_cnt(bool reset = true) { return vm_step_cnt(reset); }

    int prepare_cnt(bool reset = false) { return status(SQLITE_STMTSTATUS_REPREPARE, reset); }
    int reset_prepare_cnt(bool reset = true) { return prepare_cnt(reset); }

    int run_cnt(bool reset = false) { return status(SQLITE_STMTSTATUS_RUN, reset); }
    int reset_run_cnt(bool reset = true) { return run_cnt(reset); }

    int approximate_memused() { return status(SQLITE_STMTSTATUS_MEMUSED); }
};



void sqlite3_bind_arg(sqlite3_pstmt& s, int i, sqlite3_ptr_arg const& a)
{
    s.ptr_arg(i, a.p, a.t);
}

void sqlite3_bind_arg(sqlite3_pstmt& s, int i, nullptr_t)
{
    s.null_arg(i);
}

void sqlite3_bind_arg(sqlite3_pstmt& s, int i, _arithmetic_ auto a)
{
    s.num_arg(i, a);
}

template<_str_ A> requires(sizeof(str_value_t<A>) == sizeof(char8_t))
void sqlite3_bind_arg(sqlite3_pstmt& s, int i, A const& a)
{
    s.utf8_arg(i, a);
}

template<_str_ A> requires(sizeof(str_value_t<A>) == sizeof(char16_t))
void sqlite3_bind_arg(sqlite3_pstmt& s, int i, A const& a)
{
    s.utf16_arg(i, a);
}

void sqlite3_bind_arg(sqlite3_pstmt& s, int i, _byte_buf_ auto const& a)
{
    s.blob_arg(i, a);
}

void sqlite3_bind_arg(sqlite3_pstmt& s, int i, std::optional<auto> const& a)
{
    if(a)
        sqlite3_bind_arg(s, i, *a);
    else
        s.null_arg(i);
}



template<_arithmetic_ C>
void sqlite3_get_col(sqlite3_pstmt& s, int i, C& c)
{
    c = s.num<C>(i);
}

template<_str_ C> requires(sizeof(str_value_t<C>) == sizeof(char8_t))
void sqlite3_get_col(sqlite3_pstmt& s, int i, C& c)
{
    uassign(c, s.utf8(i));
}

template<_str_ C> requires(sizeof(str_value_t<C>) == sizeof(char16_t))
void sqlite3_get_col(sqlite3_pstmt& s, int i, C& c)
{
    uassign(c, s.utf16(i));
}

void sqlite3_get_col(sqlite3_pstmt& s, int i, _resizable_byte_buf_ auto& c)
{
    uassign(c, s.blob(i));
}

void sqlite3_get_col(sqlite3_pstmt& s, int i, std::optional<auto>& c)
{
    if(s.col_type(i) != SQLITE_NULL)
    {
        if(! c)
            c.emplace();
        s.get_col(i, *c);
    }
    else
    {
        c.reset();
    }
}


} // namespace std