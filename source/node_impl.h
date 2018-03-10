#ifndef HKEYSTORE_NODE_IMPL_H
#define HKEYSTORE_NODE_IMPL_H

#include <vector>
#include <unordered_map>
#include <string>
#include <variant>
#include <memory>
#include <mutex>
#include <boost/variant.hpp>

#include <node.h>

#include "volume_file.h"
#include "blob_property.h"

class NodeImpl : public Node, public std::enable_shared_from_this<NodeImpl>
{
public:
   using child_id_t = uint64_t;

   // New node
   NodeImpl(child_id_t child_id, std::shared_ptr<NodeImpl> parent, std::shared_ptr<VolumeFile> volume_file);

   // Existing node
   NodeImpl(child_id_t child_id, std::shared_ptr<NodeImpl> parent, std::shared_ptr<VolumeFile> volume_file, record_id_t record_id);

   std::shared_ptr<NodeImpl> get_child(const std::string& name);
   std::shared_ptr<NodeImpl> add_node_impl(const std::string& name);
   std::shared_ptr<NodeImpl> get_node_impl(const std::string& path);
   void rename_child_impl(const std::string& name, const std::string& new_name);
   void remove_child_impl(const std::string& name);

   template<typename T> void set_property_impl(const std::string& name, const T& value);
   template<typename T> bool get_property_impl(const std::string& name, T& value) const;

private:
   using mutex = std::mutex;
   using lock_guard = std::lock_guard<mutex>;
   using PropertyValue = boost::variant<int, unsigned, int64_t, uint64_t, float, double, long double, std::string, BlobProperty>;

   struct ChildNode
   {
      record_id_t record_id;
      child_id_t child_id;
      mutable std::weak_ptr<NodeImpl> node;

      template<typename Archive>
      void serialize(Archive& archive, unsigned file_version);
   };

   friend struct NodeToDelete;

   static const uint64_t DELETED_NODE_RECORD_ID = record_id_t(-1);

   void save();
   void save_nodes();
   void load();
   void update();

   void delete_from_volume();
   void child_node_record_id_updated(child_id_t child_id, record_id_t new_record_id);

   mutable mutex lock;

   record_id_t record_id;
   child_id_t child_id;

   child_id_t next_child_id = 0;

   std::unordered_map<std::string, PropertyValue> properties;
   std::unordered_map<std::string, ChildNode> nodes;
   std::unordered_map<child_id_t, std::string> child_names_by_child_ids;

   std::shared_ptr<NodeImpl> parent;
   std::shared_ptr<VolumeFile> volume_file;
};

template<typename Archive>
inline void NodeImpl::ChildNode::serialize(Archive& archive, unsigned file_version)
{
   archive & record_id;
}

#endif

