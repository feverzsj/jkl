#pragma once

#include <jkl/sqlite3/config.hpp>
#include <jkl/sqlite3/stmt.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <deque>


namespace jkl{


class sqlite3_flags_t
{
    explicit constexpr sqlite3_flags_t(int v) : value(v) {}

public:
    int const value = 0; // SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_PRIVATECACHE | SQLITE_OPEN_URI;
    
    explicit constexpr sqlite3_flags_t() = default;

    friend constexpr auto operator|(sqlite3_flags_t r, sqlite3_flags_t l) { return sqlite3_flags_t(r.value | l.value); }

    // flag clear
    // if uri/mutex/cache flags are cleared, method like sqlite3_conn.open may use sqlite3_global setting, or attach could use conn's openFlags.
    [[nodiscard]] constexpr auto clear         () const { return sqlite3_flags_t(0); }
    [[nodiscard]] constexpr auto clear_rwc     () const { return sqlite3_flags_t(value & ~(SQLITE_OPEN_READONLY | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)); }
    [[nodiscard]] constexpr auto clear_uri     () const { return sqlite3_flags_t(value & ~(SQLITE_OPEN_URI)); }
    [[nodiscard]] constexpr auto clear_mutex   () const { return sqlite3_flags_t(value & ~(SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_NOMUTEX)); }
    [[nodiscard]] constexpr auto clear_cache   () const { return sqlite3_flags_t(value & ~(SQLITE_OPEN_SHAREDCACHE | SQLITE_OPEN_PRIVATECACHE)); }
    [[nodiscard]] constexpr auto clear_non_rwcm() const { return clear_uri().clear_mutex().clear_cache(); }

    // flag set
    [[nodiscard]] constexpr auto ro () const { return clear_rwc() | sqlite3_flags_t(SQLITE_OPEN_READONLY                      ); }
    [[nodiscard]] constexpr auto rw () const { return clear_rwc() | sqlite3_flags_t(SQLITE_OPEN_READWRITE                     ); }
    [[nodiscard]] constexpr auto rwc() const { return clear_rwc() | sqlite3_flags_t(SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE); }
    [[nodiscard]] constexpr auto mem() const { return       rwc() | sqlite3_flags_t(SQLITE_OPEN_MEMORY                        ); }
    
    [[nodiscard]] constexpr auto uri         () const { return clear_uri  () | sqlite3_flags_t(SQLITE_OPEN_URI         ); }
    [[nodiscard]] constexpr auto fullmutex   () const { return clear_mutex() | sqlite3_flags_t(SQLITE_OPEN_FULLMUTEX   ); }
    [[nodiscard]] constexpr auto nomutex     () const { return clear_mutex() | sqlite3_flags_t(SQLITE_OPEN_NOMUTEX     ); }
    [[nodiscard]] constexpr auto sharedcache () const { return clear_cache() | sqlite3_flags_t(SQLITE_OPEN_SHAREDCACHE ); }
    [[nodiscard]] constexpr auto privatecache() const { return clear_cache() | sqlite3_flags_t(SQLITE_OPEN_PRIVATECACHE); }

    // flag test
    // NOTE: is_rw() include is_rwc(), so one should test is_rwc() first
    constexpr bool is_empty       () const { return  value == 0                                  ; }
    constexpr bool is_ro          () const { return  value & SQLITE_OPEN_READONLY                ; }
    constexpr bool is_rw          () const { return  value & SQLITE_OPEN_READWRITE               ; }
    constexpr bool is_rwc         () const { return (value & SQLITE_OPEN_CREATE      ) && is_rw(); }
    constexpr bool is_mem         () const { return  value & SQLITE_OPEN_MEMORY                  ; }
    constexpr bool is_uri         () const { return  value & SQLITE_OPEN_URI                     ; }
    constexpr bool is_nomutex     () const { return  value & SQLITE_OPEN_NOMUTEX                 ; }
    constexpr bool is_fullmutex   () const { return  value & SQLITE_OPEN_FULLMUTEX               ; }
    constexpr bool is_sharedcache () const { return  value & SQLITE_OPEN_SHAREDCACHE             ; }
    constexpr bool is_privatecache() const { return  value & SQLITE_OPEN_PRIVATECACHE            ; }


    [[nodiscard]] std::string to_uri_query(char const* vfs = nullptr) const
    {
        std::string t;
        
        if(is_empty())
            return t;

        char const* mode = nullptr;
        if     (is_ro ()) mode = "ro";
        else if(is_rwc()) mode = "rwc";
        else if(is_rw ()) mode = "rw";
        else if(is_mem()) mode = "memory";

        char const* cache = nullptr;
        if     (is_sharedcache ()) cache = "shared";
        else if(is_privatecache()) cache = "private";

        if(mode ) append_str(t, "mode=" , mode , "&");
        if(cache) append_str(t, "cache=", cache, "&");
        if(vfs  ) append_str(t, "vfs="  , vfs  , "&");

        t.pop_back();

        return t;
    }
};


// default: SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_PRIVATECACHE | SQLITE_OPEN_URI
// 
// SQLITE_OPEN_NOMUTEX means only the bCoreMutex is enabled at most,
// so the suggested use case is one conn per thread.
//      https://www.sqlite.org/threadsafe.html
//      https://www.sqlite.org/compile.html#threadsafe
//      
constexpr auto sqlite3_flags = sqlite3_flags_t().nomutex().privatecache().uri();



// refs:
// https://sqlite.org/lang.html
// https://sqlite.org/c3ref/intro.html
//
//
// recommended settings:
// 
// suppose the default sqlite3_flags: i.e.: nomutex().privatecache().uri()
//
//      sqlite3_conn conn;
//      
// improve general io:
//      
//      conn.set_journal_mode("WAL");
//      conn.set_synchronous("NORMAL");
//      conn.set_temp_store("MEMORY"); // may increase memory footage
//      conn.set_cache_size(...);  // may increase memory footage
//      conn.set_chunk_size(...);  // may increase file size, suggest: [1Mib, 10Mib]
//      //conn.set_page_size(...); // at least 4KiB, which should be the default
//      
// if db is only accessed by 1 thread:
// 
//      conn.set_locking_mode("EXCLUSIVE");
//
// if db is accessed by more than 1 threads:
//      
//      conn.set_busy_timeout(jkl::t_s(2));
// 
// if db is huge, and there're lots of connections:
//
//      conn.set_mmap_size(jkl::TiB(64));
//      
// when query involving large sort:
// 
//      conn.set_max_work_threads(std::thread::hardware_concurrency());
//      
// 
// settings not recommended:
// sharedcache:
//      sharedcache adds noticeable cost on Btree, and much more contention
//      chances between concurrent transactions.
//      The bundled sqlite fully disables the sharedcache.
// 
class sqlite3_conn
{
    struct deleter
    {
        void operator()(sqlite3* h)
        {
            int r = sqlite3_close_v2(h);
            if(r != SQLITE_OK)
            {
                // on close errors, sqlite3 leave conn open, which cannot be easily handled
                // with RAII and exception. The solution here is simply a hard failure,
                // as close errors are usually caused by improper use or unrecoverable errors.
                auto msg = cat_str("sqlite3_conn.close failed: ", sqlite3_errstr(r));
                JKL_ERR << msg;
                BOOST_ASSERT(false); // to fail early in debug
                throw sqlite3_err(msg); // as unique_ptr use noexcept reset() and dtor,
                                        // throw here will result std::terminate, which is expected
            }
        }
    };
    std::unique_ptr<sqlite3, deleter> _h;

public:
    sqlite3_conn() = default;

