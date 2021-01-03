#pragma once

#include <jkl/config.hpp>
#include <jkl/util/log.hpp>
#include <jkl/util/unit.hpp>
#include <stdexcept>
#include <sqlite3.h>


namespace jkl{


class sqlite3_err : public std::exception
{
    int _ec;
    std::string _msg;

public:
    explicit sqlite3_err(int ec) : _ec(ec) {}

    template<class Str>
    explicit sqlite3_err(Str&& msg, int ec = SQLITE_ERROR) : _ec(ec), _msg(std::forward<Str>(msg)) {}
    
    int err() const { return _ec; }
    char const* what() const override { return _msg.empty() ? sqlite3_errstr(_ec) : _msg.data(); }
};


inline void sqlite3_throw_on_err(int ec, sqlite3* h = nullptr)
{
    if(ec != SQLITE_OK)
        throw sqlite3_err((h ? sqlite3_errmsg(h) : ""), ec);
}


class sqlite3_global_t
{
public:
// status
// https://sqlite.org/c3ref/c_status_malloc_count.html

    static std::pair<int64_t, int64_t> status(int op, bool reset = false)
    {
        int64_t cur = 0, highWater = 0;
        sqlite3_throw_on_err(sqlite3_status64(op, &cur, &highWater, reset ? 1 : 0));
        return {cur, highWater};
    }

    static std::pair<int64_t, int64_t> reset_status(int op, bool reset = true)
    {
        return status(op, reset);
    }

    static int64_t memory_used() { return sqlite3_memory_used(); }
    static int64_t memory_highwater(bool reset = false) { return sqlite3_memory_highwater(reset ? 1 : 0); }
    static int64_t reset_memory_highwater(bool reset = true) { return memory_highwater(reset); }

    static int64_t malloc_size_highwater(bool reset = false) { return status(SQLITE_STATUS_MALLOC_SIZE, reset).second; }
    static int64_t reset_malloc_size_highwater(bool reset = true) { return malloc_size_highwater(reset); }

    static int64_t malloc_count() { return status(SQLITE_STATUS_MALLOC_COUNT).first; }
    static int64_t pagecache_used() { return status(SQLITE_STATUS_PAGECACHE_USED).first; }
    static int64_t pagecache_overflow() { return status(SQLITE_STATUS_PAGECACHE_OVERFLOW).first; }
    
    static int64_t pagecache_size_highwater(bool reset = false) { return status(SQLITE_STATUS_PAGECACHE_SIZE, reset).second; }
    static int64_t reset_pagecache_size_highwater(bool reset = true) { return pagecache_size_highwater(reset); }

#ifdef YYTRACKMAXSTACKDEPTH
    static int64_t parser_stack_highwater(bool reset = false) { return status(SQLITE_STATUS_PARSER_STACK, reset).second; }
    static int64_t reset_parser_stack_highwater(bool reset = true) { return parser_stack_highwater(reset); }
#endif

// config
// https://sqlite.org/c3ref/c_config_covering_index_scan.html

    template<class... A>
    static void config(int op, A... a)
    {
        sqlite3_throw_on_err(sqlite3_config(op, a...));
    }

    static void enable_config(int op, bool enable = true)
    {
        config(op, enable ? 1 : 0);
    }

    // https://www.sqlite.org/threadsafe.html
    // https://www.sqlite.org/compile.html#threadsafe
    static void use_singlethread() { config(SQLITE_CONFIG_SINGLETHREAD); } // bCoreMutex = 0; bFullMutex = 0;
    static void use_serialized  () { config(SQLITE_CONFIG_SERIALIZED  ); } // bCoreMutex = 1; bFullMutex = 1; same as SQLITE_OPEN_FULLMUTEX
    static void use_multithread () { config(SQLITE_CONFIG_MULTITHREAD ); } // bCoreMutex = 1; bFullMutex = 0; same as SQLITE_OPEN_NOMUTEX

    static void enable_uri(bool enable = true) { enable_config(SQLITE_CONFIG_URI, enable); }
    static void enable_covering_index_scan(bool enable = true) { enable_config(SQLITE_CONFIG_COVERING_INDEX_SCAN, enable); }
    
