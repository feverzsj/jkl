#pragma once


#include <jkl/util/str.hpp>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <lmdb.h>


namespace jkl{


class lmdb_error : std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
    explicit lmdb_error(int e) : std::runtime_error(mdb_strerror(e)) { BOOST_ASSERT(e); }
};

void lmdb_throw_on_err(int e)
{
    if(e != 0)
        throw lmdb_error(e);
}


class lmdb_env
{
    struct deleter
    {
        void operator()(MDB_env* h)
        {
            mdb_env_close(h);
        }
    };

    std::unique_ptr<MDB_env, deleter> _h;

public:
    lmdb_env(size_t MB = 0)
    {
        MDB_env* h = nullptr;
        lmdb_throw_on_err(mdb_env_create(&h));
        _h.reset(h);

        if(MB == 0)
        {
            if constexpr(sizeof(size_t) * CHAR_BIT >= 64)
                MB = 64*1024*1024; // 64TB
            else if constexpr(sizeof(size_t) * CHAR_BIT >= 32)
                MB = 2*1024; // 2GB
            else
                MB = 10; // 10MB
        }

        set_mapsize_mb(MB);
    }
    
    MDB_env* handle() const { return _h.get(); }

    // NOTE: the env is always opened with MDB_NOTLS, it's a better fit for c++
    template<class Str>
    void open(Str const& path, unsigned flags = 0, mdb_mode_t mode = 0)
    {        
        int e = mdb_env_open(handle(), as_cstr(path).data(), flags | MDB_NOTLS, mode);

        if(e != 0)
        {
            close();
            throw lmdb_error(e);
        }
    }

    template<class Str>
    void open_as_files(Str const& path, unsigned flags = 0, mdb_mode_t mode = 0)
    {
        open(path, flags | MDB_NOSUBDIR, mode);
    }

    void close() { _h.reset(); }

    template<class Str>
    void copy(Str const& path, unsigned flags = 0)
    {
        lmdb_throw_on_err(mdb_env_copy2(handle(), as_cstr(path).data(), flags));
    }

    void copyfd(mdb_filehandle_t fd, unsigned flags = 0)
    {
        lmdb_throw_on_err(mdb_env_copyfd2(handle(), fd, flags));
    }

    MDB_envinfo info()
    {
        MDB_envinfo t;
        lmdb_throw_on_err(mdb_env_info(handle(), &t));
        return t;
    }

    void sync(bool force = true)
    {
        lmdb_throw_on_err(mdb_env_sync(handle(), force ? 1 : 0));
    }

    void set_flags(unsigned flags, bool on = true)
    {
        lmdb_throw_on_err(mdb_env_set_flags(handle(), flags, on ? 1 : 0));
    }

    unsigned flags()
    {
        unsigned t;
        lmdb_throw_on_err(mdb_env_get_flags(handle(), &t));
        return t;
    }

    char const* path()
    {
        char const* t;
        lmdb_throw_on_err(mdb_env_get_path(handle(), &t));
        return t;
    }

    mdb_filehandle_t fd()
    {
        mdb_filehandle_t t;
        lmdb_throw_on_err(mdb_env_get_fd(handle(), &t));
        return t;
    }

    void set_mapsize_mb(size_t MB)
    {
        lmdb_throw_on_err(mdb_env_set_mapsize(handle(), MB * 1024 * 1024));
    }

    void set_maxreaders(unsigned n)
    {
        lmdb_throw_on_err(mdb_env_set_maxreaders(handle(), n));
    }

    unsigned maxreaders()
    {
        unsigned t;
        lmdb_throw_on_err(mdb_env_get_maxreaders(handle(), &t));
        return t;
    }

    void set_maxdbs(MDB_dbi n)
    {
        lmdb_throw_on_err(mdb_env_set_maxdbs(handle(), n));
    }

    int maxkeysize()
    {
        return mdb_env_get_maxkeysize(handle());
    }

};


class lmdb_txn_base
{
protected:
    struct deleter
    {
        void operator()(MDB_env* h)
        {
            mdb_txn_abort(h);
        }
    };

    std::unique_ptr<MDB_txn, deleter> _h;

