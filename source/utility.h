#ifndef HKEYSTORE_UTILITY_H
#define HKEYSTORE_UTILITY_H

#include <string>
#include <chrono>
#include <type_traits>

namespace hks {

std::string find_next_sub_key(const std::string& path, size_t& pos);

std::string get_path_tail(const std::string& path, size_t pos);

bool split_property_path(const std::string& property_path, std::string& node_path, std::string& property_name);
void split_node_path(const std::string& node_path, std::string& parent_path, std::string& node_name);

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

}

//  std::chrono::time_point serialization
namespace boost {
   namespace archive {
      template<typename Archive, typename clock>
      void load(Archive& ar, std::chrono::time_point<clock>& tp, unsigned)
      {
         std::chrono::milliseconds::rep millis;
         ar & millis;
         tp = std::chrono::time_point<clock>(std::chrono::milliseconds(millis));
      }

      template<typename Archive, typename clock>
      void save(Archive& ar, std::chrono::time_point<clock> const& tp, unsigned)
      {
         std::chrono::milliseconds::rep millis = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
         ar & millis;
      }

      template<typename Archive, typename clock>
      inline void serialize(Archive & ar, std::chrono::time_point<clock>& tp, unsigned version)
      {
         boost::serialization::split_free(ar, tp, version);
      }
   }
}

#endif
