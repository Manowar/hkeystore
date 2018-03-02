#ifndef HKEYSTORE_VOLUME_IMPL_H
#define HKEYSTORE_VOLUME_IMPL_H

#include <memory>

#include <storage.h>

#include "node_impl.h"

class VolumeImpl : public Volume
{
public:
   VolumeImpl();

   void set_storage(Storage* storage);
   Storage* get_storage();

   std::shared_ptr<NodeImpl> get_node(const std::string& path);
private:
   Storage* storage = nullptr;
   std::shared_ptr<NodeImpl> root;
};

#endif

