#ifndef UTIL_HH
#define UTIL_HH
#include <typeinfo>
#include <string>
#include <sstream>
#include <memory_resource>
#include <memory>

using namespace std;

/* TODO: The strings are implementation-defined.  How do they look in
   clang? */

inline std::string pretty_type(const char *raw)
{
  ostringstream os;
  os << raw;
  string s = os.str();
  while (s[0] <= '9')
    s.erase(s.begin());
  return s;
}

inline std::string pretty_type(struct prod *p)
{
  // TODO: this does not work atm, as i made the base abstract
  // TODO: also it would never return the actual object type ... pointers are not polymorphic for typeid
  // return pretty_type(typeid(*p).name());
  return {};
}

std::pmr::memory_resource *getMemoryResource();

// just a some helpers to avoid too much boilerplate
template<typename T>
std::pmr::polymorphic_allocator<T> getAllocator()
{
  return {getMemoryResource()};
}

template<typename T, typename... _Args>
std::shared_ptr<T> make_alloc_shared(_Args &&... _args)
{
  return std::allocate_shared<T>(getAllocator<T>(), std::forward<_Args>(_args)...);
}

#endif
