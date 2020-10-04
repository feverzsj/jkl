#pragma once

#include <jkl/config.hpp>
#include <jkl/util/comparator.hpp>
#include <parallel_hashmap/btree.h>


namespace jkl{


// TODO: sync with parallel_hashmap


template<class Key, class Value, class Compare = less, class Alloc = phmap::Allocator<phmap::container_internal::Pair<const Key, Value>>>
using btree_map = phmap::btree_map<Key, Value, Compare, Alloc>;

template<class Key, class Value, class Compare = less, class Alloc = phmap::Allocator<phmap::container_internal::Pair<const Key, Value>>>
using btree_multimap = phmap::btree_multimap<Key, Value, Compare, Alloc>;


template<class Key, class Compare = less, class Alloc = phmap::Allocator<Key>>
using btree_set = phmap::btree_set<Key, Compare, Alloc>;

template<class Key, class Compare = less, class Alloc = phmap::Allocator<Key>>
using btree_multiset = phmap::btree_multiset<Key, Compare, Alloc>;


} // namespace jkl
