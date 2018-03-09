#ifndef HKEYSTORE_VOLUME_FILE_H
#define HKEYSTORE_VOLUME_FILE_H

#include <memory>
#include <array>
#include <istream>
#include <fstream>
#include <mutex>
#include <functional>

using record_id_t = uint64_t;

// Storage for records with an arbitrary size
//
// Implemented as a number of lists with blocks of the same size
// Best fit size is chosen for each record

class VolumeFile
{
   VolumeFile() = default;
public:
   static const int SIZES_COUNT = 38;
   static const std::array<size_t, SIZES_COUNT> RECORD_SIZES;

   VolumeFile(const VolumeFile&) = delete;
   void operator= (const VolumeFile&) = delete;

   static bool volume_file_exists(const std::string& path);
   static void create_new_volume_file(const std::string& path);
   static std::unique_ptr<VolumeFile> open_volume_file(const std::string& path);

   void read_record(record_id_t record_id, std::function<void(std::istream&)> read);
   void write_record(record_id_t record_id, const void* data, size_t size);

   record_id_t get_root_node_record_id();
   void set_root_node_record_id(record_id_t record_id);

   record_id_t allocate_record(const void* data, size_t size);
   void delete_record(record_id_t record_id);
   record_id_t resize_record(record_id_t record_id, const void* data, size_t size);

private:
   static const int CONTROL_BLOCK_SIZE = 4096;
   static const int FREE_RECORDS_BLOCK_RECORDS_COUNT = CONTROL_BLOCK_SIZE / sizeof(size_t) - 1;

   struct HeaderBlock
   {
      char signature[4];
      int32_t version;
      size_t free_records_block_offsets[SIZES_COUNT];
      size_t available_free_records_block_offset;
      record_id_t root_node_record_id;
      char padding[CONTROL_BLOCK_SIZE - 4 - sizeof(int32_t) - SIZES_COUNT * sizeof(size_t) - sizeof(size_t) - sizeof(uint64_t)];
   };

   static_assert(sizeof(HeaderBlock) == CONTROL_BLOCK_SIZE);

   struct FreeRecordsBlock
   {
      size_t free_records_offsets[FREE_RECORDS_BLOCK_RECORDS_COUNT];
      size_t next_free_records_block_offset;
   };

   static_assert(sizeof(FreeRecordsBlock) == CONTROL_BLOCK_SIZE);

   using lock_guard = std::lock_guard<std::recursive_mutex>;

   int find_best_fit_size(size_t node_size);

   void save_header_block();

   void allocate_free_records_block(int i_size);
   void load_free_records_block(int i_size);
   void save_free_records_block(int i_size);
   void next_free_records_block(int i_size);

   void write_padding(size_t size);

   record_id_t to_record_id(int i_size, size_t offset);
   void from_record_id(record_id_t node_id, int& i_size, size_t& offset);

   std::fstream file;
   size_t file_size;
   std::recursive_mutex lock;
   HeaderBlock header_block;
   std::array<FreeRecordsBlock, SIZES_COUNT> free_records_blocks;
};

#endif

