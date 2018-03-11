#include "time_to_live_manager.h"
#include "volume_impl.h"

namespace hks {

TimeToLiveManager::TimeToLiveManager(std::unique_ptr<NodesToRemoveTree>&& nodes_to_remove_tree, VolumeImpl* volume_impl)
   : nodes_to_remove_tree(std::move(nodes_to_remove_tree))
   , volume_impl(volume_impl)
{
   thread = std::thread(&TimeToLiveManager::worker_function, this);
}

TimeToLiveManager::~TimeToLiveManager()
{
   {
      lock_guard locker(lock);
      exit = true;
   }

   work_ready.notify_all();
   thread.join();
}

void TimeToLiveManager::worker_function()
{
   while (!exit) {
      node_to_remove_key_t node_to_remove_key;
      std::vector<node_id_t> node_path_to_remove;

      {
         std::unique_lock<std::mutex> locker(lock);
         bool res = nodes_to_remove_tree->get_first(&node_to_remove_key, &node_path_to_remove);
         if (!res) {
            next_time_to_remove = timepoint();
            work_ready.wait(locker);
            continue;
         }

         duration remove_after = node_to_remove_key.time - timepoint::clock::now();
         if (remove_after > duration()) {
            next_time_to_remove = node_to_remove_key.time;
            if (work_ready.wait_for(locker, remove_after) == std::cv_status::no_timeout) {
               continue;
            }
         }
      }

      volume_impl->remove_node(node_path_to_remove);

      {
         lock_guard locker(lock);
         next_time_to_remove = timepoint();
         nodes_to_remove_tree->remove(node_to_remove_key);
      }
   }
}

void TimeToLiveManager::set_time_to_remove(const std::vector<node_id_t>& node_path, timepoint time_to_remove, timepoint previous_time_to_remove)
{
   lock_guard locker(lock);
   if (previous_time_to_remove != timepoint()) {
      nodes_to_remove_tree->remove(node_to_remove_key_t(previous_time_to_remove, node_path.back()));
      if (next_time_to_remove == previous_time_to_remove) {
         work_ready.notify_all();
      }
   }
   if (time_to_remove != timepoint()) {
      nodes_to_remove_tree->insert(node_to_remove_key_t(time_to_remove, node_path.back()), node_path);
      if (next_time_to_remove == timepoint() || next_time_to_remove > time_to_remove) {
         work_ready.notify_all();
      }
   }
}

}

