#pragma once

#include <jkl/config.hpp>
#include <boost/container/small_vector.hpp>


namespace jkl{


// supported options: boost::container::growth_factor and boost::container::inplace_alignment
template<class T, size_t N, class Allocator = void, class Options = void>
using small_vector = boost::container::small_vector<T, N, Allocator, Options>;


} // namespace jkl
