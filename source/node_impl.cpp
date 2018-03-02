#include <errors.h>
#include "node_impl.h"
#include "utility.h"

std::shared_ptr<NodeImpl> NodeImpl::get_child(const std::string& name)
{
   lock_guard locker(lock);

   auto it = nodes.find(name);
   if (it == nodes.end()) {
      return nullptr;
   } 
   return it->second;
}


template<typename T>
void NodeImpl::set_property_impl(const std::string& name, const T& value)
{
   if (name.find('.') != std::string::npos) {
      throw LogicError("Can't add property with name '" + name + "'. Property names can't contain dots");
   }

   lock_guard locker(lock);

   properties[name] = value;
}

template<typename T>
bool NodeImpl::get_property_impl(const std::string& name, T& value)
{
   lock_guard locker(lock);
   auto it = properties.find(name);
   if (it == properties.end()) {
      return false;
   }

   return std::visit([&](auto&& property_value) {
      return TypeConverter::convert<std::decay_t<decltype(property_value)>, std::decay_t<T>>(property_value, value);
   }, it->second);
}

std::shared_ptr<NodeImpl> NodeImpl::add_node_impl(const std::string& name)
{
   if (name.find('.') != std::string::npos) {
      throw LogicError("Can't add node with name '" + name + "'. Node names can't contain dots");
   }

   lock_guard locker(lock);
   if (nodes.find(name) != nodes.end()) {
      throw NodeAlreadyExists("Node " + name + " already exists.");
   }

   auto new_node = std::make_shared<NodeImpl>();
   nodes.insert({ name, new_node });
   return new_node;
}

std::shared_ptr<NodeImpl> NodeImpl::get_node_impl(std::shared_ptr<NodeImpl> node, const std::string& path)
{
   std::shared_ptr<NodeImpl> cur_node = node;
   size_t i_path = 0;
   while (i_path < path.length()) {
      lock_guard locker(cur_node->lock);

      std::string sub_key = find_next_sub_key(path, i_path);
      auto it = cur_node->nodes.find(sub_key);
      if (it == cur_node->nodes.end()) {
         return nullptr;
      }
      cur_node = it->second;
   }
   return cur_node;
}

template bool NodeImpl::get_property_impl<int>(const std::string& name, int& value);
template bool NodeImpl::get_property_impl<int64_t>(const std::string& name, int64_t& value);
template bool NodeImpl::get_property_impl<float>(const std::string& name, float& value);
template bool NodeImpl::get_property_impl<double>(const std::string& name, double& value);
template bool NodeImpl::get_property_impl<long double>(const std::string& name, long double& value);
template bool NodeImpl::get_property_impl<std::string>(const std::string& name, std::string& value);

template void NodeImpl::set_property_impl<int>(const std::string& name, const int& value);
template void NodeImpl::set_property_impl<int64_t>(const std::string& name, const int64_t& value);
template void NodeImpl::set_property_impl<float>(const std::string& name, const float& value);
template void NodeImpl::set_property_impl<double>(const std::string& name, const double& value);
template void NodeImpl::set_property_impl<long double>(const std::string& name, const long double& value);
template void NodeImpl::set_property_impl<std::string>(const std::string& name, const std::string& value);

