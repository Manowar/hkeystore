#ifndef HKEYSTORE_NODE_TO_REMOVE_KEY_H
#define HKEYSTORE_NODE_TO_REMOVE_KEY_H

#include <chrono>
#include <iostream>
#include "volume_file.h"
#include "serialization.h"

namespace hks {

struct node_to_remove_key_t
{
   using timepoint = std::chrono::time_point<std::chrono::system_clock>;

   node_to_remove_key_t();
   node_to_remove_key_t(timepoint time, node_id_t node_id);

   timepoint time;
   node_id_t node_id;

   bool operator < (const node_to_remove_key_t& rhs) const;

   void serialize(std::ostream& os) const
   {
      hks::serialize(os, time);
      hks::serialize(os, node_id);
   }

   void deserialize(std::istream& is)
   {
      hks::deserialize(is, time);
      hks::deserialize(is, node_id);
   }
};

inline node_to_remove_key_t::node_to_remove_key_t()
   : time()
   , node_id(0)
{
}

inline node_to_remove_key_t::node_to_remove_key_t(timepoint time, node_id_t node_id)
   : time(time)
   , node_id(node_id)
{
}

inline bool node_to_remove_key_t::operator < (const node_to_remove_key_t& rhs) const
{
   if (time < rhs.time) {
      return true;
   }
   if (time > rhs.time) {
      return false;
   }
   return node_id < rhs.node_id;
}

}

#endif
