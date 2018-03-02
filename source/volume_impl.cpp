#include "volume_impl.h"
#include "node_impl.h"
#include "utility.h"

VolumeImpl::VolumeImpl()
{
   root = std::make_shared<NodeImpl>();
}

void VolumeImpl::set_storage(Storage* storage)
{
   this->storage = storage;
}

Storage* VolumeImpl::get_storage()
{
   return storage;
}

std::shared_ptr<NodeImpl> VolumeImpl::get_node(const std::string& path)
{
   std::shared_ptr<NodeImpl> node = root;

   size_t i_path = 0;
   while (i_path < path.length()) {
      std::string sub_key = find_next_sub_key(path, i_path);
      node = node->get_child(sub_key);
      if (node == nullptr) {
         return nullptr;
      }
   }

   return node;
}
