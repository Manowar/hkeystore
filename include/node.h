#ifndef HKEYSTORE_NODE_H
#define HKEYSTORE_NODE_H

#include <string>
#include <memory>
#include <vector>

class Node
{
public:
   Node() = default;
   Node(const Node&) = delete;
   void operator=(const Node&) = delete;

   std::shared_ptr<Node> get_child(const std::string& name);
   std::shared_ptr<Node> add_child(const std::string& name);
   void remove_child(const std::string& name);
   void rename_child(const std::string& name, const std::string& new_name);

   bool is_deleted() const;

   bool get_property(const std::string& name, int& value) const;
   bool get_property(const std::string& name, int64_t& value) const;
   bool get_property(const std::string& name, unsigned& value) const;
   bool get_property(const std::string& name, uint64_t& value) const;
   bool get_property(const std::string& name, float& value) const;
   bool get_property(const std::string& name, double& value) const;
   bool get_property(const std::string& name, long double& value) const;
   bool get_property(const std::string& name, std::string& value) const;
   bool get_property(const std::string& name, std::vector<char>& value) const;

   void set_property(const std::string& name, int value);
   void set_property(const std::string& name, int64_t value);
   void set_property(const std::string& name, unsigned value);
   void set_property(const std::string& name, uint64_t value);
   void set_property(const std::string& name, float value);
   void set_property(const std::string& name, double value);
   void set_property(const std::string& name, long double value);
   void set_property(const std::string& name, const std::string& value);
   void set_property(const std::string& name, const std::vector<char>& value);
   void set_property(const std::string& name, void* data, size_t size);

   bool remove_property(const std::string& name);
};

#endif