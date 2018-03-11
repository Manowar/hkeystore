#include "time_to_live_manager.h"

TimeToLiveManager::TimeToLiveManager(std::unique_ptr<NodesToRemoveTree>&& nodes_to_remove_tree)
   : nodes_to_remove_tree(std::move(nodes_to_remove_tree))
{
}

void TimeToLiveManager::set_time_to_remove(const std::vector<node_id_t>& node_path, timepoint time_to_remove, timepoint previous_time_to_remove)
{
}
