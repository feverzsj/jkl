#pragma once

#include <jkl/sqlite3/tokenizer/fts5.hpp>

#include <cppjieba/QuerySegment.hpp>


namespace jkl{


class jieba_global_t
{
    inline static std::unique_ptr<cppjieba::QuerySegment> _t;

public:
    template<class Str1, class Str2>
    static void init(Str1 const& dict, Str2 const& model, std::string const& userDict = "")
    {
        _t.reset(new cppjieba::QuerySegment(dict, model, userDict));
    }

    static cppjieba::QuerySegment* get()
    {
        BOOST_ASSERT(_t);
        return _t.get();
    }
};

constexpr jieba_global_t jieba_global;



class jieba_fts5_tokenizer : public fts5_tokenizer_base<jieba_fts5_tokenizer>
{
protected:
    friend class fts5_tokenizer_base<jieba_fts5_tokenizer>;

    static void* do_create(char const**, int)
    {
        return reinterpret_cast<void*>(126126);
    }

    static void do_delete(void* d)
    {
        BOOST_ASSERT(d == reinterpret_cast<void*>(126126));
    }

    template<class F>
    int do_tokenize(int flags, char const* pText, int nText, F&& onToken)
    {
        std::vector<cppjieba::Word> tokens;
        
        jieba_global.get()->Cut(std::string(pText, nText), tokens);
        
        for(auto const& token : tokens)
        {
            int r = onToken(token.word, token.offset, token.offset + token.word.size());

            if(r != SQLITE_OK)
                return r;
        }

        return SQLITE_OK;
    }

public:
    static constexpr char const* name() { return "jieba"; }
};


} // namespace std