    lmdb_txn_base() = default;

    void begin(MDB_env* env, MDB_txn* parent, unsigned flags)
    {
        MDB_txn* h = nullptr;
        lmdb_throw_on_err(mdb_txn_begin(env, parent, flags, &h));
        _h.reset(h);
    }

    void begin(lmdb_env& env, unsigned flags = 0)
    {
        begin(env.handle(), nullptr, flags);
    }

    void begin(lmdb_txn_base& parent, unsigned flags = 0)
    {
        begin(parent.env_handle(), parent.handle(), flags);
    }

public:
    MDB_txn* handle() const { return _h.get(); }
    MDB_env* env_handle() { return mdb_txn_env(_h.get()); }

    explicit operator bool() const { return _h.operator bool(); }

    size_t id()
    {
        return mdb_txn_id(handle());
    }
};

class lmdb_rw_txn : public lmdb_txn_base
{
public:
    lmdb_rw_txn() = default;

    lmdb_rw_txn(lmdb_env& env, unsigned flags = 0)
    {
        begin(env, flags);
    }

    lmdb_rw_txn(lmdb_rw_txn& parent, unsigned flags = 0)
    {
        begin(parent, flags);
    }

    void begin(lmdb_env& env, unsigned flags = 0)
    {
        BOOST_ASSERT(! (flags & MDB_RDONLY));
        lmdb_txn_base::begin(env, flags);
    }

    void begin(lmdb_rw_txn& parent, unsigned flags = 0)
    {
        BOOST_ASSERT(! (flags & MDB_RDONLY));
        lmdb_txn_base::begin(parent, flags);
    }

    void commit()
    {
        lmdb_throw_on_err(mdb_txn_commit(handle()));
        _h.reset();
    }
};

// While any reader exists, writers cannot reuse space in the database file that has become unused in later versions.
// Due to this, continual use of long-lived read transactions may cause the database to grow without bound.
// 
//     lmdb_ro_txn txn(env, true); // constructed and initially reseted
//     // later when used
//     {
//         lmdb_ro_txn_scope txnScope(txn);
//         // make this scope as small as possible
//     }
class lmdb_ro_txn : public lmdb_txn_base
{
    bool _reseted = false;

public:
    lmdb_ro_txn() = default;

    lmdb_ro_txn(lmdb_env& env, bool reset_ = fales, unsigned flags = 0)
    {
        begin(env, reset_, flags);
    }

    lmdb_ro_txn(lmdb_txn_base& parent, bool reset_ = fales, unsigned flags = 0)
    {
        begin(parent, reset_, flags);
    }

    void begin(lmdb_env& env, bool reset_ = fales, unsigned flags = 0)
    {
        lmdb_txn_base::begin(env, flags | MDB_RDONLY);

        if(reset_)
            reset();
    }

    void begin(lmdb_txn_base& parent, bool reset_ = fales, unsigned flags = 0)
    {        
        lmdb_txn_base::begin(parent, flags | MDB_RDONLY);

        if(reset_)
            reset();
    }

    void reset()
    {
        BOOST_ASSERT(_h);
        mdb_txn_reset(handle());
        _reseted = true;
    }

    void renew()
    {
        if(_reseted)
            lmdb_throw_on_err(mdb_txn_renew(handle()));
    }
};

class lmdb_ro_txn_scope
{
    lmdb_ro_txn& _txn;

public:
    explicit lmdb_ro_txn_scope(lmdb_ro_txn& txn) : _txn(txn) { _txn.renew(); }

    ~lmdb_ro_txn_scope() { _txn.reset(); }

    MDB_txn* handle() { return _txn.handle(); }
};


inline void mdb_val_to(MDB_val const& v, MDB_val& t) noexcept { t = v; }

void mdb_val_to(MDB_val const& v, _str_ auto& t) { uassign(t, reinterpret_cast<char const*>(v.mv_data), v.mv_size); }

void to_mdb_val(MDB_val const& t, MDB_val& v) noexcept { v = t; }

void to_mdb_val(_str_ auto const& t, MDB_val& v) noexcept
{
    v.mv_data = str_data(t);
    v.mv_size = str_size(t);
}



