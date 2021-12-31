#pragma once

#include <list>
//#include <functional>
#include <type_traits>

#include <llvm/ADT/StringMap.h>

namespace celerity
{

  /// A templatized, singleton Registry for type T, which maps a String key to a T object.
  template <typename T>
  class Registry 
  {
  public:
    Registry() = delete;
    Registry(const Registry &) = delete;
    Registry(Registry &&) noexcept = delete;
    Registry &operator=(const Registry &) = delete;
    Registry &operator=(Registry &&) noexcept = delete;

    /// Returns a T object if already registered by key, otherwise returns a default-constructed T object if the key does not exist.
    template <typename... Args>
    static T dispatch(const llvm::StringRef &key, Args &&...args)
    {
      auto it = map().find(key);
      if (it == map().end())
        return T();
      return it->second(std::forward<Args>(args)...);
    }

    /// Register a T value for the string key
    static bool registerByKey(const llvm::StringRef &key, T &value)
    {
      map()[key] = value;
      return true;
    }

    /// Test whether the given key is registered
    static bool isRegistered(const llvm::StringRef &key) { return map().count(key) == 1u; }

    /// Unregisters the given identifier
    static void unregisterByKey(const llvm::StringRef &key) { map().erase(key); }

    /// registered keys
    static std::list<StringRef> getKeyList() { 
      std::list<StringRef> key_list;
      for(StringRef& s : map().keys() )
        key_list.push_back(s);
      return key_list; 
    } 
    

  private:
    static llvm::StringMap<T> &map()
    {
      static llvm::StringMap<T> registry_map;
      return registry_map;
    }
  };

} // namespace

