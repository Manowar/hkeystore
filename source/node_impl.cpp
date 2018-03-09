#include <sstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/variant.hpp>
#include <errors.h>
#include "node_impl.h"
#include "utility.h"


NodeImpl::NodeImpl(child_id_t child_id, std::shared_ptr<NodeImpl> parent, std::shared_ptr<VolumeFile> volume_file)
   : child_id(child_id)
   , parent(parent)
   , volume_file(volume_file)
{
   save();

   lock_guard locker(lock);
}

NodeImpl::NodeImpl(child_id_t child_id, std::shared_ptr<NodeImpl> parent, std::shared_ptr<VolumeFile> volume_file, record_id_t record_id)
   : child_id(child_id)
   , parent(parent)
   , volume_file(volume_file)
   , record_id(record_id)
{
   load();

   lock_guard locker(lock);
}

std::shared_ptr<NodeImpl> NodeImpl::get_child(const std::string& name)
{
   lock_guard locker(lock);

   auto it = nodes.find(name);
   if (it == nodes.end()) {
      return nullptr;
   } 

   std::shared_ptr<NodeImpl> child = it->second.node.lock();
   if (!child) {
      // Node was not loaded. Load it.
      child = std::make_shared<NodeImpl>(it->second.child_id, shared_from_this(), volume_file, it->second.record_id);
      it->second.node = child;
   }

   return child;
}

struct RemoveBlobPropertyVisitor : public boost::static_visitor<void>
{
   RemoveBlobPropertyVisitor(std::shared_ptr<VolumeFile>& volume_file);

   template<typename T>
   void operator()(const T&) const {}
   void operator()(BlobPropery& blobProperty) const { blobProperty.remove(volume_file); }
private:
   std::shared_ptr<VolumeFile>& volume_file;
};

RemoveBlobPropertyVisitor::RemoveBlobPropertyVisitor(std::shared_ptr<VolumeFile>& volume_file)
   : volume_file(volume_file)
{
}

template<typename T>
void NodeImpl::set_property_impl(const std::string& name, const T& value)
{
   if (name.find('.') != std::string::npos) {
      throw LogicError("Can't add property with name '" + name + "'. Property names can't contain dots");
   }

   lock_guard locker(lock);
   auto it = properties.find(name);
   if (it == properties.end()) {
      properties.insert({ name, value });
   } else {
      boost::apply_visitor(RemoveBlobPropertyVisitor(volume_file), it->second);
      it->second = value;
   }
   update();
}

