#ifndef HKEYSTORE_UTILITY_H
#define HKEYSTORE_UTILITY_H

#include <string>
#include <type_traits>

std::string find_next_sub_key(const std::string& path, size_t& pos);

std::string get_path_tail(const std::string& path, size_t pos);

bool split_property_path(const std::string& property_path, std::string& node_path, std::string& property_name);

class TypeConverter
{
public:
   template<typename From, typename To>
   static typename std::enable_if<(std::is_arithmetic<From>::value && std::is_arithmetic<To>::value) || std::is_same<From, To>::value, bool>::type convert(const From& from, To& to)
   {
      to = static_cast<To>(from);
      return true;
   }

   template<typename From, typename To>
   static typename std::enable_if<(!std::is_arithmetic<From>::value || !std::is_arithmetic<To>::value) && !std::is_same<From, To>::value, bool>::type convert(const From& from, To& to)
   {
      return false;
   }
};

#endif
