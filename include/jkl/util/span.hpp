#pragma once

#include <jkl/config.hpp>
#include <jkl/util/buf.hpp>


namespace jkl{


template<class T, size_t Extent>
class bspan_base
{
    T* _d;

public:
    explicit constexpr bspan_base(T* d) noexcept : _d(d) { BOOST_ASSERT(d); }

    explicit constexpr bspan_base(_buf_ auto const& b, size_t offset = 0) noexcept
        : _d(buf_data(b) + offset)
    {
        BOOST_ASSERT(offset + size() <= buf_size(b));
    }

    constexpr T* data() const noexcept { return _d; }
    static constexpr size_t size() noexcept { return Extent; }
};


template<class T>
class bspan_base<T, size_t(-1)>
{
    T*           _d;
    size_t const _n;

public:
    explicit constexpr bspan_base(T* d, size_t n) noexcept : _d(d), _n(n) { BOOST_ASSERT(d); }

    explicit constexpr bspan_base(_buf_ auto const& b, size_t offset = 0, size_t len = -1) noexcept
        : _d(buf_data(b) + offset), _n((len == -1) ? (buf_size(b) - offset) : len)
    {
        BOOST_ASSERT(offset + size() <= buf_size(b));
    }

    constexpr T*     data() const noexcept { return _d; }
    constexpr size_t size() const noexcept { return _n; }
};


template<class T, size_t Extent = size_t(-1)>
class bspan : public bspan_base<T, Extent>
{
    using base = bspan_base<T, Extent>;

public:
    static constexpr std::size_t extent = Extent;

    using value_type   = T;
    using element_type = T;
    using size_type    = size_t;

    using base::base;

    constexpr size_t size_bytes() const noexcept { return this->size() * sizeof(T); }
    constexpr bool   empty     () const noexcept { return this->size() == 0; }

    constexpr T* begin() const noexcept { return this->data(); }
    constexpr T* end  () const noexcept { return this->data() + this->size(); }

    constexpr T& front() const noexcept { static_assert(Extent); BOOST_ASSERT(this->size()); return *begin(); }
    constexpr T& back () const noexcept { static_assert(Extent); BOOST_ASSERT(this->size()); return *(end() - 1); }

    constexpr T& operator[](size_t i) const noexcept { static_assert(Extent); BOOST_ASSERT(i < this->size()); return begin()[i]; }

    template<size_t M>
    constexpr bspan<T, M> first() const noexcept
    {
        static_assert(M <= Extent);
        BOOST_ASSERT(M <= this->size());
        return bspan<T, M>(begin());
    }

    constexpr bspan<T> first(size_t m) const noexcept
    {
        BOOST_ASSERT(m <= Extent);
        return bspan<T>(begin(), m);
    }

    template<size_t M>
    constexpr bspan<T, M> last() const noexcept
    {
        static_assert(M <= Extent);
        BOOST_ASSERT(M <= this->size());
        return bspan<T, M>(end() - M);
    }

    constexpr bspan<T> last(size_t m) const noexcept
    {
        BOOST_ASSERT(m <= this->size());
        return bspan<T>(end() - m, m);
    }

    template<size_t Offset, size_t M = size_t(-1)>
    constexpr auto subspan() const noexcept
    {
        if constexpr(Extent == size_t(-1))
        {
            static_assert(Offset < this->size());

            constexpr auto left = this->size() - Offset;

            static_assert(M == -1 || M <= left);

            return bspan<T, M == -1 ? left : M>(begin() + Offset);
        }
        else
        {
            return subspan(Offset, M);
        }
    }

    constexpr bspan<T> subspan(size_t offset, size_t m = -1) const noexcept
    {
        BOOST_ASSERT(offset < this->size());

        auto left = this->size() - offset;

        BOOST_ASSERT(m == -1 || m <= left);

        return bspan<T>(begin() + offset, m == -1 ? left : m);
    }
};


} // namespace jkl
