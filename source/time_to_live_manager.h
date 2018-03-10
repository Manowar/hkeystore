#ifndef HKEYSTORE_TIME_TO_LIVE_MANAGER_H
#define HKEYSTORE_TIME_TO_LIVE_MANAGER_H

#include <memory>
#include <chrono>

#include "node_impl.h"

class TimeToLiveManager
{
   TimeToLiveManager(const TimeToLiveManager&) = delete;
   void operator=(const TimeToLiveManager&) = delete;

   void set_time_to_live(std::shared_ptr<NodeImpl> node, std::chrono::milliseconds time);
};

#endif