    explicit sqlite3_conn(_str_ auto const& path, sqlite3_flags_t flags = sqlite3_flags.rwc(), char const* vfs = nullptr)
    {
        open(path, flags, vfs);
    }

    sqlite3* handle() const { return _h.get(); }
    explicit operator bool() const { return _h.operator bool(); }

    // https://www.sqlite.org/c3ref/open.html
    // https://sqlite.org/inmemorydb.html
    // https://www.sqlite.org/sharedcache.html
    // 
    // if path == "", a temporary database (not TEMP db) will be opened, each conn is assured to have different db.
    // so flags.sharedcache() doesn't make sense with empty path. 
    // 
    // all conns opened with same non empty path and flags.sharedcache() in same process, share same page cache.
    // 
    // all conns opened with same non empty path and flags.mem().sharedcache() (uri() should also on as already did by sqlite3_flags) in same process,
    // share same in-memory db, special name ":memory:" is also considered non empty path.
    // 
    // see: openDatabase() sqlite3BtreeOpen() in sqlite source code.
    // 
    // NOTE:
    //    if path is uri, the queries in uri will overwrite corresponding flags, but these overwrites won't affect db->openFlags.
    //    check the default per connection setting in sqlite3_flags_t.
    void open(_str_ auto const& path, sqlite3_flags_t flags = sqlite3_flags.rwc(), char const* vfs = nullptr)
    {
        if(str_size(path) == 0 && flags.is_sharedcache())
            throw sqlite3_err("sqlite3_conn.open: sharedcache cannot be applied to empty path (i.e. temporary db)");

        sqlite3* h = nullptr;
        int r = sqlite3_open_v2(as_cstr(path).data(), &h, flags.value, vfs);
        
        if(r != SQLITE_OK)
        {
            sqlite3_close(h);
            throw sqlite3_err(r);
        }

        _h.reset(h);
        enable_extended_result_codes(true);
    }

    void close()
    {
        _h.reset();
    }

    // reset database back to empty database with no schema and no content
    // works even for a badly corrupted database file
    void clear()
    {
        enable_config(SQLITE_DBCONFIG_RESET_DATABASE, true);
        exec("VACUUM");
        enable_config(SQLITE_DBCONFIG_RESET_DATABASE, false);
    }


    // https://sqlite.org/lang_attach.html
    // 
    // the attached db will be opened using the same flags passed to conn.open(), except those specified in uri when open(),
    // and also exclude SQLITE_OPEN_FULLMUTEX/SQLITE_OPEN_NOMUTEX, these are just connection mutex settings.
    //
    // you can overwrite conn's openFlags's ro/rw/rwc/mem/xxxcache via uri query or 'flags' which is converted to query part of uri.
    // so uri should not use ':memory:', 'mode=xxx', 'cache=xxx', if corresponding flags is set.
    // 
    // see: openDatabase() sqlite3BtreeOpen() in sqlite source code.
    //
    // NOTE: the attached db must using same encoding as main;
    //       total attached dbs cannot exceed SQLITE_MAX_ATTACHED or max_attached_dbs()
    void attach(_str_ auto const& path, _str_ auto const& schema, sqlite3_flags_t flags = sqlite3_flags.clear(), char const* vfs = nullptr)
    {
        // TODO: use an actual uri parser

        if(( boost::contains(path, ":memory:") &&  flags.is_mem() ) ||
           ( boost::contains(path, "mode="   ) && (flags.is_ro() || flags.is_rw() || flags.is_mem ()) )
           ( boost::contains(path, "cache="  ) && (flags.is_sharedcache() || flags.is_privatecache()) )
           ( boost::contains(path, "vfs="    ) && (vfs                                              ) )
        )
        {
            throw sqlite3_err("sqlite3_conn.attach: uri query conflict with flags or vfs");
        }

        std::string p;

        if(! boost::starts_with(path, "file:"))
            p += "file:";
        p += path;

        if(auto q = flags.to_uri_query(vfs); q.size())
        {
            if(p.find('?') == npos)
                p += '?';
            else
                p += '&';

            p += q;
        }

        exec("ATTACH ? AS ?", p, schema);
    }

    void detach(_str_ auto const& schema)
    {
        exec("DETACH ?", schema);
    }


    // https://sqlite.org/pragma.html#pragma_table_info
    bool table_exists(_str_ auto const& schema, _str_ auto const& tbl)
    {
        // NOTE: PRAGMA doesn't support bindings,
        //       but PRAGMAs that return results and that have no side-effects
        //       can be accessed from ordinary SELECT statements as table-valued functions(which is still experimental)
        //       PRAGMA schema.table_info(tbl) is equal to:
        //       SELECT * FROM schema.pragma_table_info('tbl')
        
        //return prepare(cat_as_str("PRAGMA ", schema, ".table_info(", tbl, ")")).next();

        // optimizer should be smart enough to stop after one record is found, so 'LIMIT 1' is not needed
        return exec<bool>(cat_str("SELECT EXISTS(SELECT 1 FROM ", schema, ".pragma_table_info('", tbl, "'))"));
    }

    bool table_exists(_str_ auto const& tbl)
    {
        return table_exists("main", tbl);
    }