// still usable after lmdb_ro_txn::reset/renew
class lmdb_db_base
{
protected:
    MDB_dbi  _h   = 0;
    MDB_txn* _txn = nullptr;

    lmdb_db_base() = default;

    void open(MDB_txn* txn, char const* name, unsigned flags)
    {
        lmdb_throw_on_err(mdb_dbi_open(txn, name, flags, &_h));
        _txn = txn;
    }

public:
    lmdb_db_base(lmdb_db_base const&) = delete;
    lmdb_db_base& operator=(lmdb_db_base const&) = delete;

    lmdb_db_base(lmdb_db_base&& t) { *this = std::move(t); }
    lmdb_db_base& operator=(lmdb_db_base&& t) { std::swap(_h, t._h); std::swap(_txn, t._txn); return *this; }

    MDB_dbi handle() const { return _h; }
    MDB_txn* txn_handle() const { return _txn; }

    explicit operator bool() const { return _h != 0; }

    MDB_stat stat()
    {
        MDB_stat t;
        lmdb_throw_on_err(mdb_stat(_txn, _h, &t));
        return t;
    }

    unsigned flags()
    {
        unsigned t;
        lmdb_throw_on_err(mdb_dbi_flags(_txn, _h, &t));
        return t;
    }

    void set_key_compare(MDB_cmp_func* cmp)
    {
        lmdb_throw_on_err(mdb_set_compare(_txn, _h, cmp));
    }

    void set_val_compare(MDB_cmp_func* cmp)
    {
        lmdb_throw_on_err(mdb_set_dupsort(_txn, _h, cmp));
    }

    template<class K, class V>
    bool get(K const& k, V& v)
    {
        MDB_val mk, mv;
        to_mdb_val(k, mk);
        int r = mdb_get(_txn, handle(), &mk, &mv);

        if(r == 0)
        {
            mdb_val_to(mv, k);
            return true;
        }

        if(r == MDB_NOTFOUND)
            return false;

        lmdb_throw_on_err(r);
        return false;
    }

    template<class V, class K>
    V get(K const& k, V const& def = V())
    {
        V v;
        if(! get(k, v))
            return def;
        return v;
    }
};


class lmdb_ro_db : public lmdb_db_base
{
public:
    lmdb_ro_db() = default;

    lmdb_ro_db(lmdb_ro_txn& txn, unsigned flags = 0)
    {
        open(txn, flags);
    }

    template<class Str>
    lmdb_ro_db(lmdb_ro_txn& txn, Str const& name, unsigned flags = 0)
    {
        open(txn, name, flags);
    }

    void open(lmdb_ro_txn& txn, unsigned flags = 0)
    {
        lmdb_db_base::open(txn.handle(), nullptr, flags);
    }

    template<class Str>
    void open(lmdb_ro_txn& txn, Str const& name, unsigned flags = 0)
    {
        lmdb_db_base::open(txn.handle(), as_cstr(name).data(), flags);
    }
};


class lmdb_rw_db : public lmdb_db_base
{
public:
    lmdb_rw_db() = default;

    lmdb_rw_db(lmdb_rw_txn& txn, unsigned flags = 0)
    {
        open(txn, flags);
    }

    template<class Str>
    lmdb_rw_db(lmdb_rw_txn& txn, Str const& name, unsigned flags = 0)
    {
        open(txn, name, flags);
    }

    void open(lmdb_rw_txn& txn, unsigned flags = 0)
    {
        lmdb_db_base::open(txn.handle(), nullptr, flags);
    }

    template<class Str>
    void open(lmdb_rw_txn& txn, Str const& name, unsigned flags = 0)
    {
        lmdb_db_base::open(txn.handle(), as_cstr(name).data(), flags);
    }

    void drop(bool del)
    {
        lmdb_throw_on_err(mdb_drop(_txn, _h, del ? 1 : 0));
        _h = 0;
    }

    template<class K>
    bool put(K const& k, MDB_val& mv, unsigned flags = 0)
    {
        MDB_val mk; to_mdb_val(k, mk);

        int r = mdb_put(_txn, _h, &mk, &mv, flags);

        if(r == 0)
            return true;

        if(r == MDB_KEYEXIST)
            return false;

        lmdb_throw_on_err(r);
        return false;
    }

