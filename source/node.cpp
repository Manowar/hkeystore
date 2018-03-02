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
   static_cast<NodeImpl*>(this)->set_property_impl(name, (int)value);
}

void Node::set_property(const std::string& name, uint64_t value)
{
   static_cast<NodeImpl*>(this)->set_property_impl(name, (int64_t)value);
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

bool Node::get_property(const std::string& name, int& value)
{
   return static_cast<NodeImpl*>(this)->get_property_impl(name, value);
}

bool Node::get_property(const std::string& name, unsigned& value)
{
   int i_val;
   bool res = static_cast<NodeImpl*>(this)->get_property_impl(name, i_val);
   if (res) {
      value = i_val;
      return true;
   }
   return false;
}

bool Node::get_property(const std::string& name, int64_t& value)
{
   return static_cast<NodeImpl*>(this)->get_property_impl(name, value);
}

bool Node::get_property(const std::string& name, uint64_t& value)
{
   int64_t i64_val;
   bool res = static_cast<NodeImpl*>(this)->get_property_impl(name, i64_val);
   if (res) {
      value = i64_val;
      return true;
   }
   return false;
}

bool Node::get_property(const std::string& name, float& value)
{
   return static_cast<NodeImpl*>(this)->get_property_impl(name, value);
}

bool Node::get_property(const std::string& name, double& value)
{
   return static_cast<NodeImpl*>(this)->get_property_impl(name, value);
}

bool Node::get_property(const std::string& name, long double& value)
{
   return static_cast<NodeImpl*>(this)->get_property_impl(name, value);
}

bool Node::get_property(const std::string& name, std::string& value)
{
   return static_cast<NodeImpl*>(this)->get_property_impl(name, value);
}

std::shared_ptr<Node> Node::add_node(const std::string& name)
{
   return static_cast<NodeImpl*>(this)->add_node_impl(name);
}