    // https://sqlite.org/pragma.html#pragma_database_list
    template<class Seq = std::deque<std::string>>
    auto db_names()
    {
        Seq dbs;

        auto ps = prepare("SELECT name from pragma_database_list()");
        // columns: seq(INTEGER), name(TEXT), file(TEXT)

        while(ps.next())
            dbs.emplace_back(ps.utf8());

        return dbs;
    }


    template<class T>
    T pragma_get(_str_ auto const& pragma)
    {
        return exec<T>(cat_as_str("PRAGMA ", pragma));
    }

    template<class T>
    T pragma_get(_str_ auto const& scheme, _str_ auto const& pragma)
    {
        return exec<T>(cat_as_str("PRAGMA ", scheme, ".", pragma));
    }

    template<bool CheckResult = true, class T>
    void pragma_set(_str_ auto const& pragma, T const& expectedResult)
    {
        constexpr bool ResultIsStr = _str_<T>;

        char const* quote = ResultIsStr ? "'" : "";
        auto sql = cat_as_str("PRAGMA ", pragma, "=", quote, expectedResult, quote);

        exec(sql);

        if constexpr(CheckResult)
        {
            auto actualResult = pragma_get<std::conditional_t<ResultIsStr, std::string, T>>(pragma);

            bool ok = false;

            if constexpr(ResultIsStr)
            {
                ok = boost::iequals(expectedResult, actualResult);
            }
            else
            {
                ok = (expectedResult == actualResult);
            }

            if(! ok)
                throw sqlite3_err(cat_as_str(
                    "pragma_set(): '", sql, "' returned '", actualResult,
                    "' which is different from '", expectedResult, "'"));
        }
    }

    template<bool CheckResult = true>
    void pragma_set(_str_ auto const& scheme, _str_ auto const& pragma, auto const& expectedResult)
    {
        pragma_set<CheckResult>(cat_as_str(scheme, ".", pragma), expectedResult);
    }


    // https://sqlite.org/pragma.html#pragma_application_id
    void set_application_id(_str_ auto const& schema, int32_t id) { pragma_set(schema, "application_id", id); }
    void set_application_id(                          int32_t id) { set_application_id("main", id); }

    int32_t application_id(_str_ auto const& schema) { return pragma_get<int32_t>(schema, "application_id"); }
    int32_t application_id(                        ) { return application_id("main"); }

    // https://sqlite.org/pragma.html#pragma_user_version
    void set_user_version(_str_ auto const& schema, int32_t ver) { pragma_set(schema, "user_version", ver); }
    void set_user_version(                          int32_t ver) { set_user_version("main", ver); }

    int32_t user_version(_str_ auto const& schema) { return pragma_get<int32_t>(schema, "user_version"); }
    int32_t user_version(                        ) { return user_version("main"); }

    // https://sqlite.org/pragma.html#pragma_schema_version
    // incremented on schema changes, usually should not be modified by user
    void set_schema_version(_str_ auto const& schema, int32_t ver) { pragma_set(schema, "schema_version", ver); }
    void set_schema_version(                          int32_t ver) { set_schema_version("main", ver); }

    int32_t schema_version(_str_ auto const& schema) { return pragma_get<int32_t>(schema, "schema_version"); }
    int32_t schema_version(                        ) { return schema_version("main"); }

    // https://sqlite.org/pragma.html#pragma_optimize
    // To achieve the best long-term query performance without the need to do a detailed engineering analysis
    // of the application schema and SQL, it is recommended that applications run optimize just before closing
    // each database connection. Long-running applications might also benefit from setting a timer to run
    // optimize every few hours.
    void optimize(_str_ auto const& schema, unsigned mask = 0xfffe) { pragma_set<false>(schema, "optimize", mask); }
    void optimize(                          unsigned mask = 0xfffe) { optimize("main", mask); }

    void optimize_all(unsigned mask = 0xfffe) { pragma_set<false>("optimize", mask); }


    // https://sqlite.org/pragma.html#pragma_auto_vacuum
    // NONE/0 | FULL/1 | INCREMENTAL/2
    // 
    // NONE/0: pages of deleted data are reused, db file never shrink
    // FULL/1: at every transaction commit, freelist pages are moved to the end of the database file and the database file is truncated to remove the freelist pages
    // INCREMENTAL/2: freelist pages are moved and truncted only when user executes PRAGMA schema.incremental_vacuum(N);
    // Auto-vacuum does not defragment the database nor repack individual database pages the way that the VACUUM command does.
    // In fact, because it moves pages around within the file, auto-vacuum can actually make fragmentation worse.
    // 
    // The only advance seems to avoid the database file size from bloating if speed is not of first concern.
    // 
    // default: NONE/0
    // 
    // NOTE: can only be set when the database is first created, as FULL and INCREMENTAL requires storing
    //       additional information that allows each database page to be traced backwards to its referrer
    template<class T> requires(_str_<T> || std::convertible_to<T, int>)
    void set_auto_vacuum(_str_ auto const& schema, T const& mode)
    {
        int m = -1;

        if constexpr(_str_<T>)
        {
                 if(boost::iequals(mode, "NONE"       )) m = 0;
            else if(boost::iequals(mode, "FULL"       )) m = 1;
            else if(boost::iequals(mode, "INCREMENTAL")) m = 2;
        }
        else
        {
            m = mode;
        }

        if(m < 0 || m > 2)
            throw sqlite3_err(cat_as_str("set_auto_vacuum(): invalid mode: ", mode));

        pragma_set(schema, "auto_vacuum", m);
    }

    void set_auto_vacuum(auto const& mode) { set_auto_vacuum("main", mode); }

    int auto_vacuum_mode(_str_ auto const& schema) { return pragma_get<int>(schema, "auto_vacuum"); }
    int auto_vacuum_mode(                        ) { return auto_vacuum_mode("main"); }

    // https://sqlite.org/pragma.html#pragma_incremental_vacuum
    // only take effect when auto_vacuum mode is INCREMENTAL.
    // If 'pages' < freelist page count, or if 'pages' < 1, the entire freelist is cleared.
    void incremental_vacuum(_str_ auto const& schema, int pages = -1) { pragma_set<false>(schema, "incremental_vacuum", pages); }
    void incremental_vacuum(                          int pages = -1) { incremental_vacuum("main", pages); }


    // https://sqlite.org/pragma.html#pragma_automatic_index
    void enable_automatic_index (bool on = true) {        pragma_set("automatic_index", on); }
    bool automatic_index_enabled(              ) { return pragma_get<bool>("automatic_index"); }


