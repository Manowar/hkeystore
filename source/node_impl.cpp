#include <sstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/variant.hpp>
#include <errors.h>
#include "node_impl.h"
#include "utility.h"

struct RemoveBlobPropertyVisitor : public boost::static_visitor<void>
{
   RemoveBlobPropertyVisitor(std::shared_ptr<VolumeFile>& volume_file);

   template<typename T>
   void operator()(const T&) const {}
   void operator()(BlobProperty& blobProperty) const { blobProperty.remove(volume_file); }
private:
   std::shared_ptr<VolumeFile>& volume_file;
};

RemoveBlobPropertyVisitor::RemoveBlobPropertyVisitor(std::shared_ptr<VolumeFile>& volume_file)
   : volume_file(volume_file)
{
}

NodeImpl::NodeImpl(std::shared_ptr<NodeImpl> parent, std::shared_ptr<VolumeFile> volume_file)
   : parent(parent)
   , volume_file(volume_file)
{
   node_id = volume_file->allocate_next_node_id();
   save();
}

NodeImpl::NodeImpl(std::shared_ptr<NodeImpl> parent, std::shared_ptr<VolumeFile> volume_file, record_id_t record_id)
   : parent(parent)
   , volume_file(volume_file)
   , record_id(record_id)
{
   load();
}

