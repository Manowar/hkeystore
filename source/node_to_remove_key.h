#ifndef HKEYSTORE_NODE_TO_REMOVE_KEY_H
#define HKEYSTORE_NODE_TO_REMOVE_KEY_H

#include <chrono>
#include "volume_file.h"

struct node_to_remove_key_t
{
   using timepoint = std::chrono::time_point<std::chrono::system_clock>;

   node_to_remove_key_t();
   node_to_remove_key_t(timepoint time, node_id_t node_id);

   timepoint time;
   node_id_t node_id;

   bool operator < (const node_to_remove_key_t& rhs) const;

   template<class Archive>
   void serialize(Archive& ar, unsigned version);
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

template<class Archive>
void node_to_remove_key_t::serialize(Archive& ar, unsigned version)
{
   ar & time;
   ar & node_id;
}

#endif