    // https://sqlite.org/pragma.html#pragma_case_sensitive_like
    // default: disabled
    void enable_case_sensitive_like (bool on = true) {        pragma_set("case_sensitive_like", on); }
    bool case_sensitive_like_enabled(              ) { return pragma_get<bool>("case_sensitive_like"); }

    // https://sqlite.org/pragma.html#pragma_cell_size_check
    // default: disabled
    void enable_cell_size_check (bool on = true) {        pragma_set("cell_size_check", on); }
    bool cell_size_check_enabled(              ) { return pragma_get<bool>("cell_size_check"); }

    // https://sqlite.org/pragma.html#pragma_defer_foreign_keys
    void defer_fk_check   (bool on = true) {        pragma_set("defer_foreign_keys", on); }
    bool fk_check_deferred(              ) { return pragma_get<bool>("defer_foreign_keys"); }

    // https://sqlite.org/pragma.html#pragma_ignore_check_constraints
    // default: always check
    void ignore_check_constraints (bool on = true) {        pragma_set("ignore_check_constraints", on); }
    bool check_constraints_ignored(              ) { return pragma_get<bool>("ignore_check_constraints"); }


    // https://sqlite.org/pragma.html#pragma_cache_size
    // https://sqlite.org/c3ref/pcache_methods2.html
    // won't persist, each conn has it's own setting
    void set_cache_pages(_str_ auto const& schema, unsigned n) { pragma_set(schema, "cache_size", n); }
    void set_cache_pages(                          unsigned n) { set_cache_pages("main", n); }

    unsigned cache_pages(_str_ auto const& schema)
    {
        auto n = pragma_get<int>(schema, "cache_size");
        if(n >= 0)
            return static_cast<unsigned>(n);
        return static_cast<unsigned>(std::ceil(  double(-n) * 1024. / page_size().count()  ));
    }

    unsigned cache_pages() { return cache_pages("main"); }

    // https://sqlite.org/pragma.html#pragma_cache_size
    // https://sqlite.org/c3ref/pcache_methods2.html
    // won't persist, each conn has it's own setting
    // default: SQLITE_DEFAULT_CACHE_SIZE
    // this is the max cache size, sqlite3 default cache method allocates
    // several pages each time, until the total caches size reach this limit.
    void set_cache_size(_str_ auto const& schema, KiB<int> kb) { pragma_set(schema, "cache_size", -(kb.count())); }
    void set_cache_size(                          KiB<int> kb) { set_cache_size("main", kb); }
    
    _B_<int> cache_size(_str_ auto const& schema)
    {
        auto n = pragma_get<int>(schema, "cache_size");
        if(n <= 0)
            return KiB(-n);
        return page_size() * n;
    }

    _B_<int> cache_size() { return cache_size("main"); }

    // https://sqlite.org/pragma.html#pragma_mmap_size
    // cannot exceed SQLITE_MAX_MMAP_SIZE or sqlite3_global.set_mmap_size(min, max)
    void set_mmap_size(_str_ auto const& schema, _B_<int64_t> bytes) { pragma_set(schema, "mmap_size", bytes.count()); }
    void set_mmap_size(                          _B_<int64_t> bytes) { set_mmap_size("main", bytes); }

    _B_<int64_t> mmap_size(_str_ auto const& schema) { return _B_(pragma_get<int64_t>(schema, "mmap_size")); }
    _B_<int64_t> mmap_size(                        ) { return mmap_size("main"); }

    // on cache and mmap size
    // 
    // Dirty pages in a single transaction would always go to cache first.
    // If they are larger than cache, some of them will be spilled to db.(check cache_spilled)
    // https://sqlite.org/atomiccommit.html#_cache_spill_prior_to_commit
    // https://sqlite.org/pragma.html#pragma_cache_spill
    // 
    // While with mmap enabled, reads search both cache and mmap view, then actual db (in that case the page will be put in cache).
    // So the mmap size should be large enough to avoid seeking in actual db. Considering define a
    // very large SQLITE_MAX_MMAP_SIZE and mmap_size.
    // http://sqlite.1065341.n5.nabble.com/MMAP-performance-with-databases-over-2GB-td83543.html
    // and also mind common mmap pitfalls:
    // https://www.sublimetext.com/blog/articles/use-mmap-with-care
    // 
    // Conclusion:
    // The mmap may or may not improve performance depending on the mmap
    // implementation and your workload. Just try it out with a large mmap_size.
    // It's always safer to not use mmap, if portability is of concern.
    // Setting cache_size as large as possible should be a safer bet,
    // and sqlite should also have better knowledge on caching.
    // If there are lots of conns to same db, use a large enough mmap size may improve general performance.


    // https://sqlite.org/pragma.html#pragma_page_size
    // should be set on db creation or before VACUUM.
    // The page_size() will not reflect the change immediately.
    // default: 4096 bytes
    void set_page_size(_str_ auto const& schema, _B_<int> bytes) { pragma_set<false>(schema, "page_size", bytes.count()); }
    void set_page_size(                          _B_<int> bytes) { set_page_size("main", bytes); }

    _B_<int> page_size(_str_ auto const& schema) { return _B_(pragma_get<int>(schema, "page_size")); }
    _B_<int> page_size(                 ) { return page_size("main"); }

    // https://sqlite.org/pragma.html#pragma_max_page_count
    void set_max_page_count(_str_ auto const& schema, int n) { pragma_set(schema, "max_page_count", n); }
    void set_max_page_count(                          int n) { set_max_page_count("main", n); }

    int max_page_count(_str_ auto const& schema) { return pragma_get<int>(schema, "max_page_count"); }
    int max_page_count(                        ) { return max_page_count("main"); }

    // https://sqlite.org/pragma.html#pragma_freelist_count
    int free_pages(_str_ auto const& schema) { return pragma_get<int>(schema, "freelist_count"); }
    int free_pages(                        ) { return free_pages("main"); }
    
    // https://sqlite.org/pragma.html#pragma_page_count
    int total_pages(_str_ auto const& schema) { return pragma_get<int>(schema, "page_count"); }
    int total_pages(                        ) { return total_pages("main"); }


    // https://sqlite.org/pragma.html#pragma_encoding
    // can only be used when the database is first created;
    // UTF-8 | UTF-16 | UTF-16le | UTF-16be
    // default is UTF-8
    void set_encoding(_str_ auto const& enc) {        pragma_set("encoding", enc); }
    auto     encoding(                     ) { return pragma_get<std::string>("encoding"); }

