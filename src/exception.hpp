#ifndef EXCEPTION_HPP
#define EXCEPTION_HPP

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <string>

struct Exception : virtual std::exception
{
public:
  virtual ~Exception() override {}

  virtual char const* what() const noexcept override
  {
    return what_.c_str();
  }
  
  void set_what(std::string const & str)
  {
    what_ = str;
  }

  void set_what(std::string && str) noexcept
  {
    what_ = std::move(str);
  }

private:
  std::string what_;
};

template <typename T>
typename std::enable_if<std::is_base_of<Exception, T>::value, T>::type make_exception(std::string const & str)
{
  T e;
  e.set_what(str);
  return e;
}

template <typename T>
typename std::enable_if<std::is_base_of<Exception, T>::value, T>::type make_exception(std::string && str) noexcept
{
  T e;
  e.set_what(std::move(str));
  return e;
}

#endif
