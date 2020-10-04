#pragma once

#include <jkl/util/type_traits.hpp>


namespace jkl{


// template<class Archive, class T>
// bool serialize_jval(Archive& ar, T& t);


// for jobj member pair's name/key
// when read, if a name is jopt_, absent of the pair won't return false.
// when write, if a name is jopt_, the pair is only written if jopt_.write == true;
template<class Str>
class jopt_
{
public:
    Str const& str;
    bool       write;
    explicit jopt_(Str const& s, bool w = true) : str(s), write(w) {}
};


auto& jopt_get_key(auto& k) { return k; }
auto& jopt_get_key(jopt_<auto> const& t) { return t.str; }


} // namespace jkl