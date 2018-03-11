#ifndef HKEYSTORE_TIME_TO_LIVE_MANAGER_H
#define HKEYSTORE_TIME_TO_LIVE_MANAGER_H

#include <memory>
#include <chrono>

#include "node_impl.h"

class TimeToLiveManager
{
public:
   using timepoint = std::chrono::time_point<std::chrono::system_clock>;

   TimeToLiveManager(const TimeToLiveManager&) = delete;
   void operator=(const TimeToLiveManager&) = delete;

   void set_time_to_remove(const std::vector<node_id_t>& node_path, timepoint time_to_remove, timepoint previous_time_to_remove);
};

#endif
