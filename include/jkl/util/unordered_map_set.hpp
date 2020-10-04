#pragma once

#include <jkl/config.hpp>
#include <jkl/util/hash.hpp>
#include <jkl/util/comparator.hpp>
#include <robin_hood.h>


namespace jkl{


// TODO: sync with robin_hood


// robin_hood::hash doesn't respect SFINAE
//template<class T> requires(_hashable_<std::hash<T>, T>)
//struct robin_hood_hash : robin_hood::hash<T>{};

struct robin_hood_hash
{
    template<class T>
    constexpr size_t operator()(T const& t) noexcept requires(_hashable_<std::hash<T>, T> || _str_class_<T>)
    {
        if constexpr(_smart_ptr_<T>)
            return operator()(t.get());
        else if constexpr(_str_class_<T>)
            return robin_hood::hash_bytes(str_data(t), str_size(t) * sizeof(str_value_t<T>));
        else
            return robin_hood::hash<T>()(t);
    }
};

// unordered_flat_xxx store value in an vector, so references to value are not stable.
// unordered_node_xxx allocate separately for each value, and only store pointers to value in vector, so references to value are stable.
// unordered_auto_xxx choose either unordered_flat_xxx or unordered_node_xxx based on your data layout

/// map

template<class Key, class T, class Hash = select_hash<robin_hood_hash, T>, class KeyEqual = equal_to, size_t MaxLoadFactor100 = 80>
using unordered_flat_map = robin_hood::unordered_flat_map<Key, T, Hash, KeyEqual, MaxLoadFactor100>;

template<class Key, class T, class Hash = select_hash<robin_hood_hash, T>, class KeyEqual = equal_to, size_t MaxLoadFactor100 = 80>
using unordered_node_map = robin_hood::unordered_node_map<Key, T, Hash, KeyEqual, MaxLoadFactor100>;

template<class Key, class T, class Hash = select_hash<robin_hood_hash, T>, class KeyEqual = equal_to, size_t MaxLoadFactor100 = 80>
using unordered_auto_map = robin_hood::unordered_map<Key, T, Hash, KeyEqual, MaxLoadFactor100>;

/// set

template<class Key, class Hash = select_hash<robin_hood_hash, Key>, class KeyEqual = equal_to, size_t MaxLoadFactor100 = 80>
using unordered_flat_set = robin_hood::unordered_flat_set<Key, Hash, KeyEqual, MaxLoadFactor100>;

template<class Key, class Hash = select_hash<robin_hood_hash, Key>, class KeyEqual = equal_to, size_t MaxLoadFactor100 = 80>
using unordered_node_set = robin_hood::unordered_node_set<Key, Hash, KeyEqual, MaxLoadFactor100>;

template<class Key, class Hash = select_hash<robin_hood_hash, Key>, class KeyEqual = equal_to, size_t MaxLoadFactor100 = 80>
using unordered_auto_set = robin_hood::unordered_set<Key, Hash, KeyEqual, MaxLoadFactor100>;


} // namespace jkl
