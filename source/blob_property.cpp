#include "blob_property.h"

BlobProperty::BlobProperty()
   : size(0)
   , record_id(INVALID_RECORD_ID)
{
}

std::vector<char> BlobProperty::load(std::shared_ptr<VolumeFile> volume_file) 
{
   std::vector<char> res;
   res.resize(size);

   volume_file->read_record(record_id, [&](std::istream& is) {
      is.read(&res[0], size);
   });

   return res;
}

void BlobProperty::store(std::shared_ptr<VolumeFile> volume_file, const void* data, size_t size)
{
   this->size = size;

   if (record_id == INVALID_RECORD_ID) {
      record_id = volume_file->allocate_record(data, size);
   } else {
      record_id = volume_file->resize_record(record_id, data, size);
   }
}

void BlobProperty::remove(std::shared_ptr<VolumeFile> volume_file)
{
   volume_file->delete_record(record_id);

   size = 0;
   record_id = INVALID_RECORD_ID;
}

BlobHolder::BlobHolder(const void* data, size_t size)
   : data(data)
   , size(size)
{
}

BlobHolder::BlobHolder(const std::vector<char>& data)
   : data(&data[0])
   , size(data.size())
{
}
