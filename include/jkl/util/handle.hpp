#pragma once


#include <jkl/config.hpp>
#include <jkl/util/concepts.hpp>
#include <utility>
#include <functional>
// #if __has_include(<compare>)
// #include <compare>
// #endif


namespace jkl{


struct no_copier{};


// like std::unique_ptr but allow any handle like type
// and also copyable if provide a copier.
template<
    _pod_ Handle,
    class Deleter,
    class Copier,
    auto  Null = Handle{}
>
class handle_t
{
    Handle _h{Null};
    [[no_unique_address]] Deleter _del{};
    [[no_unique_address]] Copier  _cpy{};

public:
    using hanlde_type  = Handle;
    using deleter_type = Deleter;
    using copier_type  = Copier;

    constexpr handle_t() noexcept requires(! _pointer_<Deleter> && ! _pointer_<Copier>) = default;

    constexpr handle_t(Handle h) noexcept requires(! _pointer_<Deleter> && ! _pointer_<Copier>)
        : _h{h}
    {}

    constexpr handle_t(Handle h, Deleter d) noexcept requires(! _pointer_<Copier>)
        : _h{h}, _del{d}
    {}

    constexpr handle_t(Handle h, Copier c) noexcept requires(! _pointer_<Deleter>)
        : _h{h}, _cpy{c}
    {}

    constexpr handle_t(Handle h, Deleter d, Copier c) noexcept
        : _h{h}, _del{d}, _cpy{c}
    {}

    ~handle_t() noexcept
    {
        if(valid())
            _del(_h);
    }

    handle_t(handle_t const& r) noexcept(noexcept(_cpy(_h))) requires(! std::same_as<Copier, no_copier>)
        : _h{r ? r._cpy(r._h) : r._h}, _del{r._del}, _cpy{r._cpy}
    {}

    handle_t& operator=(handle_t const& r) noexcept(noexcept(_cpy(_h))) requires(! std::same_as<Copier, no_copier>)
    {
        if(this != std::addressof(r))
        {
            _h   = (r ? r._cpy(_h) : r.h);
            _del =  r._del;
            _cpy =  r._cpy;
        }
        return *this;
    }

    handle_t(handle_t&& r) noexcept
        : _h{r.release()}, _del{std::move(r._del)}, _cpy{std::move(r._cpy)}
    {}

    handle_t& operator=(handle_t&& r) noexcept
    {
        if(this != std::addressof(r))
        {
            reset(r.release());
            _del = std::move(r._del);
            _cpy = std::move(r._cpy);
        }
        return *this;
    }

    bool valid() const noexcept { return _h != Null; }
    explicit operator bool() const noexcept { return valid(); }

    auto* operator->() noexcept
    {
        if constexpr(std::is_pointer_v<Handle>)
            return _h;
        else
            return &_h;
    }

    auto* operator->() const noexcept
    {
        if constexpr(std::is_pointer_v<Handle>)
            return _h;
        else
            return &_h;
    }

    auto& operator*() noexcept
    {
        if constexpr(std::is_pointer_v<Handle>)
            return *_h;
        else
            return _h;
    }

    auto& operator*() const noexcept
    {
        if constexpr(std::is_pointer_v<Handle>)
            return *_h;
        else
            return _h;
    }

    auto        get        () const noexcept { return _h  ; }
    auto const& get_deleter() const noexcept { return _del; }
    auto const& get_copier () const noexcept { return _cpy; }
    auto      & get_deleter()       noexcept { return _del; }
    auto      & get_copier ()       noexcept { return _cpy; }

    void swap(handle_t& r) noexcept
    {
        std::swap(_h  , r._h  );
        std::swap(_del, r._del);
        std::swap(_cpy, r._cpy);
    }
    void reset(Handle h = Null) noexcept
    {
        if(valid())
            _del(_h);
        _h = h;
    }

    [[nodiscard]] Handle release() noexcept { return std::exchange(_h, Null); }

// #if __has_include(<compare>)
//     auto operator<=>(handle_t const&) const = default;
// #endif
};


template<class Handle, class Deleter, auto Null = Handle{}>
using unique_handle = handle_t<Handle, Deleter, no_copier, Null>;

template<class Handle, class Deleter, class Copier, auto Null = Handle{}>
using copyable_handle = handle_t<Handle, Deleter, Copier, Null>;


} // namespace jkl


namespace std{

template<class H, class D, class C, auto N>
struct hash<jkl::handle_t<H, D, C, N>>
{
    auto operator()(jkl::handle_t<H, D, C, N> const& h) const noexcept
    {
        return hash<H>()(h.get());
    }
};

} // namespace std
