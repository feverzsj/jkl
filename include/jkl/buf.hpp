#pragma once

#include <jkl/config.hpp>
#include <jkl/util/buf.hpp>
#include <boost/asio/buffer.hpp>


namespace jkl{


template<class T> concept _mutable_bufseq_ = asio::is_mutable_buffer_sequence<std::remove_cv_t<T>>::value;
template<class T> concept _const_bufseq_ = asio::is_const_buffer_sequence<std::remove_cv_t<T>>::value;
template<class T> concept _bufseq_ = _mutable_bufseq_<T> || _const_bufseq_<T>;

template<class T> concept _dynabuf_v1_ = asio::is_dynamic_buffer_v1<std::remove_cv_t<T>>::value;
template<class T> concept _dynabuf_v2_ = asio::is_dynamic_buffer_v2<std::remove_cv_t<T>>::value;

template<class T> concept _dynabuf_v1_or_v2_ = _dynabuf_v1_<T> || _dynabuf_v2_<T>;



auto asio_buf(_byte_buf_ auto& b) noexcept
{
    return asio::buffer(buf_data(b), buf_size(b));
}

auto buy_asio_buf(_resizable_buf_ auto& b, size_t n) noexcept
{
    return asio::buffer(buy_buf(b, n), n);
}


// a buffer wrapper that satisfies asio DynamicBuffer_v1 and DynamicBuffer_v2
template<_resizable_byte_buf_ Buf>
class dynabuf
{
    Buf& _b;
    size_t const _ms;
#ifndef BOOST_ASIO_NO_DYNAMIC_BUFFER_V1
    size_t _s;
#endif

public:
    typedef asio::const_buffer const_buffers_type;

    typedef asio::mutable_buffer mutable_buffers_type;

    explicit dynabuf(Buf& b, size_t maxSize = SIZE_MAX) noexcept
        : _b(b), _ms(maxSize)
    #ifndef BOOST_ASIO_NO_DYNAMIC_BUFFER_V1
        ,_s(SIZE_MAX)
    #endif
    {}


    // DynamicBuffer_v1: size of input sequence.
    // DynamicBuffer_v2: size of underlying buf if less than max_size(), otherwise max_size().
    size_t size() const noexcept
    {
    #ifndef BOOST_ASIO_NO_DYNAMIC_BUFFER_V1
        if(_s != SIZE_MAX)
            return _s;
    #endif
        return (std::min)(_b.size(), max_size());
    }

    // DynamicBuffer_v1: the allowed maximum of the sum of the sizes of the input sequence and output sequence.
    // DynamicBuffer_v2: the allowed maximum size of the underlying memory.
    size_t max_size() const noexcept
    {
        return _ms;
    }

    // Get the maximum size that the buffer may grow to without triggering
    // reallocation.
    // DynamicBuffer_v1: the current total capacity of the buffer, i.e. for both the input sequence and output sequence.
    // DynamicBuffer_v2: the current capacity of the underlying vector if less than max_size().
    // Otherwise returns max_size().
    size_t capacity() const noexcept
    {
        return (std::min)(_b.capacity(), max_size());
    }

#ifndef BOOST_ASIO_NO_DYNAMIC_BUFFER_V1
    // DynamicBuffer_v1: Get a list of buffers that represents the input sequence.
    const_buffers_type data() const noexcept
    {
        return const_buffers_type(asio::buffer(_b, _s));
    }
#endif

    // DynamicBuffer_v2: get a sequence of buffers that represents the underlying memory at pos and max size n.
    mutable_buffers_type data(size_t pos, size_t n) noexcept
    {
        return {asio::buffer(asio::buffer(_b, _ms) + pos, n)};
    }

    // DynamicBuffer_v2: get a sequence of buffers that represents the underlying memory at pos and max size n.
    const_buffers_type data(size_t pos, size_t n) const noexcept
    {
        return {asio::buffer(asio::buffer(_b, _ms) + pos, n)};
    }

#ifndef BOOST_ASIO_NO_DYNAMIC_BUFFER_V1
    // DynamicBuffer_v1: get a list of buffers that represents the output sequence, with the given size.
    mutable_buffers_type prepare(size_t n)
    {
        if(size () > max_size() || max_size() - size() < n)
            throw std::length_error("dynabuf too long");

        if(_s == SIZE_MAX)
            _s = _b.size(); // Enable v1 behavior.

        _b.resize(_s + n);

        return asio::buffer(asio::buffer(_b) + _s, n);
    }

    // DynamicBuffer_v1: move bytes from the output sequence to the input sequence.
    void commit(size_t n)
    {
        _s += (std::min)(n, _b.size() - _s);
        _b.resize(_s);
    }
#endif

    // DynamicBuffer_v2: grow the underlying memory by the specified number of bytes.
    void grow(size_t n)
    {
        if(size() > max_size() || max_size() - size() < n)
            throw std::length_error("dynabuf too long");
        _b.resize(size() + n);
    }

    // DynamicBuffer_v2: Shrink the underlying memory by the specified number of bytes.
    void shrink(size_t n)
    {
        _b.resize(n > size() ? 0 : size() - n);
    }

    // DynamicBuffer_v1: remove n characters from the beginning of input sequence.
    //                   If n is greater than the size of the input sequence, the
    //                   entire input sequence is consumed and no error is issued
    // DynamicBuffer_v2: erases n bytes from the beginning of the vector.
    //                   If n is greater than the current size of the vector, the vector is emptied.
    void consume(size_t n)
    {
    #ifndef BOOST_ASIO_NO_DYNAMIC_BUFFER_V1
        if(_s != SIZE_MAX)
        {
            size_t consume_length = (std::min)(n, _s);
            _b.erase(_b.begin(), _b.begin() + consume_length);
            _s -= consume_length;
            return;
        }
    #endif
        _b.erase(_b.begin(), _b.begin() + (std::min)(size(), n));
    }
};


} // namespace jkl