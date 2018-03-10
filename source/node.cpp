#include <node.h>
#include "node_impl.h"

void Node::set_property(const std::string& name, int value)
{
   static_cast<NodeImpl*>(this)->set_property_impl(name, value);
}

void Node::set_property(const std::string& name, int64_t value)
{
   static_cast<NodeImpl*>(this)->set_property_impl(name, value);
}

void Node::set_property(const std::string& name, unsigned value)
{
   static_cast<NodeImpl*>(this)->set_property_impl(name, value);
}

void Node::set_property(const std::string& name, uint64_t value)
{
   static_cast<NodeImpl*>(this)->set_property_impl(name, value);
}

void Node::set_property(const std::string& name, float value)
{
   static_cast<NodeImpl*>(this)->set_property_impl(name, value);
}

void Node::set_property(const std::string& name, double value)
{
   static_cast<NodeImpl*>(this)->set_property_impl(name, value);
}

void Node::set_property(const std::string& name, long double value)
{
   static_cast<NodeImpl*>(this)->set_property_impl(name, value);
}

void Node::set_property(const std::string& name, const std::string& value)
{
   static_cast<NodeImpl*>(this)->set_property_impl(name, value);
}

void Node::set_property(const std::string& name, const std::vector<char>& value)
{
   static_cast<NodeImpl*>(this)->set_property_impl(name, BlobHolder(value));
}

void Node::set_property(const std::string& name, void* data, size_t size)
{
   static_cast<NodeImpl*>(this)->set_property_impl(name, BlobHolder(data, size));
}

bool Node::get_property(const std::string& name, int& value) const
{
   return static_cast<const NodeImpl*>(this)->get_property_impl(name, value);
}

bool Node::get_property(const std::string& name, unsigned& value) const
{
   return static_cast<const NodeImpl*>(this)->get_property_impl(name, value);
}

bool Node::get_property(const std::string& name, int64_t& value) const
{
   return static_cast<const NodeImpl*>(this)->get_property_impl(name, value);
}

bool Node::get_property(const std::string& name, uint64_t& value) const
{
   return static_cast<const NodeImpl*>(this)->get_property_impl(name, value);
}

bool Node::get_property(const std::string& name, float& value) const
{
   return static_cast<const NodeImpl*>(this)->get_property_impl(name, value);
}

bool Node::get_property(const std::string& name, double& value) const
{
   return static_cast<const NodeImpl*>(this)->get_property_impl(name, value);
}

bool Node::get_property(const std::string& name, long double& value) const
{
   return static_cast<const NodeImpl*>(this)->get_property_impl(name, value);
}

bool Node::get_property(const std::string& name, std::string& value) const
{
   return static_cast<const NodeImpl*>(this)->get_property_impl(name, value);
}

bool Node::get_property(const std::string & name, std::vector<char>& value) const
{
   return static_cast<const NodeImpl*>(this)->get_property_impl(name, value);
}

std::shared_ptr<Node> Node::add_child(const std::string& name)
{
   return static_cast<NodeImpl*>(this)->add_child_impl(name);
}

void Node::remove_child(const std::string& name)
{
   static_cast<NodeImpl*>(this)->remove_child_impl(name);
}

void Node::rename_child(const std::string& name, const std::string& new_name)
{
   static_cast<NodeImpl*>(this)->rename_child_impl(name, new_name);
}