    // http://www.sqlite.org/c3ref/busy_timeout.html
    // http://www.sqlite.org/pragma.html#pragma_busy_timeout
    // sets a busy handler that sleeps for a specified amount of time when a table is locked.
    // The handler will sleep multiple times until at least "ms" milliseconds of sleeping have accumulated.
    // After at least "ms" milliseconds of sleeping, the handler returns 0 which causes sqlite3_step() to return SQLITE_BUSY.
    // Calling this routine with an argument less than or equal to zero turns off all busy handlers.
    // There can only be a single busy handler for a particular database connection at any given moment.
    // If another busy handler was defined (using sqlite3_busy_handler()) prior to calling this routine, that other busy handler is cleared.
    void set_busy_timeout(t_ms<int> ms) { sqlite3_throw_on_err(sqlite3_busy_timeout(handle(), ms.count())); }
    // NOTE: set_busy_timeout(ms that <= 0) clears busy handle, but not the last busyTimeout which returned by this method.
    t_ms<int> busy_timeout() { return t_ms(pragma_get<int>("busy_timeout")); }

    // https://sqlite.org/pragma.html#pragma_locking_mode
    // NORMAL | EXCLUSIVE
    // prefer EXCLUSIVE, when no other process will access the same schema/database.
    // default: NORMAL
    void set_locking_mode(_str_ auto const& schema, _str_ auto const& mode) { pragma_set(schema, "locking_mode", mode); }
    void set_locking_mode(                          _str_ auto  const& mode) { set_locking_mode("main", mode); }

    // applied to all databases(except temp), including any databases attached later
    void set_all_locking_mode(_str_ auto const& mode) { pragma_set("locking_mode", mode); }

    auto locking_mode(_str_ auto const& schema) { return pragma_get<std::string>(schema, "locking_mode"); }
    auto locking_mode(                        ) { return locking_mode("main"); }

    // https://sqlite.org/pragma.html#pragma_journal_mode
    // DELETE | TRUNCATE | PERSIST | MEMORY | WAL | OFF
    // 
    // In rollback journal modes(DELETE | TRUNCATE | PERSIST | MEMORY), contents to be changed
    // are written to journal first, then the database is modified, on transaction commit, the
    // journal is reset, transaction is safely committed. So a write transaction blocks any other
    // transactions in rollback journal modes.
    // https://sqlite.org/lockingv3.html#rollback
    // 
    // In WAL mode, changes(modified pages) are appended to wal first. On transaction commit, if wal
    // contains more pages(not used by other read transactions) than wal_autocheckpoint, these pages
    // will be move back to database file. This is call checkpointing. WAL mode supports multiple read
    // transactions and a single write transaction. WAL implements MVCC, so that each read transaction
    // starts at different time, when write transaction interleaved, can see different version of database.
    // Large wal could slow down read, as read may need look into both database and wal.
    // https://www.sqlite.org/wal.html
    // 
    // If your application mostly writes lots of new rows in batch, rollback journal modes or OFF are faster.
    // If your application mostly reads, rollback journal modes or OFF are faster.
    // If either, WAL is preferred.
    // 
    // rollback journal modes are recommended to be used with FULL or NORMAL synchronous mode.
    // WAL mode is recommended to be used with NORMAL synchronous mode.
    // 
    // If you just want populate a database as fast as possible, set journal_mode=OFF and synchronous=OFF.
    // Batch lots of writes in a single transaction and set a large enough cache size to hold such transaction.
    // 
    // OFF: disables transaction ROLLBACK.
    //
    // default: DELETE
    void set_journal_mode(_str_ auto const& schema, _str_ auto const& mode) { pragma_set(schema, "journal_mode", mode); }
    void set_journal_mode(_str_ auto const& mode) { set_journal_mode("main", mode); }

    void set_all_journal_mode(_str_ auto const& mode) { pragma_set("journal_mode", mode); }

    auto journal_mode(_str_ auto const& schema) { return pragma_get<std::string>(schema, "journal_mode"); }
    auto journal_mode(                        ) { return journal_mode("main"); }

    // https://sqlite.org/pragma.html#pragma_journal_size_limit
    // -1 for no limit (default)
    //  0 for minimum 
    void set_journal_size_limit(_str_ auto const& schema, _B_<int> bytes) { pragma_set(schema, "journal_size_limit", bytes.count()); }
    void set_journal_size_limit(                          _B_<int> bytes) { set_journal_size_limit("main", bytes); }

    _B_<int> journal_size_limit(_str_ auto const& schema) { return _B_(pragma_get<int>(schema, "journal_size_limit")); }
    _B_<int> journal_size_limit(                        ) { return journal_size_limit("main"); }


    // https://sqlite.org/c3ref/wal_autocheckpoint.html
    // https://sqlite.org/pragma.html#pragma_wal_autocheckpoint
    void set_wal_autocheckpoint(int pages) { sqlite3_throw_on_err(sqlite3_wal_autocheckpoint(handle(), pages)); }
    int wal_autocheckpoint() { return pragma_get<int>("wal_autocheckpoint"); }

    // https://www.sqlite.org/c3ref/wal_checkpoint_v2.html
    // https://sqlite.org/pragma.html#pragma_wal_checkpoint
    std::pair<int, int> wal_checkpoint(_str_ auto const& db, int mode = SQLITE_CHECKPOINT_PASSIVE)
    {
        int nLog = 0, nCkpt = 0;
        sqlite3_throw_on_err(
            sqlite3_wal_checkpoint_v2(handle(), as_cstr(db).data(), mode, &nLog, &nCkpt),
            handle());
        return {nLog, nCkpt};
    }

    void wal_checkpoint_all(int mode = SQLITE_CHECKPOINT_PASSIVE)
    {
        sqlite3_throw_on_err(
            sqlite3_wal_checkpoint_v2(handle(), nullptr, mode, nullptr, nullptr),
            handle());
    }


