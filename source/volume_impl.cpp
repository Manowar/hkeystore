#include "volume_impl.h"
#include "node_impl.h"
#include "utility.h"

VolumeImpl::VolumeImpl(const std::string& volume_file_path, bool create_if_not_exist)
{
   if (create_if_not_exist) {
      if (!VolumeFile::volume_file_exists(volume_file_path)) {
         // Create new volume
         VolumeFile::create_new_volume_file(volume_file_path);
         std::shared_ptr<VolumeFile> volume_file = VolumeFile::open_volume_file(volume_file_path);
         root = std::make_shared<NodeImpl>(0, nullptr, volume_file);
         return;
      }
   }

   // Open existing volume
   std::shared_ptr<VolumeFile> volume_file = VolumeFile::open_volume_file(volume_file_path);
   root = std::make_shared<NodeImpl>(0, nullptr, volume_file, volume_file->get_root_node_record_id());
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
