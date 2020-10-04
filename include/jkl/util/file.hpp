#pragma once

#include <jkl/util/buf.hpp>
#include <jkl/util/str.hpp>
#include <cstdio>


namespace jkl{


class stdio_file
{
    FILE* _io = nullptr;

public:
    using size_type = size_t;
    using pos_type  = long;
    using off_type  = long;

    stdio_file() = default;
    ~stdio_file() { close(); }

    stdio_file(stdio_file const&) = delete;
    stdio_file& operator=(stdio_file const&) = delete;

    stdio_file(stdio_file&& f) { std::swap(_io, f._io); }
    stdio_file& operator=(stdio_file&& f) { std::swap(_io, f._io); }

    stdio_file(_char_str_ auto const& path, _char_str_ auto const& mode)
    {
        open(path, mode);
    }

    explicit operator bool() const { return _io; }
    FILE* handle() const { return _io; }

    // http://en.cppreference.com/w/cpp/io/c/fopen
    // NOTE: try not using text mode, it's not portable.
    bool open(_char_str_ auto const& path, _char_str_ auto const& mode)
    {
        close();
        _io = fopen(as_cstr(path).data(), mode);
        return _io != nullptr;
    }

    // fclose may fail when the pending operations fails, e.g. fflush, but the stream is closed anyway,
    // so one should check fflush before close
    bool close()
    {
        if(_io)
        {
            int r = fclose(_io);
            _io = nullptr;
            return r == 0;
        }

        return true;
    }

    FILE* handle() { return _io; }

    // error indicator can be cleared by clearerr, rewind or freopen.
    bool error() const
    {
        BOOST_ASSERT(_io);
        return ferror(_io) != 0;
    }

    void clear_error()
    {
        BOOST_ASSERT(_io);
        clearerr(_io);
    }

    // *DO NOT* use this as bounds check for read loop.
    // see: http://en.cppreference.com/w/cpp/io/c/feof
    bool eof() const
    {
        BOOST_ASSERT(_io);
        return feof(_io) != 0;
    }

    bool get_pos(pos_type& pos) const
    {
        BOOST_ASSERT(_io);

        pos_type t = ftell(_io);

        if(-1L == t)
            return false;

        pos = t;
        return true;
    }

    bool seek(off_type offset, int origin = SEEK_SET)
    {
        BOOST_ASSERT(_io);
        return fseek(_io, offset, origin) == 0;
    }

    bool seek_cur(off_type offset)
    {
        return seek(offset, SEEK_CUR);
    }

    bool seek_end(off_type offset)
    {
        return seek(offset, SEEK_END);
    }

    bool rewind()
    {
        BOOST_ASSERT(_io);
        ::rewind(_io);
        return true;
    }

    bool flush()
    {
        BOOST_ASSERT(_io);
        return fflush(_io) == 0;
    }

    bool put(char c)
    {
        // http://en.cppreference.com/w/cpp/io/c/fputc
        return fputc((unsigned char)c, _io) != EOF;
    }

    bool get(char& c)
    {
        int r = fgetc(_io);
        if(r == EOF)
            return false;
        (unsigned char&)c = r;
        return true;
    }

    // write n object T from d to stream.
    // returns the count of object T successfully written.
    // If this number differs from the n, a writing error prevented the function from completing.
    // In this case, the error indicator (ferror) will be set for the stream.
    size_type write(_trivially_copyable_ auto const* d, size_type n)
    {
        BOOST_ASSERT(_io);
        return fwrite(d, sizeof(*d), n, _io);
    }

    size_type write(_buf_ auto const& d)
    {
        return write(buf_data(d), buf_size(d));
    }


    // read at most maxCnt object T to d.
    // returns the count of object T actaully read; if this number differs from maxCnt,
    // either a reading error occurred or the end-of-file was reached.
    size_type read(_trivially_copyable_ auto* d, size_type maxCnt)
    {
        BOOST_ASSERT(_io);
        return fread(d, sizeof(*d), maxCnt, _io);
    }

    size_type read(_buf_ auto& d)
    {
        auto n = read(buf_data(d), buf_size(d));
        resize_buf(d, n);
        return n;
    }

    size_type read(_buf_ auto& d, size_type maxCnt)
    {
        rescale_buf(d, maxCnt);
        return read(d);
    }

    size_type append_read(_buf_ auto& d, size_type maxCnt)
    {
        size_type s = buf_size(d);
        resize_buf(d, s +- maxCnt);
        auto n = read(buf_data(d) +- s, maxCnt);
        resize_buf(d, s +- n);
        return n;
    }

    // get size of available bytes to read,
    // the stream must be opened in binary mode
    bool get_bytes_available(size_type& s)
    {
        pos_type beg, end;
        return get_pos(beg) && seek_end(0) && get_pos(end) && cvt_num(end - beg, s) && seek(beg);
    }

    // read all available bytes of stream
    // the stream must be opened in binary mode
    bool read_all_bytes(_byte_buf_ auto& d)
    {
        static_assert(sizeof(buf_value(d)) == 1);
        size_type s;
        return get_bytes_available(s) && read(d, s) == s;
    }

    template<size_t N>
    char* read_line(char (&buf)[N])
    {
        return fgets(buf, N, _io);
    }
};


stdio_file open_ro_bin_file(_char_str_ auto const& path)
{
    return {path, "rb"};
}

bool read_whole_file(_char_str_ auto const& path, _byte_buf_ auto& buf)
{
    auto f = open_ro_bin_file(path);
    return f && f.read_all_bytes(buf);
}

template<class Buf = string>
std::pair<Buf, bool> read_whole_file(_char_str_ auto const& path)
{
    Buf buf;
    bool r = read_whole_file(path, buf);
    return {buf, r};
}


} // namespace jkl
