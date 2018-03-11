#ifndef HKEYSTORE_TIME_TO_LIVE_MANAGER_H
#define HKEYSTORE_TIME_TO_LIVE_MANAGER_H

#include <memory>
#include <chrono>
#include <thread>
#include <mutex>

#include "bplus_tree.h"
#include "node_to_remove_key.h"

namespace hks {

class VolumeImpl;

class TimeToLiveManager
{
public:
   using NodesToRemoveTree = BplusTree<node_to_remove_key_t, std::vector<node_id_t>>;
   using timepoint = node_to_remove_key_t::timepoint;
   using duration = timepoint::duration;

   TimeToLiveManager(std::unique_ptr<NodesToRemoveTree>&& nodes_to_remove_tree, VolumeImpl* volume_impl);
   ~TimeToLiveManager();

   TimeToLiveManager(const TimeToLiveManager&) = delete;
   void operator=(const TimeToLiveManager&) = delete;

   void set_time_to_remove(const std::vector<node_id_t>& node_path, timepoint time_to_remove, timepoint previous_time_to_remove);

   void worker_function();

private:
   using lock_guard = std::lock_guard<std::mutex>;

   std::mutex lock;
   std::condition_variable work_ready;
   bool exit = false;
   std::unique_ptr<NodesToRemoveTree> nodes_to_remove_tree;
   std::thread thread;
   VolumeImpl* volume_impl;
   timepoint next_time_to_remove;
};

}

#endif
