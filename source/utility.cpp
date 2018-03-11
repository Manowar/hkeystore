#include <cassert>
#include "utility.h"

namespace hks {

std::string find_next_sub_key(const std::string& path, size_t& pos)
{
   size_t dot_offset = pos == 0 ? 0 : 1;
   size_t i_dot = path.find('.', pos + dot_offset);
   std::string sub_key;
   if (i_dot == std::string::npos) {
      sub_key = path.substr(pos + dot_offset);
      pos = path.length();
   } else {
      sub_key = path.substr(pos + dot_offset, i_dot - pos - dot_offset);
      pos = i_dot;
   }
   return sub_key;
}

std::string get_path_tail(const std::string& path, size_t pos)
{
   if (pos == 0) {
      return path;
   }
   if (pos == path.length()) {
      return "";
   }
   assert(path[pos] == '.');
   return path.substr(pos + 1);
}

bool split_property_path(const std::string& property_path, std::string& node_path, std::string& property_name)
{
   size_t i_dot = property_path.find_last_of('.');
   if (i_dot == std::string::npos) {
      return false;
   }
   node_path = property_path.substr(0, i_dot);
   property_name = property_path.substr(i_dot + 1);
   return true;
}

void split_node_path(const std::string& node_path, std::string& parent_path, std::string& node_name)
{
   size_t i_dot = node_path.find_last_of('.');
   if (i_dot == std::string::npos) {
      parent_path = "";
      node_name = node_path;
   } else {
      parent_path = node_path.substr(0, i_dot);
      node_name = node_path.substr(i_dot + 1);
   }
}

}
