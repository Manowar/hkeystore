#ifndef HKEYSTORE_BLOB_PROPERTY_H
#define HKEYSTORE_BLOB_PROPERTY_H

#include <vector>
#include "volume_file.h"
#include "serialization.h"

namespace hks {

struct BlobHolder {
   const void* data;
   size_t size;

   BlobHolder(const void* data, size_t size);
   BlobHolder(const std::vector<char>& data);
};

class BlobProperty {
public:
   BlobProperty();

   std::vector<char> load(std::shared_ptr<VolumeFile> volume_file);
   void store(std::shared_ptr<VolumeFile> volume_file, const void* data, size_t size);
   void remove(std::shared_ptr<VolumeFile> volume_file);

   void serialize(std::ostream& os) const;
   void deserialize(std::istream& is);

private:
   static const record_id_t INVALID_RECORD_ID = record_id_t(-1);

   size_t size;
   record_id_t record_id;
};

inline void BlobProperty::serialize(std::ostream& os) const
{
   hks::serialize(os, size);
   hks::serialize(os, record_id);
}

inline void BlobProperty::deserialize(std::istream& is) 
{
   hks::deserialize(is, size);
   hks::deserialize(is, record_id);
}

}

#endif
