#pragma once


#include <jkdf/sery/varint.hpp>
#include <jkdf/pbf/type.hpp>
#include <jkdf/pbf/exception.hpp>


namespace jkdf{


// we force the optimized read/write for protobuf:
//   the field def must ordered ascend as filed number in .proto file and code;
//   read and write must follow the exact same order;
//   update can only append new fields;
//   if integer field can be negative, it must use sintXX. For this reason, pbf_intXX are removed;
//   for tag(id, wireType) and length delimiter, we choose the underlying type to be uint32_t, so its max size on wire is 5 bytes.

class pbf_reader
{
    using value_type = int8_t; // for read_single_value() pbf_len_dlm

    int8_t const* _beg;
    int8_t const* _end;
    uint32_t _id, _wireType;

    auto left() const
    {
        return make_buf(_beg, _end - _beg);
    }

    void use(size_t n)
    {
        _beg += n;
    }

    template<class T>
    T get_varint()
    {
        T v;

        auto [used, ok] = read_varint(left(), v);
        
        use(used);

        if(BOOST_UNLIKELY(! ok))
            throw pbf_exception{"failed to read varint"};

        return v;
    }

    void read_tag()
    {
        uint32_t tag;

        auto [used, ok] = read_varint(left(), tag);
        
        use(used);

        if(ok)
        {
            _id       = tag >> 3u;
            _wireType = tag & 0x07u;
        }
        else
        {
            _id = 0; // if(_id == 0) failed to read tag
        }
    }

    template<class Ptype>
    Ptype::type read_single_value_and_tag()
    {
        typename Ptype::type v = read_single_value();
        read_tag();
        return v;
    }

    template<class Ptype>
    Ptype::type read_single_value()
    {
        using type = typename Ptype::type;

        if constexpr(Ptype::detailed_wire_type == pbf_varint)
        {
            return get_varint<type>();
        }
        else if constexpr(Ptype::detailed_wire_type == pbf_zigzag_varint)
        {
            return zigzag_decode(get_varint<std::make_unsigned_t<type>>());
        }
        else if constexpr(Ptype::detailed_wire_type == pbf_bool_varint)
        {
            if(BOOST_UNLIKELY(beg == end))
                throw pbf_exception{"no byte for bool"};

            return (*_beg != 0);
        }
        else if constexpr(Ptype::detailed_wire_type == pbf_fix32 || Ptype::detailed_wire_type == pbf_fix64)
        {
            if(BOOST_UNLIKELY(size() < sizeof(typename Ptype::type)))
                throw pbf_exception{"not enough bytes for fixed type"};

            type v;
            load_le(_beg, v);
            use(sizeof(type));
            return v;
        }
        else if constexpr(Ptype::detailed_wire_type == pbf_len_dlm)
        {
            type v{reinterpret_cast<typename type::value_type const*>(_beg), get_varint<uint32_t>()};
            use(v.size());
            return v;
        }
        else if constexpr(Ptype::detailed_wire_type == pbf_sub_msg)
        {
            pbf_reader v{_beg, get_varint<uint32_t>()};
            use(v.size());
            return v;
        }
        else
        {
            _JKDF_DEPENDENT_FAIL(Ptype, "unsupported type");
        }
    }

public:
    template<class Buf>
    explicit pbf_reader(Buf const& b)
        : pbf_reader{buf_begin(b), buf_end(b)}
    {}
    
    pbf_reader(void const* beg, size_t len)
        : pbf_reader{beg, beg + len}
    {}

    pbf_reader(void const* beg, void const* end)
        : _beg{static_cast<int const*>(beg)}, _end{static_cast<int const*>(end)}
    {
        read_tag();
    }

    size_t size() const
    {
        return _end - _beg;
    }

    bool empty() const
    {
        return _beg == _end;
    }

    template<class Ptype>
    Ptype::type required()
    {
        if(BOOST_UNLIKELY(_id != Ptype::id || _wireType != Ptype::wire_type))
        {
            if(_id == 0)
                throw pbf_exception{"failed to read tag"};
            if(_id != Ptype::id)
                throw pbf_exception{"id mismatch"};
            if(_wireType != Ptype::wire_type)
                throw pbf_exception{"wire type mismatch"};
        }

        return read_single_value_and_tag<Ptype>();
    }

    template<class Ptype>
    std::optional<Ptype::type> optional()
    {
        if(BOOST_UNLIKELY(_id == 0 || _wireType != Ptype::wire_type))
        {
            if(_id == 0)
                throw pbf_exception{"failed to read tag"};
            if(_wireType != Ptype::wire_type)
                throw pbf_exception{"wire type mismatch"};
        }

        if(_id != Ptype::id)
            return {};

        return read_single_value_and_tag<Ptype>();
    }

    template<class Ptype>
    Ptype::type optional(Ptype::type default_)
    {
        if(BOOST_UNLIKELY(_id == 0 || _wireType != Ptype::wire_type))
        {
            if(_id == 0)
                throw pbf_exception{"failed to read tag"};
            if(_wireType != Ptype::wire_type)
                throw pbf_exception{"wire type mismatch"};
        }

        if(_id != Ptype::id)
            return default_;

        return read_single_value_and_tag<Ptype>();
    }

    // or you can write your own loop with optional(), which is useful for repeated sub message fields
    template<class Ptype, class Seq>
    void repeated(Seq& seq)
    {
        while(auto v = optional<Ptype>())
            seq.emplace_back(std::move(*v));
    }

    template<class Ptype, class Seq>
    void packed_repeated(Seq& seq)
    {
        static_assert(Ptype::wire_type != pbf_len_dlm); // only repeated scalar type can be packed

        if(BOOST_UNLIKELY(_id == 0 || _wireType != pbf_len_dlm))
        {
            if(_id == 0)
                throw pbf_exception{"failed to read tag"};
            if(_wireType != pbf_len_dlm)
                throw pbf_exception{"wire type mismatch"};
        }

        if(_id == Ptype::id)
        {
            pbf_reader sub{_beg, get_varint<uint32_t>()};

            while(sub.size())
                seq.emplace_back(sub.read_single_value<Ptype>());
        }
    }

    // a map is a repeated message, that each contains two optional fields: <key:id=1, value:id=2>
    // for our use case, we'll force <key, value> to be required
    template<template<uint32_t> class KPtype, template<uint32_t> class VPtype, uint32_t Id, class Map>
    void map(Map& m)
    {
        while(auto pa = optional<pbf_message<Id>>())
        {
            m.emplace(pa->required<KPtype<1>>(), pa->required<VPtype<2>>());
        }
    }

    // only one field in oneof will present on wire
    template<class...Ptypes>
    auto oneof()
    {
        static_assert(sizeof...(Ptypes) > 2);

        if(BOOST_UNLIKELY(_id == 0))
            throw pbf_exception{"failed to read tag"};

        return do_oneof(Ptypes{}...);
    }

    void do_oneof()
    {
        throw pbf_exception{"no field found for oneof"};
    }

    template<class Ptype, class...Ptypes>
    auto do_oneof(Ptype, Ptypes...ptypes)
    {
        if(id == Ptype::id)
        {
            if(BOOST_UNLIKELY(_wireType != Ptype::wire_type))
                throw pbf_exception{"wire type mismatch"};
            return read_single_value_and_tag<Ptype>();
        }
        else
        {
            return do_oneof(ptypes...);
        }
    }
};

} // namespace jkdf