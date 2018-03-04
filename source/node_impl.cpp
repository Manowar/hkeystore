#include <sstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/variant.hpp>
#include <errors.h>
#include "node_impl.h"
#include "utility.h"


NodeImpl::NodeImpl(const std::string& name, std::shared_ptr<NodeImpl> parent, std::shared_ptr<VolumeFile> volume_file)
   : name(name)
   , parent(parent)
   , volume_file(volume_file)
{
   save();

   lock_guard locker(lock);
}

NodeImpl::NodeImpl(const std::string& name, std::shared_ptr<NodeImpl> parent, std::shared_ptr<VolumeFile> volume_file, uint64_t node_id)
   : name(name)
   , parent(parent)
   , volume_file(volume_file)
   , node_id(node_id)
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
      child = std::make_shared<NodeImpl>(name, shared_from_this(), volume_file, it->second.node_id);
      it->second.node = child;
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

   properties[name] = value;
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

   auto new_node = std::make_shared<NodeImpl>(name, shared_from_this(), volume_file);

   ChildNode child_node;
   child_node.node_id = new_node->node_id;
   child_node.node = new_node;
   nodes.insert({ name, child_node });

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

void NodeImpl::child_node_id_updated(const std::string& name, uint64_t new_node_id)
{
   lock_guard locker(lock);

   auto it = nodes.find(name);
   // TODO: a new node with the same name may already exist, old node may be already deleted
   // Need to lock parent node on child node operations
   if (it == nodes.end()) {
      return;
   }
   it->second.node_id = new_node_id;

   save_nodes();
}

void NodeImpl::save()
{
   std::ostringstream os;
   boost::archive::binary_oarchive oa(os);
   oa & nodes;
   oa & properties;
   std::string data = os.str();
   node_id = volume_file->allocate_node(data.c_str(), data.length());

   if (!parent) {
      volume_file->set_root_node_id(node_id);
   }
}

void NodeImpl::save_nodes()
{
   std::ostringstream os;
   boost::archive::binary_oarchive oa(os);
   oa & nodes;
   std::string data = os.str();
   volume_file->write_node(node_id, data.c_str(), data.length());
}

void NodeImpl::load()
{
   volume_file->read_node(node_id, [&](std::istream& is) {
      boost::archive::binary_iarchive ia(is);
      ia & nodes;
      ia & properties;
   });
}

void NodeImpl::update()
{
   uint64_t old_node_id = node_id;
   save();
   if (old_node_id != node_id) {
      if (parent) {
         parent->child_node_id_updated(name, node_id);
      } else {
         volume_file->set_root_node_id(node_id);
      }
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


