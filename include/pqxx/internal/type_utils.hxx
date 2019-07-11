/** Type/template metaprogramming utilities for use internally in libpqxx
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_TYPE_UTILS
#define PQXX_H_TYPE_UTILS

#include <memory>
#include <type_traits>

#include <optional>


namespace pqxx::internal
{
/// Extract the content type held by an `optional`-like wrapper type.
/* Replace nested `std::remove_*`s with `std::remove_cvref` in C++20 */
template<typename T> using inner_type = typename std::remove_cv<
  typename std::remove_reference<
    decltype(*std::declval<T>())
  >::type
>::type;

/// Does the given type have an `operator *()`?
template<typename T, typename = void> struct is_derefable : std::false_type {};
template<typename T> struct is_derefable<T, std::void_t<
  // Disable for arrays so they don't erroneously decay to pointers.
  inner_type<typename std::enable_if<not std::is_array<T>::value, T>::type>
>> : std::true_type {};

/// Should the given type be treated as an optional-value wrapper type?
template<typename T, typename = void> struct is_optional : std::false_type {};
template<typename T> struct is_optional<T, typename std::enable_if<(
  is_derefable<T>::value
  // Check if an `explicit operator bool` exists for this type
  && std::is_constructible<bool, T>::value
)>::type> : std::true_type {};

/// Can `nullopt_t` implicitly convert to type T?
template<
  typename T,
  typename = void
> struct takes_std_nullopt : std::false_type {};

template<typename T> struct takes_std_nullopt<
    T,
    typename std::enable_if<std::is_assignable<T, std::nullopt_t>::value>::type
> : std::true_type {};

/// Is type T a `std::tuple<>`?
template<typename T, typename = void> struct is_tuple : std::false_type {};
template<typename T> struct is_tuple<
  T,
  typename std::enable_if<(std::tuple_size<T>::value >= 0)>::type
> : std::true_type {};

/// Is type T an iterable container?
template<typename T, typename = void> struct is_container : std::false_type {};
template<typename T> struct is_container<
  T,
  std::void_t<
    decltype(std::begin(std::declval<T>())),
    decltype(std::end(std::declval<T>())),
    // Some people might implement a `std::tuple<>` specialization that is
    // iterable when all the contained types are the same ;)
    typename std::enable_if<not is_tuple<T>::value>::type
  >
> : std::true_type {};

/// Get an appropriate null value for the given type.
/**
 * pointer types                         `nullptr`
 * `std::optional<>`-like                `std::nullopt`
 * `std::experimental::optional<>`-like  `std::experimental::nullopt`
 * other types                           `pqxx::string_traits<>::null()`
 * Users may add support for their own wrapper types following this pattern.
 */
template<typename T> constexpr auto null_value()
  -> typename std::enable_if<
    (
      is_optional<T>::value
      && not takes_std_nullopt<T>::value
      && std::is_assignable<T, std::nullptr_t>::value
    ),
    std::nullptr_t
  >::type
{ return nullptr; }

template<typename T> constexpr auto null_value()
  -> typename std::enable_if<
    (not is_optional<T>::value && not takes_std_nullopt<T>::value),
    decltype(pqxx::string_traits<T>::null())
  >::type
{ return pqxx::string_traits<T>::null(); }

template<typename T> constexpr auto null_value()
  -> typename std::enable_if<
    takes_std_nullopt<T>::value,
    std::nullopt_t
  >::type
{ return std::nullopt; }

/// Construct an optional-like type from the stored type.
/** 
 * While these may seem redundant, they are necessary to support smart pointers
 * as optional storage types in a generic manner.  It is suggested NOT to
 * provide a version for `inner_type<T>*` as that will almost certainly leak
 * memory.
 * Users may add support for their own wrapper types following this pattern.
 */
// Enabled if the wrapper type can be directly constructed from the wrapped type
// (e.g. `std::optional<>`); explicitly disabled for raw pointers in case the
// inner type is convertible to a pointer (e.g. `int`)
template<typename T, typename V> constexpr auto make_optional(V&& v)
  -> typename std::enable_if<
    not std::is_same<T, inner_type<T>*>::value,
    decltype(T(std::forward<V>(v)))
  >::type
{ return T(std::forward<V>(v)); }
// Enabled if T is a specialization of `std::unique_ptr<>`.
template<typename T, typename V> constexpr auto make_optional(V&& v)
  -> typename std::enable_if<
    std::is_same<T, std::unique_ptr<inner_type<T>>>::value,
    std::unique_ptr<inner_type<T>>
  >::type
{
  return std::unique_ptr<inner_type<T>>(new inner_type<T>(std::forward<V>(v)));
}
// Enabled if T is a specialization of `std::shared_ptr<>`.
template<typename T, typename V> constexpr auto make_optional(V&& v)
  -> typename std::enable_if<
    std::is_same<T, std::shared_ptr<inner_type<T>>>::value,
    std::shared_ptr<inner_type<T>>
  >::type
{ return std::make_shared<inner_type<T>>(std::forward<V>(v)); }

} // namespace pqxx::internal


// TODO: Move?
namespace pqxx
{
/// Meta `pqxx::string_traits` for std::optional-like types.
template<typename T> struct string_traits<
  T,
  typename std::enable_if<internal::is_optional<T>::value>::type
>
{
  static constexpr bool has_null() noexcept { return true; }
  static bool is_null(const T& v)
    { return (not v || string_traits<I>::is_null(*v)); }
  static constexpr T null() { return internal::null_value<T>(); }
  static void from_string(std::string_view str, T &obj)
  {
    if (str.data() == nullptr) obj = null();
    else
    {
      I inner;
      string_traits<I>::from_string(str, inner);
      // Utilize existing memory if possible (e.g. for pointer types).
      if (obj) *obj = inner;
      // Important to assign to set valid flag for smart optional types.
      else obj = internal::make_optional<T>(inner);
    }
  }
  static std::string to_string(const T& Obj)
  {
    if (is_null(Obj)) internal::throw_null_conversion(type_name<T>);
    return string_traits<I>::to_string(*Obj);
  }

private:
  using I = internal::inner_type<T>;
};
} // namespace pqxx
#endif
