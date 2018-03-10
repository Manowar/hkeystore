#include <storage.h>
#include <errors.h>
#include "volume_impl.h"
#include "utility.h"
#include "node_impl.h"


std::shared_ptr<Volume> Storage::open_volume(const std::string& path, bool create_if_not_exist)
{
   return std::make_shared<VolumeImpl>(path, create_if_not_exist);
}

void Storage::mount(std::shared_ptr<Volume> volume, const std::string& path)
{
   mount(volume, path, "");
}

void Storage::mount(std::shared_ptr<Volume> volume, const std::string& path, const std::string& node_path)
{
   std::shared_ptr<VolumeImpl> volume_impl = std::static_pointer_cast<VolumeImpl>(volume);
   if (volume_impl->get_storage() != nullptr && volume_impl->get_storage() != this) {
      throw LogicError("Can't mount volume to more than 1 storage at the same time");
   }
   volume_impl->set_storage(this);

   std::unique_lock<std::shared_mutex> lock(volumes_lock);
   MountNode* node = &volumes_root;
   size_t i_path = 0;
   while (i_path < path.length()) {
      std::string sub_key = find_next_sub_key(path, i_path);
      node = &node->nodes[sub_key];
   }

   std::shared_ptr<NodeImpl> node_to_mount = volume_impl->get_node(node_path);
   if (!node_to_mount) {
      throw NoSuchNode("Mounting volume doesn't have node '" + node_path + "'");
   }

   node->volumes.push_back(node_to_mount);
}

std::shared_ptr<Node> Storage::get_node(const std::string& path) const
{
   std::shared_lock<std::shared_mutex> lock(volumes_lock);
   const MountNode* node = &volumes_root;
   size_t i_path = 0;
   while (true) {
      for (auto& volume_node : node->volumes) {
         std::shared_ptr<Node> node = volume_node->get_node_impl(get_path_tail(path, i_path));
         if (node) {
            return node;
         }
      }

      if (i_path == path.length()) {
         return nullptr;
      }

      std::string sub_key = find_next_sub_key(path, i_path);
      auto it = node->nodes.find(sub_key);
      if (it == node->nodes.end()) {
         return nullptr;
      }
      node = &it->second;
   }
}

bool Storage::get_property(const std::string& path, int& value) const
{
   return get_property_impl(path, value);
}

bool Storage::get_property(const std::string& path, unsigned& value) const
{
   return get_property_impl(path, value);
}

bool Storage::get_property(const std::string& path, int64_t& value) const
{
   return get_property_impl(path, value);
}

bool Storage::get_property(const std::string& path, uint64_t& value) const
{
   return get_property_impl(path, value);
}

bool Storage::get_property(const std::string& path, float& value) const
{
   return get_property_impl(path, value);
}

bool Storage::get_property(const std::string& path, double& value) const
{
   return get_property_impl(path, value);
}

bool Storage::get_property(const std::string& path, long double& value) const
{
   return get_property_impl(path, value);
}

bool Storage::get_property(const std::string& path, std::string& value) const
{
   return get_property_impl(path, value);
}

bool Storage::get_property(const std::string& path, std::vector<char>& value) const
{
   return get_property_impl(path, value);
}

bool Storage::set_property(const std::string& path, int value)
{
   return set_property_impl(path, value);
}

bool Storage::set_property(const std::string& path, unsigned value)
{
   return set_property_impl(path, value);
}

bool Storage::set_property(const std::string& path, int64_t value)
{
   return set_property_impl(path, value);
}

bool Storage::set_property(const std::string& path, uint64_t value)
{
   return set_property_impl(path, value);
}

bool Storage::set_property(const std::string& path, float value)
{
   return set_property_impl(path, value);
}

bool Storage::set_property(const std::string& path, double value)
{
   return set_property_impl(path, value);
}

bool Storage::set_property(const std::string& path, long double value)
{
   return set_property_impl(path, value);
}

bool Storage::set_property(const std::string& path, const std::string& value)
{
   return set_property_impl(path, value);
}

bool Storage::set_property(const std::string& path, const std::vector<char>& value)
{
   return set_property_impl(path, BlobHolder(value));
}

bool Storage::set_property(const std::string& path, void* data, size_t size)
{
   return set_property_impl(path, BlobHolder(data, size));
}

template<typename T>
bool Storage::set_property_impl(const std::string& path, const T& value)
{
   std::string node_path;
   std::string property_name;
   if (!split_property_path(path, node_path, property_name)) {
      throw LogicError(path + " is not a valid property path");
   }

   std::shared_ptr<NodeImpl> node = std::static_pointer_cast<NodeImpl>(get_node(node_path));
   if (!node) {
      return false;
   }

   node->set_property_impl(property_name, value);
   return true;
}

template<typename T>
bool Storage::get_property_impl(const std::string& path, T& value) const
{
   std::string node_path;
   std::string property_name;
   if (!split_property_path(path, node_path, property_name)) {
      throw LogicError(path + " is not a valid property path");
   }

   std::shared_ptr<Node> node = get_node(node_path);
   if (!node) {
      return false;
   }

   return node->get_property(property_name, value);
}

//template bool Storage::get_property<int>(const std::string& path, int& value);
//template bool Storage::get_property<unsigned>(const std::string& path, unsigned& value);
//template bool Storage::get_property<int64_t>(const std::string& path, int64_t& value);
//template bool Storage::get_property<uint64_t>(const std::string& path, uint64_t& value);
//template bool Storage::get_property<float>(const std::string& path, float& value);
//template bool Storage::get_property<double>(const std::string& path, double& value);
//template bool Storage::get_property<long double>(const std::string& path, long double& value);
//template bool Storage::get_property<std::string>(const std::string& path, std::string& value);
//
//template bool Storage::set_property<int>(const std::string& path, const int& value);
//template bool Storage::set_property<unsigned>(const std::string& path, const unsigned& value);
//template bool Storage::set_property<int64_t>(const std::string& path, const int64_t& value);
//template bool Storage::set_property<uint64_t>(const std::string& path, const uint64_t& value);
//template bool Storage::set_property<float>(const std::string& path, const float& value);
//template bool Storage::set_property<double>(const std::string& path, const double& value);
//template bool Storage::set_property<long double>(const std::string& path, const long double& value);
//template bool Storage::set_property<std::string>(const std::string& path, const std::string& value);
//