template<typename T>
bool NodeImpl::get_property_impl(const std::string& name, T& value)
{
   lock_guard locker(lock);
   auto it = properties.find(name);
   if (it == properties.end()) {
      return false;
   }
   
   return boost::apply_visitor([&](auto&& property_value) {
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

   child_id_t child_id = next_child_id++;

   auto new_node = std::make_shared<NodeImpl>(child_id, shared_from_this(), volume_file);

   ChildNode child_node;
   child_node.record_id = new_node->record_id;
   child_node.node = new_node;
   child_node.child_id = child_id;

   nodes.insert({ name, child_node });
   child_names_by_child_ids.insert({ child_id, name });

   update();

   return new_node;
}

std::shared_ptr<NodeImpl> NodeImpl::get_node_impl(const std::string& path)
{
   std::shared_ptr<NodeImpl> cur_node = shared_from_this();
   size_t i_path = 0;
   while (i_path < path.length()) {
      std::string sub_key = find_next_sub_key(path, i_path);
      cur_node = cur_node->get_child(sub_key);
      if (!cur_node) {
         return nullptr;
      }
   }
   return cur_node;
}

void NodeImpl::rename_child_impl(const std::string& name, const std::string& new_name)
{
   lock_guard locker(lock);

   auto it = nodes.find(name);
   if (it == nodes.end()) {
      throw NoSuchNode("Node with name '" + name + "' doesn't exist");
   }
   if (nodes.find(new_name) != nodes.end()) {
      throw NodeAlreadyExists("Node with name '" + name + "' already exists");
   }

   child_id_t child_id = it->second.child_id;

   auto child_node_handler = nodes.extract(it);
   child_node_handler.key() = new_name;
   nodes.insert(std::move(child_node_handler));

   child_names_by_child_ids[child_id] = new_name;

   update();
}

void NodeImpl::remove_child_impl(const std::string & name)
{
   lock_guard locker(lock);

   auto it = nodes.find(name);
   if (it == nodes.end()) {
      throw NoSuchNode("Node with name '" + name + "' doesn't exist");
   }

   std::shared_ptr<NodeImpl> removing_node = it->second.node.lock();
   if (removing_node == nullptr) {
      removing_node = std::make_shared<NodeImpl>(it->second.child_id, shared_from_this(), volume_file, it->second.record_id);
   }
   removing_node->delete_from_volume();

   child_id_t child_id = it->second.child_id;
   nodes.erase(it);
   child_names_by_child_ids.erase(child_id);

   update();
}

void NodeImpl::set_blob_property_impl(const std::string & name, const void* data, size_t size)
{
   if (name.find('.') != std::string::npos) {
      throw LogicError("Can't add property with name '" + name + "'. Property names can't contain dots");
   }

   lock_guard locker(lock);

   BlobPropery blob_property;
   blob_property.store(volume_file, data, size);

   auto it = properties.find(name);
   if (it == properties.end()) {
      properties.insert({ name, blob_property });
   } else {
      boost::apply_visitor(RemoveBlobPropertyVisitor(volume_file), it->second);
      it->second = blob_property;
   }
   update();
}

bool NodeImpl::get_blob_property_impl(const std::string & name, std::vector<char>& value)
{
   lock_guard locker(lock);
   auto it = properties.find(name);
   if (it == properties.end()) {
      return false;
   }

   BlobPropery blob_property;
   bool res = boost::apply_visitor([&](auto&& property_value) {
      return TypeConverter::convert<std::decay_t<decltype(property_value)>, BlobPropery>(property_value, blob_property);
   }, it->second);

   if (res) {
      value = blob_property.load(volume_file);
   }
   return res;
}

void NodeImpl::child_node_record_id_updated(child_id_t child_id, record_id_t new_record_id)
{
   lock_guard locker(lock);

   auto it = child_names_by_child_ids.find(child_id);
   if (it == child_names_by_child_ids.end()) {
      return;
   }

   const std::string& child_name = it->second;
   nodes[child_name].record_id = new_record_id;

   save_nodes();
}

void NodeImpl::save()
{
   if (record_id == DELETED_NODE_RECORD_ID) {
      return;
   }

   std::ostringstream os;
   boost::archive::binary_oarchive oa(os);
   oa & nodes;
   oa & properties;
   std::string data = os.str();
   record_id = volume_file->allocate_record(data.c_str(), data.length());

   if (!parent) {
      volume_file->set_root_node_record_id(record_id);
   }
}

void NodeImpl::save_nodes()
{
   if (record_id == DELETED_NODE_RECORD_ID) {
      return;
   }

   std::ostringstream os;
   boost::archive::binary_oarchive oa(os);
   oa & nodes;
   std::string data = os.str();
   volume_file->write_record(record_id, data.c_str(), data.length());
}

void NodeImpl::load()
{
   volume_file->read_record(record_id, [&](std::istream& is) {
      boost::archive::binary_iarchive ia(is);
      ia & nodes;
      ia & properties;
   });

   for (auto it = nodes.begin(); it != nodes.end(); ++it) {
      it->second.child_id = next_child_id++;
   }
}

void NodeImpl::update()
{
   if (record_id == DELETED_NODE_RECORD_ID) {
      return;
   }

   record_id_t old_record_id = record_id;
   save();
   if (old_record_id != record_id) {
      if (parent) {
         parent->child_node_record_id_updated(child_id, record_id);
      } else {
         volume_file->set_root_node_record_id(record_id);
      }
   }
}

struct NodeToDelete {
   std::shared_ptr<NodeImpl> node;
   bool children_added;

   NodeToDelete(std::shared_ptr<NodeImpl> node)
      : node(node)
      , children_added(false)
   {
      node->lock.lock();
   }

   NodeToDelete(const NodeToDelete&) = delete;
   NodeToDelete(NodeToDelete&&) = default;
   void operator=(const NodeToDelete&) = delete;

   ~NodeToDelete() {
      node->lock.unlock();
   }   
};

void NodeImpl::delete_from_volume()
{
   // Not using recursion due to possible large nodes depth, which could cause stack overflow

   std::vector<NodeToDelete> nodes_to_delete;
   nodes_to_delete.push_back(NodeToDelete(shared_from_this()));

   while (nodes_to_delete.size() > 0) {
      NodeToDelete& node_to_delete = nodes_to_delete.back();

      if (node_to_delete.node->record_id == DELETED_NODE_RECORD_ID) {
         nodes_to_delete.pop_back();
         continue;
      }

      if (!node_to_delete.children_added) {
         node_to_delete.children_added = true;
         std::shared_ptr<NodeImpl> node = node_to_delete.node;
         for (auto it = node->nodes.begin(); it != node->nodes.end(); ++it) {
            std::shared_ptr<NodeImpl> child = it->second.node.lock();
            if (!child) {
               child = std::make_shared<NodeImpl>(it->second.child_id, shared_from_this(), volume_file, it->second.record_id);
            }
            nodes_to_delete.push_back(NodeToDelete(child));
         }
         continue;
      }

      node_to_delete.node->volume_file->delete_record(node_to_delete.node->record_id);
      for (auto key_property : node_to_delete.node->properties) {
         boost::apply_visitor(RemoveBlobPropertyVisitor(node_to_delete.node->volume_file), key_property.second);
      }
      node_to_delete.node->record_id = DELETED_NODE_RECORD_ID;
      node_to_delete.node->volume_file = nullptr;

      nodes_to_delete.pop_back();
   }
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
