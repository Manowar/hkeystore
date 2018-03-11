#ifndef HKEYSTORE_SERIALIZATION_H
#define HKEYSTORE_SERIALIZATION_H

#include <iostream>
#include <vector>
#include <string>
#include <variant>

namespace hks {

template<typename T>
inline typename std::enable_if<std::is_arithmetic<T>::value, void>::type serialize(std::ostream& is, T value)
{
   is.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template<typename T>
inline typename std::enable_if<std::is_arithmetic<T>::value, void>::type deserialize(std::istream& is, T& value)
{
   is.read(reinterpret_cast<char*>(&value), sizeof(T));
}

template<typename T>
auto serialize(std::ostream& os, const T& value) -> decltype(value.serialize(os))
{
   value.serialize(os);
}

template<typename T>
auto deserialize(std::istream& is, T& value) -> decltype(value.deserialize(is))
{
   value.deserialize(is);
}

template<typename T, int N>
inline void serialize(std::ostream& os, const T (&array)[N])
{
   for (int i = 0; i < N; i++) {
      serialize(os, array[i]);
   }
}

template<typename T, int N>
inline void deserialize(std::istream& is, T (&array)[N])
{
   for (int i = 0; i < N; i++) {
      deserialize(is, array[i]);
   }
}

template<typename T>
inline void serialize(std::ostream& os, const std::vector<T>& vector)
{
   size_t size = vector.size();
   serialize(os, size);
   for (size_t i = 0; i < size; i++) {
      serialize(os, vector[i]);
   }
}

template<typename T>
inline void deserialize(std::istream& is, std::vector<T>& vector)
{
   vector.clear();

   size_t size;
   deserialize(is, size);
   vector.resize(size);

   for (size_t i = 0; i < size; i++) {
      deserialize(is, vector[i]);
   }
}

inline void serialize(std::ostream& os, const std::string& string)
{
   size_t size = string.size();
   serialize(os, size);
   os.write(string.c_str(), size);
}

inline void deserialize(std::istream& is, std::string& string)
{
   size_t size;
   deserialize(is, size);

   std::vector<char> string_chars(size);
   is.read(string_chars.data(), size);
   string.assign(string_chars.data(), size);
}

template<typename key, typename value>
inline void serialize(std::ostream& os, const std::unordered_map<key, value>& map)
{
   size_t size = map.size();
   serialize(os, size);
   for (auto it = map.begin(); it != map.end(); ++it) {
      serialize(os, it->first);
      serialize(os, it->second);
   }
}

template<typename key, typename value>
inline void deserialize(std::istream& is, std::unordered_map<key, value>& map)
{
   map.clear();

   size_t size;
   deserialize(is, size);
   for (size_t i = 0; i < size; i++) {
      key k;
      value v;
      deserialize(is, k);
      deserialize(is, v);
      map.insert({ k, v });
   }
}

template<typename clock>
inline void deserialize(std::istream& is, std::chrono::time_point<clock>& tp)
{
   std::chrono::milliseconds::rep millis;
   deserialize(is, millis);
   tp = std::chrono::time_point<clock>(std::chrono::milliseconds(millis));
}

template<typename clock>
void serialize(std::ostream& os, const std::chrono::time_point<clock>& tp)
{
   std::chrono::milliseconds::rep millis = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
   serialize(os, millis);
}

template<typename... Types>
void serialize(std::ostream& os, const std::variant<Types...>& value)
{
   size_t type = value.index();
   serialize(os, type);
   std::visit([&](auto&& property_value) {
      serialize(os, property_value);
   }, value);
}


template<int N, typename... Types>
inline void deserialize_specific_type(std::istream& is, std::variant<Types...>& value)
{
   std::variant_alternative_t<N, NodeImpl::PropertyValue> v;
   deserialize(is, v);
   value = v;
}

template<int N, typename... Types>
struct variant_deserializer
{
   static inline void deserialize_type_n(size_t type, std::istream& is, std::variant<Types...>& value)
   {
      if (type == N) {
         deserialize_specific_type<N, Types...>(is, value);
      } else {
         variant_deserializer<N - 1, Types...>::deserialize_type_n(type, is, value);
      }
   }
};

template<typename... Types>
struct variant_deserializer<0, Types...>
{
   static inline void deserialize_type_n(size_t type, std::istream& is, std::variant<Types...>& value)
   {
      deserialize_specific_type<0, Types...>(is, value);
   }
};


template<typename... Types>
void deserialize(std::istream& is, std::variant<Types...>& value)
{
   size_t type;
   deserialize(is, type);
   variant_deserializer<std::variant_size<std::variant<Types...>>() - 1, Types...>::deserialize_type_n(type, is, value);
}

}

#endif
