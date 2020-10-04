#pragma once

#include <jkl/config.hpp>
#include <jkl/util/comparator.hpp>
#include <jkl/util/small_vector.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>


namespace jkl{


// TODO: sync with boost::container

/// map

template<class Key, class T, class Compare = less, class AllocatorOrContainer = boost::container::new_allocator<std::pair<Key, T>>>
using flat_map = boost::container::flat_map<Key, T, Compare, AllocatorOrContainer>;

template<class Key, class T, class Compare = less, class AllocatorOrContainer = boost::container::new_allocator<std::pair<Key, T>>>
using flat_multimap = boost::container::flat_multimap<Key, T, Compare, AllocatorOrContainer>;

template<class Key, class T, size_t N, class Compare = less, class Allocator = void, class Options = void>
using small_flat_map = flat_map<Key, T, Compare, small_vector<std::pair<Key, T>, N, Allocator, Options>>;

template<class Key, class T, size_t N, class Compare = less, class Allocator = void, class Options = void>
using small_flat_multimap = flat_multimap<Key, T, Compare, small_vector<std::pair<Key, T>, N, Allocator, Options>>;


/// set

template<class Key, class Compare = less, class AllocatorOrContainer = boost::container::new_allocator<std::pair<Key, T>>>
using flat_set = boost::container::flat_set<Key, Compare, AllocatorOrContainer>;

template<class Key, class Compare = less, class AllocatorOrContainer = boost::container::new_allocator<std::pair<Key, T>>>
using flat_multiset = boost::container::flat_multiset<Key, Compare, AllocatorOrContainer>;

template<class Key, size_t N, class Compare = less, class Allocator = void, class Options = void>
using small_flat_set = flat_set<Key, Compare, small_vector<Key, N, Allocator, Options>>;

template<class Key, size_t N, class Compare = less, class Allocator = void, class Options = void>
using small_flat_multiset = flat_multiset<Key, Compare, small_vector<Key, N, Allocator, Options>>;


} // namespace jkl