std::shared_ptr<NodeImpl> NodeImpl::get_child_impl(const std::string& name)
{
   lock_guard locker(lock);

   auto it = nodes.find(name);
   if (it == nodes.end()) {
      return nullptr;
   } 

   std::shared_ptr<NodeImpl> child = it->second.node.lock();
   if (!child) {
      // Node was not loaded. Load it.
      child = std::make_shared<NodeImpl>(shared_from_this(), volume_file, it->second.record_id);
      it->second.node = child;
      assert(it->second.node_id == child->node_id);
   }

   return child;
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

bool NodeImpl::remove_property_impl(const std::string& name)
{
   lock_guard locker(lock);

   auto it = properties.find(name);
   if (it == properties.end()) {
      return false;
   }

   boost::apply_visitor(RemoveBlobPropertyVisitor(volume_file), it->second);
   properties.erase(it);

   update();
   return true;
}

void NodeImpl::set_time_to_live(std::chrono::milliseconds time)
{
   lock_guard locker(lock);
   if (parent == nullptr) {
      throw LogicError("Can't delete root node");
   }
   time_to_remove = std::chrono::system_clock::now() + time;
   update();
}

template<>
void NodeImpl::set_property_impl<BlobHolder>(const std::string& name, const BlobHolder& blob)
{
   if (name.find('.') != std::string::npos) {
      throw LogicError("Can't add property with name '" + name + "'. Property names can't contain dots");
   }

   lock_guard locker(lock);

   BlobProperty blob_property;
   blob_property.store(volume_file, blob.data, blob.size);

   auto it = properties.find(name);
   if (it == properties.end()) {
      properties.insert({ name, blob_property });
   }
   else {
      boost::apply_visitor(RemoveBlobPropertyVisitor(volume_file), it->second);
      it->second = blob_property;
   }
   update();
}

template<typename T>
bool NodeImpl::get_property_impl(const std::string& name, T& value) const
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

template<>
bool NodeImpl::get_property_impl<std::vector<char>>(const std::string& name, std::vector<char>& value) const
{
   lock_guard locker(lock);
   auto it = properties.find(name);
   if (it == properties.end()) {
      return false;
   }

   BlobProperty blob_property;
   bool res = boost::apply_visitor([&](auto&& property_value) {
      return TypeConverter::convert<std::decay_t<decltype(property_value)>, BlobProperty>(property_value, blob_property);
   }, it->second);

   if (res) {
      value = blob_property.load(volume_file);
   }
   return res;
}

std::shared_ptr<NodeImpl> NodeImpl::add_child_impl(const std::string& name)
{
   if (name.find('.') != std::string::npos) {
      throw LogicError("Can't add node with name '" + name + "'. Node names can't contain dots");
   }

   lock_guard locker(lock);
   if (nodes.find(name) != nodes.end()) {
      throw NodeAlreadyExists("Node " + name + " already exists.");
   }

   auto new_node = std::make_shared<NodeImpl>(shared_from_this(), volume_file);

   ChildNode child_node;
   child_node.record_id = new_node->record_id;
   child_node.node = new_node;
   child_node.node_id = new_node->node_id;

   nodes.insert({ name, child_node });
   child_names_by_ids.insert({ child_node.node_id, name });

   update();

   return new_node;
}

std::shared_ptr<NodeImpl> NodeImpl::get_node_impl(const std::string& path)
{
   std::shared_ptr<NodeImpl> cur_node = shared_from_this();
   size_t i_path = 0;
   while (i_path < path.length()) {
      std::string sub_key = find_next_sub_key(path, i_path);
      cur_node = cur_node->get_child_impl(sub_key);
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

   node_id_t child_node_id = it->second.node_id;

   auto child_node_handler = nodes.extract(it);
   child_node_handler.key() = new_name;
   nodes.insert(std::move(child_node_handler));

   child_names_by_ids[child_node_id] = new_name;

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
      removing_node = std::make_shared<NodeImpl>(shared_from_this(), volume_file, it->second.record_id);
   }
   removing_node->delete_from_volume();

   node_id_t child_node_id = it->second.node_id;
   nodes.erase(it);
   child_names_by_ids.erase(child_node_id);

   update();
}

bool NodeImpl::is_deleted_impl() const
{
   lock_guard locker(lock);

   return record_id == DELETED_NODE_RECORD_ID;
}


void NodeImpl::child_node_record_id_updated(node_id_t child_node_id, record_id_t new_record_id)
{
   lock_guard locker(lock);

   auto it = child_names_by_ids.find(child_node_id);
   if (it == child_names_by_ids.end()) {
      // Child has been deleted
      return;
   }

   const std::string& child_name = it->second;
   nodes[child_name].record_id = new_record_id;

   save_nodes();
}

std::vector<node_id_t> NodeImpl::get_unique_node_path()
{
   std::vector<std::shared_ptr<NodeImpl>> path_to_root;
   std::shared_ptr<NodeImpl> node = shared_from_this();
   while (node) {
      path_to_root.push_back(node);
      node = node->parent;
   }

   std::vector<node_id_t> path;
   for (size_t i = path_to_root.size(); i-- > 0; ) {
      path.push_back(path_to_root[i]->node_id);
   }
   return path;
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
   oa & node_id;
   oa & time_to_remove;
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
      ia & node_id;
      ia & time_to_remove;
   });

   for (auto it = nodes.begin(); it != nodes.end(); ++it) {
      child_names_by_ids.insert({ it->second.node_id, it->first });
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
         parent->child_node_record_id_updated(node_id, record_id);
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
      if (node) {
         node->lock.unlock();
      }
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
               child = std::make_shared<NodeImpl>(shared_from_this(), volume_file, it->second.record_id);
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

template bool NodeImpl::get_property_impl<int>(const std::string& name, int& value) const;
template bool NodeImpl::get_property_impl<unsigned>(const std::string& name, unsigned& value) const;
template bool NodeImpl::get_property_impl<int64_t>(const std::string& name, int64_t& value) const;
template bool NodeImpl::get_property_impl<uint64_t>(const std::string& name, uint64_t& value) const;
template bool NodeImpl::get_property_impl<float>(const std::string& name, float& value) const;
template bool NodeImpl::get_property_impl<double>(const std::string& name, double& value) const;
template bool NodeImpl::get_property_impl<long double>(const std::string& name, long double& value) const;
template bool NodeImpl::get_property_impl<std::string>(const std::string& name, std::string& value) const;
template bool NodeImpl::get_property_impl<std::vector<char>>(const std::string& name, std::vector<char>& value) const;

template void NodeImpl::set_property_impl<int>(const std::string& name, const int& value);
template void NodeImpl::set_property_impl<unsigned>(const std::string& name, const unsigned& value);
template void NodeImpl::set_property_impl<int64_t>(const std::string& name, const int64_t& value);
template void NodeImpl::set_property_impl<uint64_t>(const std::string& name, const uint64_t& value);
template void NodeImpl::set_property_impl<float>(const std::string& name, const float& value);
template void NodeImpl::set_property_impl<double>(const std::string& name, const double& value);
template void NodeImpl::set_property_impl<long double>(const std::string& name, const long double& value);
template void NodeImpl::set_property_impl<std::string>(const std::string& name, const std::string& value);
template void NodeImpl::set_property_impl<BlobHolder>(const std::string& name, const BlobHolder& value);
