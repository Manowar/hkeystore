#ifndef HKEYSTORE_VOLUME_IMPL_H
#define HKEYSTORE_VOLUME_IMPL_H

#include <memory>

#include <storage.h>

#include "node_impl.h"
#include "time_to_live_manager.h"
#include "bplus_tree.h"

namespace hks {

class VolumeImpl : public Volume
{
public:
   VolumeImpl(const std::string& volume_file_path, bool create_if_not_exist);

   void set_storage(Storage* storage);
   Storage* get_storage();

   TimeToLiveManager* get_time_to_live_manager();
   std::shared_ptr<VolumeFile> get_volume_file();

   std::shared_ptr<NodeImpl> get_node(const std::string& path);
   bool remove_node(const std::vector<node_id_t>& path_to_remove);

private:
   using NodesToRemoveTree = TimeToLiveManager::NodesToRemoveTree;

   Storage* storage = nullptr;

   std::shared_ptr<NodeImpl> root;
   std::unique_ptr<TimeToLiveManager> time_to_live_manager;
   std::shared_ptr<VolumeFile> volume_file;
};

}

#endif

