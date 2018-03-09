#ifndef HKEYSTORE_BLOB_PROPERTY_H
#define HKEYSTORE_BLOB_PROPERTY_H

#include <vector>
#include "volume_file.h"

class BlobPropery {
public:
   BlobPropery();

   std::vector<char> load(std::shared_ptr<VolumeFile>& volume_file);
   void store(std::shared_ptr<VolumeFile>& volume_file, const void* data, size_t size);
   void remove(std::shared_ptr<VolumeFile>& volume_file);

   template<typename Archive> 
   void serialize(Archive& archive, unsigned file_version);

private:
   static const record_id_t INVALID_RECORD_ID = record_id_t(-1);

   size_t size;
   record_id_t record_id;
};

#endif

template<typename Archive>
inline void BlobPropery::serialize(Archive & archive, unsigned file_version)
{
   archive & size;
   archive & record_id;
}
