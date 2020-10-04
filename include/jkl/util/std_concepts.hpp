#pragma once

#include <cstddef> // for _LIBCPP_STD_VER

#if __has_include(<concepts>) && ! defined(_LIBCPP_STD_VER)
#   include <concepts>
#else
#include <jkl/config.hpp>
#include <jkl/util/type_traits.hpp>
#include <type_traits>


namespace jkl_detail{

template<class T, class U>
concept SameHelper = std::is_same_v<T, U>;

} // namespace jkl_detail


namespace std{


template<class T, class U>
concept same_as = jkl_detail::SameHelper<T, U> && jkl_detail::SameHelper<U, T>;

template<class Derived, class Base>
concept derived_from = is_base_of_v<Base, Derived> &&
                       is_convertible_v<const volatile Derived*, const volatile Base*>;

template<class From, class To>
concept convertible_to = is_convertible_v<From, To> &&
                         requires(add_rvalue_reference_t<From> (&f)()){ static_cast<To>(f()); };

template<class T, class U>
concept common_reference_with = same_as<jkl::common_reference_t<T, U>, jkl::common_reference_t<U, T>> &&
                                convertible_to<T, jkl::common_reference_t<T, U>> &&
                                convertible_to<U, jkl::common_reference_t<T, U>>;

template<class T, class U>
concept common_with = same_as<
                          common_type_t<T, U>, common_type_t<U, T>
                      > &&
                      requires{
                          static_cast<common_type_t<T, U>>(declval<T>());
                          static_cast<common_type_t<T, U>>(declval<U>());
                      } &&
                      common_reference_with<
                          add_lvalue_reference_t<const T>,
                          add_lvalue_reference_t<const U>
                      > &&
                      common_reference_with<
                          add_lvalue_reference_t<common_type_t<T, U>>,
                          jkl::common_reference_t<
                          add_lvalue_reference_t<const T>,
                          add_lvalue_reference_t<const U>>>;

template<class T> concept integral          = is_integral_v<T>;
template<class T> concept signed_integral   = integral<T> && is_signed_v<T>;
template<class T> concept unsigned_integral = integral<T> && ! signed_integral<T>;
template<class T> concept floating_point    = is_floating_point_v<T>;

template<class LHS, class RHS>
concept assignable_from = is_lvalue_reference_v<
                              LHS
                          > &&
                          common_reference_with<
                              const remove_reference_t<LHS>&,
                              const remove_reference_t<RHS>&
                          >&&
                          requires(LHS lhs, RHS&& rhs){
                                  { lhs = forward<RHS>(rhs) } -> same_as<LHS>;
                          };

template<class T>
concept swappable = requires(T& a, T& b){ /*ranges::*/swap(a, b); };

template<class T, class U>
concept swappable_with = common_reference_with<T, U> &&
                         requires(T&& t, U&& u){
                             /*ranges::*/swap(forward<T>(t), forward<T>(t));
                             /*ranges::*/swap(forward<U>(u), forward<U>(u));
                             /*ranges::*/swap(forward<T>(t), forward<U>(u));
                             /*ranges::*/swap(forward<U>(u), forward<T>(t));
                         };

template<class T>
concept destructible = is_nothrow_destructible_v<T>;

template<class T, class... Args>
concept constructible_from = destructible<T> && is_constructible_v<T, Args...>;

template<class T>
concept default_initializable = constructible_from<T> &&
                                requires { T{}; ::new (static_cast<void*>(nullptr)) T; };

template<class T>
concept move_constructible = constructible_from<T, T> && convertible_to<T, T>;

template<class T>
concept copy_constructible = move_constructible<T> &&
                             constructible_from<T, T&> && convertible_to<T&, T> &&
                             constructible_from<T, const T&> && convertible_to<const T&, T> &&
                             constructible_from<T, const T> && convertible_to<const T, T>;

template<class T>
concept movable = is_object_v<T> &&
                  move_constructible<T> &&
                  assignable_from<T&, T> &&
                  swappable<T>;

template<class T>
concept copyable = copy_constructible<T> &&
                   movable<T> &&
                   assignable_from<T&, const T&>;

template<class B>
concept boolean = movable<remove_cvref_t<B>> &&
                  requires(const remove_reference_t<B>& b1, const remove_reference_t<B>& b2, const bool a)
                  {
                          {   b1     } -> convertible_to<bool>;
                          { ! b1     } -> convertible_to<bool>;
                          { b1 && b2 } -> same_as<bool>;
                          { b1 &&  a } -> same_as<bool>;
                          {  a && b2 } -> same_as<bool>;
                          { b1 || b2 } -> same_as<bool>;
                          { b1 ||  a } -> same_as<bool>;
                          {  a || b2 } -> same_as<bool>;
                          { b1 == b2 } -> convertible_to<bool>;
                          { b1 ==  a } -> convertible_to<bool>;
                          {  a == b2 } -> convertible_to<bool>;
                          { b1 != b2 } -> convertible_to<bool>;
                          { b1 !=  a } -> convertible_to<bool>;
                          {  a != b2 } -> convertible_to<bool>;
                  };

template<class T, class U>  // exposition only
concept __WeaklyEqualityComparableWith = requires(const remove_reference_t<T>& t, const remove_reference_t<U>& u)
                                         {
                                                 { t == u } -> boolean;
                                                 { t != u } -> boolean;
                                                 { u == t } -> boolean;
                                                 { u != t } -> boolean;
                                         };
template<class T>
concept equality_comparable = __WeaklyEqualityComparableWith<T, T>;

template<class T, class U>
concept equality_comparable_with = equality_comparable<T> &&
                                   equality_comparable<U> &&
                                   common_reference_with<
                                       const remove_reference_t<T>&,
                                       const remove_reference_t<U>&> &&
                                   equality_comparable<
                                       jkl::common_reference_t<
                                       const remove_reference_t<T>&,
                                       const remove_reference_t<U>&>> &&
                                   __WeaklyEqualityComparableWith<T, U>;

template<class T>
concept totally_ordered = equality_comparable<T> &&
                          requires(const remove_reference_t<T>& a, const remove_reference_t<T>& b)
                          {
                                  { a <  b } -> boolean;
                                  { a >  b } -> boolean;
                                  { a <= b } -> boolean;
                                  { a >= b } -> boolean;
                          };

template<class T, class U>
concept totally_ordered_with = totally_ordered<T> &&
                               totally_ordered<U> &&
                               equality_comparable_with<T, U> &&
                               totally_ordered<
                                   jkl::common_reference_t<
                                       const remove_reference_t<T>&,
                                       const remove_reference_t<U>&>> &&
                               requires(const remove_reference_t<T>& t, const remove_reference_t<U>& u)
                               {
                                           { t <  u } -> boolean;
                                           { t >  u } -> boolean;
                                           { t <= u } -> boolean;
                                           { t >= u } -> boolean;
                                           { u <  t } -> boolean;
                                           { u >  t } -> boolean;
                                           { u <= t } -> boolean;
                                           { u >= t } -> boolean;
                               };

template<class T>
concept semiregular = copyable<T> && default_initializable<T>;

template<class T>
concept regular = semiregular<T> && equality_comparable<T>;

template<class F, class... Args>
concept invocable = requires(F&& f, Args&&... args) {
                        invoke(forward<F>(f), forward<Args>(args)...);
                    };

template<class F, class... Args>
concept regular_invocable = invocable<F, Args...>; // just semantically different from invocable

template<class F, class... Args>
concept predicate = regular_invocable<F, Args...> && boolean<invoke_result_t<F, Args...>>;

template<class R, class T, class U>
concept relation = predicate<R, T, T> && predicate<R, U, U> &&
                   predicate<R, T, U> && predicate<R, U, T>;

template<class R, class T, class U >
concept equivalence_relation = relation<R, T, U>;

template<class R, class T, class U>
concept strict_weak_order = relation<R, T, U>;


} // namespace std

#endif
