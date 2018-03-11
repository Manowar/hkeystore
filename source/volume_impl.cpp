#include "volume_impl.h"
#include "node_impl.h"
#include "utility.h"

VolumeImpl::VolumeImpl(const std::string& volume_file_path, bool create_if_not_exist)
{
   if (create_if_not_exist) {
      if (!VolumeFile::volume_file_exists(volume_file_path)) {
         // Create new volume
         VolumeFile::create_new_volume_file(volume_file_path);
         volume_file = VolumeFile::open_volume_file(volume_file_path);
         root = std::make_shared<NodeImpl>(nullptr, this);
         std::unique_ptr<NodesToRemoveTree> nodes_to_remove_tree = std::make_unique<NodesToRemoveTree>(volume_file);
         volume_file->set_bplus_tree_record_id(nodes_to_remove_tree->get_record_id());
         time_to_live_manager = std::make_unique<TimeToLiveManager>(std::move(nodes_to_remove_tree));
         return;
      }
   }

   // Open existing volume
   volume_file = VolumeFile::open_volume_file(volume_file_path);
   root = std::make_shared<NodeImpl>(nullptr, this, volume_file->get_root_node_record_id());
   std::unique_ptr<NodesToRemoveTree> nodes_to_remove_tree = std::make_unique<NodesToRemoveTree>(volume_file, volume_file->get_bplus_tree_record_id());
   time_to_live_manager = std::make_unique<TimeToLiveManager>(std::move(nodes_to_remove_tree));
}

void VolumeImpl::set_storage(Storage* storage)
{
   this->storage = storage;
}

Storage* VolumeImpl::get_storage()
{
   return storage;
}

TimeToLiveManager * VolumeImpl::get_time_to_live_manager()
{
   return time_to_live_manager.get();
}

std::shared_ptr<VolumeFile> VolumeImpl::get_volume_file()
{
   return volume_file;
}

std::shared_ptr<NodeImpl> VolumeImpl::get_node(const std::string& path)
{
   std::shared_ptr<NodeImpl> node = root;

   size_t i_path = 0;
   while (i_path < path.length()) {
      std::string sub_key = find_next_sub_key(path, i_path);
      node = node->get_child_impl(sub_key);
      if (node == nullptr) {
         return nullptr;
      }
   }

   return node;
}
