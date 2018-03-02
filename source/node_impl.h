#ifndef HKEYSTORE_NODE_IMPL_H
#define HKEYSTORE_NODE_IMPL_H

#include <unordered_map>
#include <string>
#include <variant>
#include <memory>
#include <mutex>
#include <boost/smart_ptr/detail/spinlock.hpp>

#include <node.h>


class NodeImpl : public Node
{
public:   
   std::shared_ptr<NodeImpl> get_child(const std::string& name);

   std::shared_ptr<NodeImpl> add_node_impl(const std::string& name);
   static std::shared_ptr<NodeImpl> get_node_impl(std::shared_ptr<NodeImpl> node, const std::string& path);

   template<typename T> void set_property_impl(const std::string& name, const T& value);
   template<typename T> bool get_property_impl(const std::string& name, T& value);

private:
   using spinlock = boost::detail::spinlock;
   using lock_guard = std::lock_guard<spinlock>;
   using PropertyValue = std::variant<int, int64_t, float, double, long double, std::string>;

   spinlock lock;

   std::unordered_map<std::string, std::shared_ptr<NodeImpl>> nodes;
   std::unordered_map<std::string, PropertyValue> properties;
};


#endif
