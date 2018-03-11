#include <storage.h>
#include <errors.h>
#include "volume_impl.h"
#include "utility.h"
#include "node_impl.h"

namespace hks {

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

   node->mount_points.emplace_back(volume_impl, node_path, node_to_mount);
}

void Storage::unmount(std::shared_ptr<Volume> volume, const std::string& path)
{
   unmount(volume, path, "");
}

void Storage::unmount(std::shared_ptr<Volume> volume, const std::string& path, const std::string& node_path)
{
   std::shared_ptr<VolumeImpl> volume_impl = std::static_pointer_cast<VolumeImpl>(volume);

   std::unique_lock<std::shared_mutex> lock(volumes_lock);

   std::vector<MountNode*> nodes_stack;
   std::vector<std::string> keys_stack;
   nodes_stack.push_back(&volumes_root);
   keys_stack.push_back("");

   MountNode* node = &volumes_root;
   size_t i_path = 0;
   while (i_path < path.length()) {
      std::string sub_key = find_next_sub_key(path, i_path);
      auto it = node->nodes.find(sub_key);
      if (it == node->nodes.end()) {
         throw LogicError("Volume was not mounted at specified point");
      }
      node = &it->second;
      nodes_stack.push_back(node);
      keys_stack.push_back(sub_key);
   }

   auto it = std::find_if(node->mount_points.begin(), node->mount_points.end(),
      [&](const MountPoint& mount_point)->bool { return mount_point.volume == volume_impl && mount_point.node_path == node_path; });

   if (it == node->mount_points.end()) {
      throw LogicError("Volume was not mounted at specified point");
   }

   node->mount_points.erase(it);

   // Clear reduntant mount nodes
   for (size_t i = nodes_stack.size() - 1; i > 0; --i) {
      MountNode* stack_node = nodes_stack[i];
      if (stack_node->mount_points.size() == 0 && stack_node->nodes.size() == 0) {
         nodes_stack[i - 1]->nodes.erase(keys_stack[i - 1]);
      } else {
         break;
      }
   }
}


std::shared_ptr<Node> Storage::get_node(const std::string& path) const
{
   std::shared_lock<std::shared_mutex> lock(volumes_lock);
   const MountNode* node = &volumes_root;
   size_t i_path = 0;
   while (true) {
      for (auto& mount_point : node->mount_points) {
         std::shared_ptr<Node> node = mount_point.node->get_node_impl(get_path_tail(path, i_path));
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

std::shared_ptr<Node> Storage::add_node(const std::string& path, const std::string& name)
{
   std::shared_ptr<Node> parent = get_node(path);
   if (parent == nullptr) {
      throw NoSuchNode("Node '" + path + "' doesn't exist");
   }

   return parent->add_child(name);
}

void Storage::remove_node(const std::string& path)
{
   std::string parent_path;
   std::string node_name;
   split_node_path(path, parent_path, node_name);
   std::shared_ptr<Node> parent = get_node(parent_path);
   if (parent == nullptr) {
      throw NoSuchNode("Node '" + path + "' doesn't exist");
   }
   parent->remove_child(node_name);
}

void Storage::rename_node(const std::string& path, const std::string& new_name)
{
   std::string parent_path;
   std::string node_name;
   split_node_path(path, parent_path, node_name);
   std::shared_ptr<Node> parent = get_node(parent_path);
   if (parent == nullptr) {
      throw NoSuchNode("Node '" + path + "' doesn't exist");
   }
   parent->rename_child(node_name, new_name);
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

bool Storage::remove_property(const std::string& path)
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

   return node->remove_property(property_name);
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


Storage::MountPoint::MountPoint(std::shared_ptr<VolumeImpl>& volume, const std::string& node_path, std::shared_ptr<NodeImpl>& node)
   : volume(volume)
   , node_path(node_path)
   , node(node)
{
}

}
