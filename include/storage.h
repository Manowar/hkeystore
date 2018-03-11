#ifndef HKEYSTORE_STORAGE_H
#define HKEYSTORE_STORAGE_H


#include <memory>
#include <string>
#include <vector>
#include <shared_mutex>
#include <unordered_map>

#include <volume.h>

namespace hks {

class Node;
class VolumeImpl;
class NodeImpl;

class Storage
{
public:
   Storage() = default;

   Storage(const Storage&) = delete;
   void operator=(const Storage&) = delete;

   std::shared_ptr<Volume> open_volume(const std::string& path, bool create_if_not_exist);

   void mount(std::shared_ptr<Volume> volume, const std::string& path);
   void mount(std::shared_ptr<Volume> volume, const std::string& path, const std::string& node_path);
   void unmount(std::shared_ptr<Volume> volume, const std::string& path);
   void unmount(std::shared_ptr<Volume> volume, const std::string& path, const std::string& node_path);

   std::shared_ptr<Node> get_node(const std::string& path) const;
   std::shared_ptr<Node> add_node(const std::string& path, const std::string& name);
   void remove_node(const std::string& path);
   void rename_node(const std::string& path, const std::string& new_name);

   bool get_property(const std::string& path, int& value) const;
   bool get_property(const std::string& path, int64_t& value) const;
   bool get_property(const std::string& path, unsigned& value) const;
   bool get_property(const std::string& path, uint64_t& value) const;
   bool get_property(const std::string& path, float& value) const;
   bool get_property(const std::string& path, double& value) const;
   bool get_property(const std::string& path, long double& value) const;
   bool get_property(const std::string& path, std::string& value) const;
   bool get_property(const std::string& path, std::vector<char>& value) const;

   bool set_property(const std::string& path, int value);
   bool set_property(const std::string& path, int64_t value);
   bool set_property(const std::string& path, unsigned value);
   bool set_property(const std::string& path, uint64_t value);
   bool set_property(const std::string& path, float value);
   bool set_property(const std::string& path, double value);
   bool set_property(const std::string& path, long double value);
   bool set_property(const std::string& path, const std::string& value);
   bool set_property(const std::string& path, const std::vector<char>& value);
   bool set_property(const std::string& path, void* data, size_t size);

   bool remove_property(const std::string& path);

private:
   struct MountPoint
   {
      MountPoint(std::shared_ptr<VolumeImpl>& volume, const std::string& node_path, std::shared_ptr<NodeImpl>& node);

      std::shared_ptr<VolumeImpl> volume;
      std::string node_path;
      std::shared_ptr<NodeImpl> node;
   };

   struct MountNode
   {
      std::unordered_map<std::string, MountNode> nodes;
      std::vector<MountPoint> mount_points;
   };

   template<typename T>
   bool get_property_impl(const std::string& path, T& value) const;
   template<typename T>
   bool set_property_impl(const std::string& path, const T& value);

   mutable std::shared_mutex volumes_lock;
   MountNode volumes_root;
};

}

#endif
