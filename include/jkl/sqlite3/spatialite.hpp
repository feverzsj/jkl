#pragma once

#include <jkl/sqlite3/conn.hpp>

#include <spatialite.h>


namespace jkl{


class spatialite_conn : public sqlite3_conn
{
    struct deleter
    {
        void operator()(void* ctx)
        {
            spatialite_cleanup_ex(ctx);
        }
    };
    std::unique_ptr<void, deleter> _ctx;

public:
    spatialite_conn() = default;

    template<class Str>
    explicit spatialite_conn(Str const& path,
                             sqlite3_flags_t flags = sqlite3_flags.rwc(),
                             char const* vfs = nullptr)
    {
        open(path, flags, vfs);
    }

    template<class Str>
    void open(Str const& path, sqlite3_flags_t flags = sqlite3_flags.rwc(), const char* vfs = nullptr)
    {
        _ctx.reset(spatialite_alloc_connection());
        if(! _ctx)
            throw sqlite3_err("spatialite_alloc_connection() failed");
        sqlite3_conn::open(path, flags, vfs);
        spatialite_init_ex(sqlite3_conn::handle(), _ctx.get(), 0);
    }

    void close()
    {
        _ctx.reset();
        sqlite3_conn::close();
    }

    // http://www.gaia-gis.it/gaia-sins/spatialite-sql-latest.html#p16
    // 
    // srs: spatial reference system, only corresponding ESPG SRID definition will be inserted into the spatial_ref_sys table
    //      WGS84/WGS84_ONLY
    //      NONE/EMPTY : no ESPG SRID definition will be inserted into the spatial_ref_sys table
    //      
    // NOTE: if meta table already exits, this method will do nothing and return false,
    //       even if the 'srs' is different.
    template<class Str>
    bool init_spatial_meta_data(Str const& srs, bool singleTransaction = true)
    {
        return exec<bool>("SELECT InitSpatialMetaData(?, ?)", singleTransaction, srs);
    }

    // all possible ESPG SRID definitions will be inserted into the spatial_ref_sys table
    bool init_spatial_meta_data(bool singleTransaction = true)
    {
        return exec<bool>("SELECT InitSpatialMetaData(?)", singleTransaction);
    }
};


} // namespace std