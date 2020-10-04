#pragma once

#include <jkl/sqlite3/conn.hpp>


namespace jkl{


template<class Derived>
class fts5_tokenizer_base
{
protected:
    static Derived* do_create(char const** azArg, int nArg)
    {
        return new Derived(azArg, nArg);
    }

    static void do_delete(void* d)
    {
        delete reinterpret_cast<Derived*>(d);
    }

    static int xCreate(void*, char const** azArg, int nArg, Fts5Tokenizer** ppOut)
    {
        if(auto d = Derived::do_create(azArg, nArg))
        {
            *ppOut = reinterpret_cast<Fts5Tokenizer*>(d);
            return SQLITE_OK;
        }
        return SQLITE_ERROR;
    }

    static void xDelete(Fts5Tokenizer* d)
    {
        Derived::do_delete(d);
    }

    static int xTokenize(Fts5Tokenizer* d, void* pCtx, int flags, char const* pText, int nText, 
                         int (*xToken)(void*, int, char const*, int, int, int))
    {
        return reinterpret_cast<Derived*>(d)->
            do_tokenize(flags, pText, nText,
                [pCtx, flags, xToken](auto const& token, auto start, auto end){
                    return xToken(pCtx, flags, str_data(token), static_cast<int>(str_size(token)),
                                  static_cast<int>(start), static_cast<int>(end));
                }
            );
    }

public:
    static void register_on(sqlite3_conn& c)
    {
        fts5_api* api = c.fts5();
        fts5_tokenizer t{xCreate, xDelete, xTokenize};
        sqlite3_throw_on_err(api->xCreateTokenizer(api, Derived::name(), api, &t, nullptr));
    }
};


} // namespace std