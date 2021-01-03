#pragma once

#include <jkl/config.hpp>
#include <jkl/result.hpp>
#include <jkl/util/buf.hpp>
#include <jkl/util/str.hpp>
#include <boost/beast/core/file.hpp>
#include <boost/beast/core/file_stdio.hpp>


namespace jkl{


using file_mode = beast::file_mode;


template<class Impl>
class synced_file_wrapper : public Impl
{
public:
    explicit operator bool() const noexcept { return Impl::is_open(); }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    aresult<> open(_char_str_ auto const& path, file_mode mode)
    {
        aerror_code ec;
        Impl::open(as_cstr(path).data(), mode, ec);
        return ec;
    }

    aresult<> close()
    {
        aerror_code ec;
        Impl::close(ec);
        return ec;
    }

    aresult<std::uint64_t> size() const
    {
        aerror_code ec;
        Impl::size(ec);
        return ec;
    }

    aresult<std::uint64_t> pos() const
    {
        aerror_code ec;
        Impl::pos(ec);
        return ec;
    }

    aresult<> seek(std::uint64_t offset)
    {
        aerror_code ec;
        Impl::seek(offset, ec);
        return ec;
    }

    // read at most n _trivially_copyable_ object to d.
    // returns the count of object actaully read; if this number differs from maxCnt,
    // either a reading error occurred or the end-of-file was reached.
    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    aresult<size_t> read(_trivially_copyable_ auto* d, size_t n) const
    {
        aerror_code ec;
        size_t nByte = Impl::read(d, n * sizeof(d), ec);
        if(! ec)
            return nByte / sizeof(d);
        return ec;
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    aresult<size_t> read(_trivially_copyable_buf_ auto&& d) const
    {
        JKL_TRY(size_t n, read(buf_data(d), buf_size(d)));

        if constexpr(_resizable_buf_<decltype(d)>)
            resize_buf(d, n);

        return n;
    }

    // write n _trivially_copyable_ object from d to file.
    // returns the count of object successfully written.
    // If this number differs from n, a writing error prevented the function from completing.
    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    aresult<size_t> write(_trivially_copyable_ auto* d, size_t n)
    {
        aerror_code ec;
        size_t nByte = Impl::write(d, n * sizeof(d), ec);
        if(! ec)
            return nByte / sizeof(d);
        return ec;
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    aresult<size_t> write(_trivially_copyable_buf_ auto const& d)
    {
        return write(buf_data(d), buf_size(d));
    }

    _JKL_MSVC_WORKAROUND_TEMPL_FUN_ABBR
    aresult<size_t> read_left_bytes(_resizable_byte_buf_ auto& d)
    {
        JKL_TRY(size_t n, size());
        rescale_buf(d, n);
        return read(d);
    }
};


using synced_file       = synced_file_wrapper<beast::file>;
using synced_stdio_file = synced_file_wrapper<beast::file_stdio>;


} // namespace jkl
