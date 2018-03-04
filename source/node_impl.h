#ifndef HKEYSTORE_NODE_IMPL_H
#define HKEYSTORE_NODE_IMPL_H

#include <unordered_map>
#include <string>
#include <variant>
#include <memory>
#include <mutex>
#include <boost/variant.hpp>

#include <node.h>

#include "volume_file.h"

class NodeImpl : public Node, public std::enable_shared_from_this<NodeImpl>
{
public:
   // New node
   NodeImpl(const std::string& name, std::shared_ptr<NodeImpl> parent, std::shared_ptr<VolumeFile> volume_file);

   // Existing node
   NodeImpl(const std::string& name, std::shared_ptr<NodeImpl> parent, std::shared_ptr<VolumeFile> volume_file, uint64_t node_id);

   std::shared_ptr<NodeImpl> get_child(const std::string& name);
   std::shared_ptr<NodeImpl> add_node_impl(const std::string& name);
   std::shared_ptr<NodeImpl> get_node_impl(const std::string& path);

   template<typename T> void set_property_impl(const std::string& name, const T& value);
   template<typename T> bool get_property_impl(const std::string& name, T& value);

   void child_node_id_updated(const std::string& name, uint64_t new_node_id);

private:
   using mutex = std::mutex;
   using lock_guard = std::lock_guard<mutex>;
   using PropertyValue = boost::variant<int, int64_t, float, double, long double, std::string>;

   struct ChildNode
   {
      uint64_t node_id;
      std::weak_ptr<NodeImpl> node;

      template<typename Archive>
      void serialize(Archive& archive, unsigned file_version);
   };

   void save();
   void save_nodes();
   void load();
   void update();

   mutex lock;

   std::string name;

   uint64_t node_id;
   std::unordered_map<std::string, PropertyValue> properties;
   std::unordered_map<std::string, ChildNode> nodes;

   std::shared_ptr<NodeImpl> parent;
   std::shared_ptr<VolumeFile> volume_file;
};

template<typename Archive>
inline void NodeImpl::ChildNode::serialize(Archive& archive, unsigned file_version)
{
   archive & node_id;
}

#endif