    template<class K, class V>
    bool put(K const& k, V const& v, unsigned flags = 0)
    {
        MDB_val mv; to_mdb_val(v, mv);
        return put(k, mv, flags);
    }

    template<class K>
    bool del(K const& k, MDB_val* mv = nullptr)
    {
        MDB_val mk;
        to_mdb_val(k, mk);
        int r = mdb_del(_txn, _h, &mk, mv);

        if(r == 0)
            return true;

        if(r == MDB_NOTFOUND)
            return false;

        lmdb_throw_on_err(r);
        return false;
    }

    template<class K, class V>
    bool del(K const& k, V const& v)
    {
        MDB_val mv;
        to_mdb_val(v, mv);
        return del(k, &mv);
    }
};






template<class T>
struct lmdb_compare
{
    static int compare(MDB_val const* a, MDB_val const* b)
    {
        if constexpr(_str_<T>)
        {
            using ch_t   = str_value_t<T>;
            using traits = std::char_traits<ch_t>;

            auto const r = traits::compare(reinterpret_cast<ch_t const*>(a->mv_data),
                                           reinterpret_cast<ch_t const*>(b->mv_data),
                                           std::min(a->mv_size, b->mv_size));

            if(r != 0)
                return r;

            if(a->mv_size < b->mv_size)
                return -1;

            if(a->mv_size > b->mv_size)
                return 1;

            return 0;
        }
        else
        {
            T ta; mdb_val_to(*a, ta);
            T tb; mdb_val_to(*b, tb);

            if(ta < tb)
                return -1;
            if(ta > tb)
                return 1;
            return 0;
        }
    }
};


template<
    class Key, class Val,
    bool  Dup = false,
    class KeyCompare = lmdb_compare<Key>,
    class ValCompare = lmdb_compare<Val>,
>
class lmdb_map : public lmdb_rw_db
{
public:
    template<class Txn>
    lmdb_map(Txn& txn, unsigned flags = 0)
        : lmdb_db_base(txn, flags | (Dup ? MDB_DUPSORT: 0))
    {
        set_key_compare(KeyCompare::compare);
        set_val_compare(ValCompare::compare);
    }

    template<class Txn, class Str>
    lmdb_map(Txn& txn, Str const& name, unsigned flags = 0)
        : lmdb_db_base(txn, flags | (Dup ? MDB_DUPSORT: 0))
    {
        set_key_compare(KeyCompare::compare);
        set_val_compare(ValCompare::compare);
    }
    
    bool get(Key const& k, Val& v)
    {
        return lmdb_db_base::get(k, v);
    }

    Val get(Key const& k, Val const& def = Val())
    {
        Val v;
        if(! get(k, v))
            return def;
        return v;
    }

    bool put(Key const& k, Val const& v, unsigned flags = 0)
    {
        return lmdb_db_base::put(k, v, flags);
    }

    bool del(Key const& k)
    {
        return lmdb_db_base::del(k);
    }

    bool del(Key const& k, Val const& v)
    {
        return lmdb_db_base::del(k, v);
    }
};


class lmdb_cursor
{
protected:
    struct deleter
    {
        void operator(MDB_cursor* h)
        {
            mdb_cursor_close(h);
        }
    };

    std::unique_ptr<MDB_cursor> _h;

    lmdb_cursor() = default;

    void open(lmdb_db_base& db)
    {
        MDB_cursor* h = nullptr;
        lmdb_throw_on_err(mdb_cursor_open(db.txn_handle(), db.handle(), &h));
        _h.reset(h);
    }

public:
    MDB_cursor* handle() const { return _h.get(); }
    MDB_cursor* txn_handle() const { return mdb_cursor_txn(handle()); }
};


class lmdb_ro_cursor : public lmdb_cursor
{
public:
    lmdb_ro_cursor(lmdb_ro_db& db) { open(db); }

    // after lmdb_ro_txn.renew()ed, cursors from it must also be renewed
    void renew()
    {
        lmdb_throw_on_err(mdb_cursor_renew(txn_handle(), handle()));
    }
};


}   // namespace jkl