    // https://sqlite.org/pragma.html#pragma_synchronous
    //  OFF/0 | NORMAL/1 | FULL/2 | EXTRA/3
    //  
    //  durabilities:
    //  
    //               OFF  |     NORMAL         |  FULL  |  EXTRA
    // App crash :    ok         ok                ok       ok
    // OS/Power  :    no    WAL may rollback       ok       ok
    //                      last trxn, DELETE
    //                      may rarely corrupt.
    template<class T> requires(_str_<T> || std::convertible_to<T, int>)
    void set_synchronous(_str_ auto const& schema, T const& mode)
    {
        int m = -1;

        if constexpr(_str_<T>)
        {
                 if(boost::iequals(mode, "OFF"   )) m = 0;
            else if(boost::iequals(mode, "NORMAL")) m = 1;
            else if(boost::iequals(mode, "FULL"  )) m = 2;
            else if(boost::iequals(mode, "EXTRA" )) m = 3;
        }
        else
        {
            m = mode;
        }

        if(m < 0 || m > 3)
            throw sqlite3_err(cat_as_str("set_synchronous(): invalid mode: ", mode));

        pragma_set(schema, "synchronous", m);
    }

    void set_synchronous(auto const& mode) { set_synchronous("main", mode); }

    int synchronous_mode(_str_ auto const& schema) { return pragma_get<int>(schema, "synchronous"); }
    int synchronous_mode(                        ) { return synchronous_mode("main"); }


    // https://www.sqlite.org/pragma.html#pragma_temp_store
    // DEFAULT/0 | FILE/1 | MEMORY/2
    template<class T> requires(_str_<T> || std::convertible_to<T, int>)
    void set_temp_store(T const& mode)
    {
        int m = -1;

        if constexpr(_str_<T>)
        {
                 if(boost::iequals(mode, "DEFAULT")) m = 0;
            else if(boost::iequals(mode, "FILE"   )) m = 1;
            else if(boost::iequals(mode, "MEMORY" )) m = 2;
        }
        else
        {
            m = mode;
        }

        if(m < 0 || m > 2)
            throw sqlite3_err(cat_as_str("set_temp_store(): invalid mode: ", mode));

        pragma_set("temp_store", m);
    }

    int temp_store() { return pragma_get<int>("temp_store"); }                                                                                                                                                              
                                                                                                                                                              // https://www.sqlite.org/c3ref/get_autocommit.html
    // Autocommit mode is on by default. It's disabled by a BEGIN statement and re-enabled by a COMMIT or ROLLBACK.
    bool autocommit() { return sqlite3_get_autocommit(handle()) != 0; }

    char const* filename(_str_ auto const& db)
    {
        return sqlite3_db_filename(handle(), as_cstr(db).data());
    }

    sqlite3_pstmt prepare(_str_ auto const& sql, unsigned prepFlags = 0)
    {
        sqlite3_stmt* stmt = nullptr;
        sqlite3_throw_on_err(
            sqlite3_prepare_v3(handle(), str_data(sql), static_cast<int>(str_size(sql)),
                               prepFlags, &stmt, nullptr),
            handle());
        return sqlite3_pstmt(stmt);
    }

    sqlite3_pstmt prepare_persistent(_str_ auto const& sql)
    {
        return prepare(sql, SQLITE_PREPARE_PERSISTENT);
    }

    // can run zero or more UTF-8 encoded semicolon-separate SQL statements
    // if result is required, use exec() or prepare()
    void exec_multi(_str_ auto const& sqls)
    {
        sqlite3_throw_on_err(
            sqlite3_exec(handle(), as_cstr(sqls).data(), nullptr, nullptr, nullptr),
            handle());
    }

    // execute one SQL statement with args, optionally return specified column(s) of first row.
    // if T is not void, return the Ith column of first row;
    // if T is tuple like, return columns start at Ith;
    // NOTE: don't use none ownership type like string_view,
    //       as the statement will be reset when this method return.
    template<class T = void, unsigned Ith = 0>
    T exec(_str_ auto const& sql, auto const&... a)
    {
        return prepare(sql).exec<T, Ith>(a...);
    }

    int64_t last_insert_rowid() { return sqlite3_last_insert_rowid(handle()); }

    int affected_rows() const { return sqlite3_changes(handle()); }

    int last_err() const { return sqlite3_errcode(handle()); }

    char const* last_err_msg() const { return sqlite3_errmsg(handle()); }

    int last_extended_err() const { return sqlite3_extended_errcode(handle()); }

    void enable_extended_result_codes(bool enable)
    {
        sqlite3_throw_on_err(sqlite3_extended_result_codes(handle(), enable ? 1 : 0));
    }

    int total_changes() { return sqlite3_total_changes(handle()); }

    void interrupt() { sqlite3_interrupt(handle()); }

    // use enable_load_extension_capi() to enable before use this
    void load_extension(_str_ auto const& file, char const* entryPoint = nullptr)
    {
        char* errMsg = nullptr;
        int ec = sqlite3_load_extension(handle(), as_cstr(file).data(), entryPoint, &errMsg);
        if(ec != SQLITE_OK)
        {
            sqlite3_err t(errMsg, ec);
            sqlite3_free(errMsg);
            throw t;
        }
    }

    fts5_api* fts5()
    {
        fts5_api* t = nullptr;
        exec("SELECT fts5(?)", sqlite3_ptr_arg(&t, "fts5_api_ptr"));
        if(! t)
            throw sqlite3_err("sqlite3_conn.fts5: no fts5_api");
        return t;
    }

// Low-Level Control Of VFS
// https://sqlite.org/c3ref/file_control.html
// https://sqlite.org/c3ref/c_fcntl_begin_atomic_write.html

    void fcntl(_str_ auto const& db, int op, void* v)
    {
        if(SQLITE_OK != sqlite3_file_control(handle(), as_cstr(db).data(), op, v))
            throw sqlite3_err("sqlite3_conn::fcntl failed: possiblely caused by invalid db name or xFileControl method");
    }

    // https://sqlite.org/c3ref/c_fcntl_begin_atomic_write.html#sqlitefcntlchunksize
    //
    // extends and truncates the database file in chunk
    // large chunks may reduce file system level fragmentation and improve performance on some systems,
    // at the cost of larger file size.
    void set_chunk_size(_str_ auto const& db, _B_<int> s)
    {
        static_assert(sizeof(s) == sizeof(int));
        fcntl(db, SQLITE_FCNTL_CHUNK_SIZE, &s);
    }

    void set_chunk_size(_B_<int> s) { set_chunk_size("main", s); }

    void set_all_chunk_size(_B_<int> s)
    {
        for(auto const& db : db_names())
            set_chunk_size(db, s);
    }

