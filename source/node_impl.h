#ifndef HKEYSTORE_NODE_IMPL_H
#define HKEYSTORE_NODE_IMPL_H

#include <vector>
#include <unordered_map>
#include <string>
#include <variant>
#include <memory>
#include <mutex>
#include <chrono>

#include <node.h>

#include "volume_impl.h"
#include "blob_property.h"

namespace hks {

class NodeImpl : public Node, public std::enable_shared_from_this<NodeImpl>
{
public:
   using PropertyValue = std::variant<int, unsigned, int64_t, uint64_t, float, double, long double, std::string, BlobProperty>;

   // New node
   NodeImpl(std::shared_ptr<NodeImpl> parent, VolumeImpl* volume_impl);

   // Existing node
   NodeImpl(std::shared_ptr<NodeImpl> parent, VolumeImpl* volume_impl, record_id_t record_id);

   std::shared_ptr<NodeImpl> get_child_impl(const std::string& name);
   std::shared_ptr<NodeImpl> add_child_impl(const std::string& name);
   std::shared_ptr<NodeImpl> get_node_impl(const std::string& path);
   void rename_child_impl(const std::string& name, const std::string& new_name);
   void remove_child_impl(const std::string& name);

   bool is_deleted_impl() const;

   node_id_t get_node_id() const;
   std::shared_ptr<NodeImpl> get_child_impl(node_id_t node_id);
   bool remove_child_impl(node_id_t node_id);

   template<typename T> void set_property_impl(const std::string& name, const T& value);
   template<typename T> bool get_property_impl(const std::string& name, T& value) const;
   bool remove_property_impl(const std::string& name);

   void set_time_to_live(std::chrono::milliseconds time);

private:
   using mutex = std::mutex;
   using lock_guard = std::lock_guard<mutex>;
   using timepoint = std::chrono::time_point<std::chrono::system_clock>;

   struct ChildNode
   {
      record_id_t record_id;
      node_id_t node_id;
      mutable std::weak_ptr<NodeImpl> node;

      void serialize(std::ostream& os) const;
      void deserialize(std::istream& os);
   };

   friend struct NodeToDelete;

   static const uint64_t DELETED_NODE_RECORD_ID = record_id_t(-1);

   void save(bool create_new);
   void save_nodes();
   void load();
   void update();

   void delete_from_volume();

   void child_node_record_id_updated(node_id_t child_node_id, record_id_t new_record_id);

   std::vector<node_id_t> get_unique_node_path();

   std::shared_ptr<NodeImpl> do_get_child(const std::string& name);
   void do_remove_child(const std::string& name);

   mutable mutex lock;

   record_id_t record_id;
   node_id_t node_id;
   timepoint time_to_remove;

   std::unordered_map<std::string, ChildNode> nodes;
   std::unordered_map<std::string, PropertyValue> properties;
   std::unordered_map<node_id_t, std::string> child_names_by_ids;

   std::shared_ptr<NodeImpl> parent;
   VolumeImpl* volume_impl;
};


inline void NodeImpl::ChildNode::serialize(std::ostream& os) const
{
   hks::serialize(os, record_id);
   hks::serialize(os, node_id);
}

inline void NodeImpl::ChildNode::deserialize(std::istream& is)
{
   hks::deserialize(is, record_id);
   hks::deserialize(is, node_id);
}

}

#endif

