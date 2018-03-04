#ifndef HKEYSTORE_STORAGE_H
#define HKEYSTORE_STORAGE_H


#include <memory>
#include <string>
#include <shared_mutex>
#include <unordered_map>

#include <volume.h>

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

   std::shared_ptr<Node> get_node(const std::string& path);

   template<typename T> bool get_property(const std::string& path, T& value);
   template<typename T> bool set_property(const std::string& path, const T& value);

private:
   struct MountNode
   {
      std::unordered_map<std::string, MountNode> nodes;
      std::vector<std::shared_ptr<NodeImpl>> volumes;
   };

   std::shared_mutex volumes_lock;
   MountNode volumes_root;
};

#endif
