#pragma once

#include <jkl/config.hpp>
#include <jkl/buf.hpp>
#include <jkl/traits.hpp>
#include <jkl/ec_awaiter.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>


namespace jkl{



auto read_some(auto& s, _mutable_bufseq_ auto const& b, auto... p)
{
    return make_ec_awaiter<size_t>(
        [&, b](auto&& h){ get_astream(s).async_read_some(b, std::move(h)); },
        p...);
}

template<_byte_buf_ B>
auto read_some(auto& s, B& b, auto... p)
{
    if constexpr(_resizable_buf_<B>)
    {
        return make_ec_awaiter_c<size_t>(
            [&](auto&& h){ get_astream(s).async_read_some(asio_buf(b), std::move(h)); },
            [&](size_t n){ resize_buf(b, n); },
            p...);
    }
    else
    {
        return make_ec_awaiter<size_t>(
            [&](auto&& h){ get_astream(s).async_read_some(b, asio_buf(b), std::move(h)); },
            p...);
    }
}


auto read_some_to_tail(auto& s, _resizable_byte_buf_ auto& b, size_t maxSize, auto... p)
{
    return make_ec_awaiter_c<size_t>(
        [&, maxSize](auto&& h){ get_astream(s).async_read_some(buy_asio_buf(b, maxSize), std::move(h)); },
        [&, maxSize](size_t n){ resize_buf(b, buf_size(b) - maxSize + n); },
        p...);
}


auto write_some(auto& s, _const_bufseq_ auto const& b, auto... p)
{
    return make_ec_awaiter<size_t>(
        [&, b](auto&& h){ get_astream(s).async_write_some(b, std::move(h)); },
        p...);
}

auto write_some(auto& s, _byte_buf_ auto& b, auto... p)
{
    return make_ec_awaiter<size_t>(
        [&](auto&& h){ get_astream(s).async_write_some(asio_buf(b), std::move(h)); },
        p...);
}


auto write_some_from(auto& s, _byte_buf_ auto& b, size_t pos, auto... p)
{                                     // ^^^^^ not using auto const&, since const& also passes in prvalue, while we want ref here.    
    BOOST_ASSERT(buf_size(b) > pos);

    return make_ec_awaiter_c<size_t>(
        [&, pos](auto&& h){ get_astream(s).async_write_some(asio::buffer(buf_data(b) + pos, buf_size(b) - pos), std::move(h)); },
        p...);
}


auto read(auto& s, _mutable_bufseq_ auto const& b, auto&& compCond, auto... p)
{
    return make_ec_awaiter<size_t>(
        [&, b, compCond = std::forward<decltype(compCond)>(compCond)](auto&& h){
            asio::async_read(get_astream(s), b, compCond, std::move(h));
        },
        p...);
}

auto read(auto& s, _byte_buf_ auto& b, auto&& compCond, auto... p)
{
    return make_ec_awaiter<size_t>(
        [&, compCond = std::forward<decltype(compCond)](auto&& h){
            asio::async_read(get_astream(s), asio_buf(b), compCond, std::move(h));
        },
        p...);
}

auto read_to_fill(auto& s, auto&& b, auto... p)
{
    return read(s, std::forward<decltype(b)>(b), asio::transfer_all(), p...);
}

auto read_to_tail(auto& s, _resizable_byte_buf_ auto& b, size_t size, auto... p)
{
    return make_ec_awaiter_c<size_t>(
        [&, size](auto&& h){ asio::async_read(get_astream(s), asio::buffer(buy_buf(b, size), size), std::move(h)); },
        [&, size](size_t n){ resize_buf(b, buf_size() - size + n); },
        p...);
}


// the returned size is up to and including the matched sequence,
// but the actual read size may pass the matched sequence in the buf, as it won't put back what has been read.
// Match can be a char, a string_view or a MatchCondition, which will be copied in.

auto read_until(auto& s, _dynabuf_v1_or_v2_ auto const& b, auto&& match, auto... p)
{
    return make_ec_awaiter<size_t>(
        [&, b, match = std::forward<decltype(match)>(match)](auto&& h){
            asio::async_read_until(get_astream(s), b, match, std::move(h));
        },
        p...);
}

auto read_until(auto& s, _resizable_byte_buf auto& b, auto&& match, auto... p)
{
    return read_until(s, dynabuf(b), std::forward<decltype(match)>(match), p...);
}


} // namespace jkl