    void set_journal_chunk_size(_str_ auto const& db, _B_<int> s)
    {
        static_assert(sizeof(s) == sizeof(int));

        sqlite3_file* f = nullptr;
        
        fcntl(db, SQLITE_FCNTL_JOURNAL_POINTER, &f);

        if(! f)
            throw sqlite3_err("sqlite3_conn::set_journal_chunk_size: cannot find jounral");

        if(SQLITE_OK != f->pMethods->xFileControl(f, SQLITE_FCNTL_CHUNK_SIZE, &s))
            throw sqlite3_err("sqlite3_conn::set_journal_chunk_size failed");
    }

    void set_journal_chunk_size(_B_<int> s) { set_journal_chunk_size("main", s); }

    void set_all_journal_chunk_size(_B_<int> s)
    {
        for(auto const& db : db_names())
            set_journal_chunk_size(db, s);
    }

    // https://sqlite.org/c3ref/c_fcntl_begin_atomic_write.html#sqlitefcntlvfsname
    std::string vfs_name(_str_ auto const& db)
    {
        std::string t;

        char* s = nullptr;
        fcntl(db, SQLITE_FCNTL_VFSNAME, &s);
        if(s)
        {
            t = s;
            sqlite3_free(s);
        }

        return t;
    }


// config
// https://sqlite.org/c3ref/c_dbconfig_defensive.html

    void config(int op, auto... a)
    {
        sqlite3_throw_on_err(sqlite3_db_config(handle(), op, a...));
    }

    void enable_config(int op, bool enable = true)
    {
        config(op, enable ? 1 : 0, nullptr);
    }

    bool config_enabled(int op)
    {
        int t;
        config(op, -1, &t);
        return t;
    }

    void set_lookaside(int sz, int n, void* b) { config(SQLITE_DBCONFIG_LOOKASIDE, b, sz, n); }

    void enable_fk(bool enable = true) { enable_config(SQLITE_DBCONFIG_ENABLE_FKEY, enable); }
    bool fk_enabled() { config_enabled(SQLITE_DBCONFIG_ENABLE_FKEY); }

    void enable_trigger(bool enable = true) { enable_config(SQLITE_DBCONFIG_ENABLE_TRIGGER, enable); }
    bool trigger_enabled() { config_enabled(SQLITE_DBCONFIG_ENABLE_TRIGGER); }

    void enable_fts3_tokenizer(bool enable = true) { enable_config(SQLITE_DBCONFIG_ENABLE_FTS3_TOKENIZER, enable); }
    bool fts3_tokenizer_enabled() { config_enabled(SQLITE_DBCONFIG_ENABLE_FTS3_TOKENIZER); }

    void enable_load_extension_capi(bool enable = true) { enable_config(SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, enable); }
    bool load_extension_capi_enabled() { config_enabled(SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION); }

    // enable/disable load extension from both sql and capi
    void enable_load_extension_both(bool enable = true) { sqlite3_enable_load_extension(handle(), enable ? 1 : 0); }

    void set_maindb_name(_str_ auto const& b) { config(SQLITE_DBCONFIG_MAINDBNAME, as_cstr(b).data()); }

    void enable_ckpt_on_close(bool enable = true) { enable_config(SQLITE_DBCONFIG_NO_CKPT_ON_CLOSE, !enable); }
    bool ckpt_on_close_enabled() { return ! config_enabled(SQLITE_DBCONFIG_NO_CKPT_ON_CLOSE); }

    void enable_qpsg(bool enable = true) { enable_config(SQLITE_DBCONFIG_ENABLE_QPSG, enable); }
    bool qpsg_enabled() { config_enabled(SQLITE_DBCONFIG_ENABLE_QPSG); }

    void enable_trigger_eqp(bool enable = true) { enable_config(SQLITE_DBCONFIG_TRIGGER_EQP, enable); }
    bool trigger_eqp_enabled() { config_enabled(SQLITE_DBCONFIG_TRIGGER_EQP); }

    void enable_defensive(bool enable = true) { enable_config(SQLITE_DBCONFIG_DEFENSIVE, enable); }
    bool defensive_enabled() { config_enabled(SQLITE_DBCONFIG_DEFENSIVE); }

    void enable_writable_schema(bool enable = true) { enable_config(SQLITE_DBCONFIG_WRITABLE_SCHEMA, enable); }
    bool writable_schema_enabled() { config_enabled(SQLITE_DBCONFIG_WRITABLE_SCHEMA); }
    
    void enable_legacy_alter_table(bool enable = true) { enable_config(SQLITE_DBCONFIG_LEGACY_ALTER_TABLE, enable); }
    bool legacy_alter_table_enabled() { config_enabled(SQLITE_DBCONFIG_LEGACY_ALTER_TABLE); }

    void enable_dqs_dml(bool enable = true) { enable_config(SQLITE_DBCONFIG_DQS_DML, enable); }
    bool dqs_dml_enabled() { config_enabled(SQLITE_DBCONFIG_DQS_DML); }
    
    void enable_dqs_ddl(bool enable = true) { enable_config(SQLITE_DBCONFIG_DQS_DDL, enable); }
    bool dqs_ddl_enabled() { config_enabled(SQLITE_DBCONFIG_DQS_DDL); }

// limit
// https://sqlite.org/c3ref/limit.html
// https://sqlite.org/c3ref/c_limit_attached.html
    void set_limit(int id, int newVal)
    {
        if(sqlite3_limit(handle(), id, newVal) != newVal)
            throw sqlite3_err(cat_as_str("set_limit(", id, ", ", newVal, ") failed"));
    }

    int limit(int id) { return sqlite3_limit(handle(), id, -1); }

    // currently only some ORDER BY query can take advantage of multi threaded sort.
    // default: 0, i.e. disabled.
    // http://sqlite.1065341.n5.nabble.com/how-is-quot-pragma-threads-4-quot-working-td91469.html
    // https://www.sqlite.org/c3ref/c_config_covering_index_scan.html#sqliteconfigpmasz
    void set_max_work_threads(int n) {    set_limit(SQLITE_LIMIT_WORKER_THREADS, n); }
    int      max_work_threads(     ) { return limit(SQLITE_LIMIT_WORKER_THREADS); }

    void set_max_attached_dbs(int n) {    set_limit(SQLITE_LIMIT_ATTACHED, n); }
    int      max_attached_dbs(     ) { return limit(SQLITE_LIMIT_ATTACHED); }

// status
// https://sqlite.org/c3ref/c_dbstatus_options.html

