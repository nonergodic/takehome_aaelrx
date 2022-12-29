#ifndef UTILS_HPP
#define UTILS_HPP

#include <vector>
#include <type_traits>

template <typename FuncT> //for FuncT = [](size_t) -> T
auto make_vector(
  std::size_t size,
  FuncT const & functor
) -> typename std::enable_if_t<
      std::is_invocable_v<FuncT(std::size_t)>,
      std::vector<typename std::invoke_result<FuncT, std::size_t>::type>
    > {
  using T = std::invoke_result<FuncT, std::size_t>::type;
  auto vec = std::vector<T>(size);
  for (size_t i = 0; i < size; ++i)
    vec[i] = functor(i);
  return vec;
}

template <typename FuncT> //for FuncT = []() -> T
auto make_vector(
  std::size_t size,
  FuncT const & functor
) -> typename std::enable_if_t<
    std::is_invocable_v<FuncT()>,
    std::vector<typename std::invoke_result<FuncT>::type>
  > {
  //REVIEW would functor wrapping + call-forwarding be preferable to ~ code duplication?
  using T = std::invoke_result<FuncT>::type;
  auto vec = std::vector<T>(size);
  std::generate(std::begin(vec), std::end(vec), functor);
  return vec;
}

#endif