    static void enable_shared_cache(bool enable = true) { sqlite3_throw_on_err(sqlite3_enable_shared_cache(enable ? 1 : 0)); }

    static void prefer_small_malloc(bool prefer = true) { enable_config(SQLITE_CONFIG_SMALL_MALLOC, prefer); }
    static void enable_malloc_statics(bool enable = true) { enable_config(SQLITE_CONFIG_MEMSTATUS, enable); }

    static void set_pagecache(void* b, int sz, int n) { config(SQLITE_CONFIG_PAGECACHE, b, sz, n); }
    static void set_heap(void* b, _B_<int> s, _B_<int> min) { config(SQLITE_CONFIG_HEAP, b, s.count(), min.count()); }
    static void set_lookaside(int sz, int n) { config(SQLITE_CONFIG_LOOKASIDE, sz, n); }
    
    static void set_min_pma_size(unsigned n) { config(SQLITE_CONFIG_PMASZ, n); }

    // https://www.sqlite.org/c3ref/c_config_covering_index_scan.html#sqliteconfigmmapsize
    // https://www.sqlite.org/pragma.html#pragma_mmap_size
    static void set_mmap_size(_B_<int64_t> min, _B_<int64_t> max) { config(SQLITE_CONFIG_MMAP_SIZE, min.count(), max.count()); }
    
    static void set_stmtjrnl_spill(_B_<int> bytes) { config(SQLITE_CONFIG_STMTJRNL_SPILL, bytes.count()); }
    static void set_sorterref_size(_B_<int> bytes) { config(SQLITE_CONFIG_SORTERREF_SIZE, bytes.count()); }
    static void set_memdb_maxsize(_B_<int64_t> bytes) { config(SQLITE_CONFIG_MEMDB_MAXSIZE, bytes.count()); }

    static void set_mem_methods(sqlite3_mem_methods const& m) { config(SQLITE_CONFIG_MALLOC, &m); }
    static void get_mem_methods(sqlite3_mem_methods& m) { config(SQLITE_CONFIG_GETMALLOC, &m); }

    static void set_mutex_methods(sqlite3_mutex_methods const& m) { config(SQLITE_CONFIG_MUTEX, &m); }
    static void get_mutex_methods(sqlite3_mutex_methods& m) { config(SQLITE_CONFIG_GETMUTEX, &m); }

    static void set_pcache_methods(sqlite3_pcache_methods2 const& m) { config(SQLITE_CONFIG_PCACHE2, &m); }
    static void get_pcache_methods(sqlite3_pcache_methods2& m) { config(SQLITE_CONFIG_GETPCACHE2, &m); }
    
    static int pcahce_hdrsz()
    {
        int sz = 0;
        config(SQLITE_CONFIG_PCACHE_HDRSZ, &sz);
        return sz;
    }

    static void reset_errlog_callback(void(*cb)(void*, int, char const*) = nullptr, void* d = nullptr)
    {
        config(SQLITE_CONFIG_LOG, cb, d);
    }

#ifdef SQLITE_ENABLE_SQLLOG
    static void reset_sqllog_callback(void(*cb)(void*, sqlite3*, char const*, int) = nullptr, void* d = nullptr)
    {
        config(SQLITE_CONFIG_SQLLOG, cb, d);
    }
#endif

#ifdef SQLITE_WIN32_MALLOC
    static void set_win32_heapsize(_B_<int> bytes) { config(SQLITE_CONFIG_WIN32_HEAPSIZE, bytes.count()); }
#endif

#ifndef SQLITE_OMIT_COMPILEOPTION_DIAGS
    static auto used_compile_options()
    {
        std::vector<char const*> opts;
        int i = 0;
        while(char const* o = sqlite3_compileoption_get(i++))
            opts.emplace_back(o);
        return opts;
    }

    static bool compile_option_used(char const* o)
    {
        return sqlite3_compileoption_used(o) == 1;
    }
#endif
};


inline constexpr sqlite3_global_t sqlite3_global;


} // namespace jkl