    std::pair<int, int> status(int op, bool reset = false)
    {
        int cur = 0, highWater = 0;
        sqlite3_throw_on_err(sqlite3_db_status(handle(), op, &cur, &highWater, reset ? 1 : 0));
        return {cur, highWater};
    }

    std::pair<int, int> reset_status(int op, bool reset = true)
    {
        return status(op, reset);
    }

    int lookaside_used(bool reset = false) { return status(SQLITE_DBSTATUS_LOOKASIDE_USED, reset).first; }
    int reset_lookaside_used(bool reset = true) { return lookaside_used(reset); }

    int lookaside_hit(bool reset = false) { return status(SQLITE_DBSTATUS_LOOKASIDE_HIT, reset).second; }
    int reset_lookaside_hit(bool reset = true) { return lookaside_hit(reset); }

    int lookaside_miss_size(bool reset = false) { return status(SQLITE_DBSTATUS_LOOKASIDE_MISS_SIZE, reset).second; }
    int reset_lookaside_miss_size(bool reset = true) { return lookaside_miss_size(reset); }

    int lookaside_miss_full(bool reset = false) { return status(SQLITE_DBSTATUS_LOOKASIDE_MISS_FULL, reset).second; }
    int reset_lookaside_miss_full(bool reset = true) { return lookaside_miss_full(reset); }

    int cache_used_shared() { return status(SQLITE_DBSTATUS_CACHE_USED_SHARED).first; }
    int cache_used() { return status(SQLITE_DBSTATUS_CACHE_USED).first; }

    int schema_used() { return status(SQLITE_DBSTATUS_SCHEMA_USED).first; }
    int stmt_used() { return status(SQLITE_DBSTATUS_STMT_USED).first; }

    int cache_spilled(bool reset = false) { return status(SQLITE_DBSTATUS_CACHE_SPILL, reset).first; }
    int reset_cache_spilled(bool reset = true) { return cache_spilled(reset); }

    int cache_hit(bool reset = false) { return status(SQLITE_DBSTATUS_CACHE_HIT, reset).first; }
    int reset_cache_hit(bool reset = true) { return cache_hit(reset); }

    int cache_miss(bool reset = false) { return status(SQLITE_DBSTATUS_CACHE_MISS, reset).first; }
    int reset_cache_miss(bool reset = true) { return cache_miss(reset); }

    int cache_written(bool reset = false) { return status(SQLITE_DBSTATUS_CACHE_WRITE, reset).first; }
    int reset_cache_written(bool reset = true) { return cache_written(reset); }

    int deferred_fk_checks() { return status(SQLITE_DBSTATUS_DEFERRED_FKS).first; }
};


// https://sqlite.org/lang_transaction.html
// NOTE: do not nest transactions, use savepoint 
class sqlite3_scoped_transaction
{
    sqlite3_conn& _c;
    bool _commited = false;

public:
    explicit sqlite3_scoped_transaction(sqlite3_conn& c, std::string_view const& type = "DEFERRED")
        : _c(c)
    {
        _c.exec(cat_str("BEGIN ", type));
    }

    void commit()
    {
        BOOST_ASSERT(! _commited);
        _c.exec("COMMIT");
        _commited = true;
    }

    ~sqlite3_scoped_transaction()
    {
        if(! _commited)
            _c.exec("ROLLBACK");
    }

    // https://sqlite.org/lang_savepoint.html
    // 
    // SAVEPOINT names are case-insensitive, and do not have to be unique.
    // Any RELEASE or ROLLBACK TO of a SAVEPOINT will act upon the most recent one with a matching name.
    //
    // SAVEPOINT merely marks the uncommitted changes, and go through them.

    void savepoint(_str_ auto const& name)
    {
        _c.exec(cat_str("SAVEPOINT ", name));
    }

    void release(_str_ auto const& name)
    {
        _c.exec(cat_str("RELEASE ", name));
    }

    void rollback_to(_str_ auto const& name)
    {
        _c.exec(cat_str("ROLLBACK TO ", name));
    }
};

class sqlite3_scoped_savepoint
{
    sqlite3_scoped_transaction& _tr;
    bool _released = false;
    std::string _name;

public:
    explicit sqlite3_scoped_savepoint(sqlite3_scoped_transaction& t, std::string_view const& name = "sp")
        : _tr(t), _released(false), _name(name)
    {
        _tr.savepoint(name);
    }

    void release()
    {
        BOOST_ASSERT(! _released);
        _tr.release(_name);
        _released = true;
    }

    ~sqlite3_scoped_savepoint()
    {
        if(! _released)
            _tr.rollback_to(_name);
    }
};




// https://sqlite.org/backup.html
// https://sqlite.org/c3ref/backup_finish.html
class sqlite3_db_backup
{
    struct deleter
    {
        void operator()(sqlite3_backup* h)
        {
            int r = sqlite3_backup_finish(h);
            if(r != SQLITE_OK)
                JKL_ERR << sqlite3_errstr(r);

            BOOST_ASSERT(r == SQLITE_OK); // to fail early in debug
        }
    };
    std::unique_ptr<sqlite3_backup, deleter> _h;

public:
    sqlite3_db_backup(sqlite3_conn& src, _str_ auto const& srcDb, sqlite3_conn& dst, _str_ auto const& dstDb)
    {
        sqlite3_backup* h = sqlite3_backup_init(dst.handle(), as_cstr(dstDb).data(),
                                                src.handle(), as_cstr(srcDb).data());
        
        if(! h)
            throw sqlite3_err(dst.last_err_msg(), dst.last_err());
        
        _h.reset(h);
    }

    sqlite3_db_backup(sqlite3_conn& src, sqlite3_conn& dst)
        : sqlite3_db_backup(src, "main", dst, "main")
    {}

    sqlite3_backup* handle() const { return _h.get(); }

    // return if there is more pages to process
    // unrecoverable errors will be thrown.
    bool step(int nPage)
    {
        int r = sqlite3_backup_step(handle(), nPage);
        
        if(r = SQLITE_OK || r == SQLITE_BUSY || r == SQLITE_LOCKED)
            return false;

        if(r == SQLITE_DONE)
            return true;

        throw sqlite3_err(r);
    }

    bool step_all() { return step(-1); }

    // source db remaining and total page count, only update on step()
    int remaining() { return sqlite3_backup_remaining(handle()); }
    int pagecount() { return sqlite3_backup_pagecount(handle()); }
};


} // namespace std