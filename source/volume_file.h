#ifndef HKEYSTORE_VOLUME_FILE_H
#define HKEYSTORE_VOLUME_FILE_H

#include <memory>
#include <array>
#include <istream>
#include <fstream>
#include <mutex>
#include <functional>

class VolumeFile
{
   VolumeFile() = default;
public:
   VolumeFile(const VolumeFile&) = delete;
   void operator= (const VolumeFile&) = delete;

   static bool volume_file_exists(const std::string& path);
   static void create_new_volume_file(const std::string& path);
   static std::unique_ptr<VolumeFile> open_volume_file(const std::string& path);

   void read_node(uint64_t node_id, std::function<void(std::istream&)> read);
   void write_node(uint64_t node_id, const void* data, size_t size);

   uint64_t get_root_node_id();
   void set_root_node_id(uint64_t root_node_id);

   uint64_t allocate_node(const void* data, size_t size);
   void remove_node(uint64_t node_id);
   uint64_t resize_node(uint64_t node_id, const void* data, size_t size);

private:
   static const int SIZES_COUNT = 8;
   static const int CONTROL_BLOCK_SIZE = 4096;
   static const int FREE_NODES_BLOCK_NODES_COUNT = CONTROL_BLOCK_SIZE / sizeof(size_t) - 1;
   static const std::array<size_t, SIZES_COUNT> NODE_SIZES;

   struct HeaderBlock
   {
      char signature[4];
      int32_t version;
      size_t free_nodes_block_offsets[SIZES_COUNT];
      size_t available_free_nodes_block_offset;
      uint64_t root_node_id;
      char padding[CONTROL_BLOCK_SIZE - 4 - sizeof(int32_t) - SIZES_COUNT * sizeof(size_t) - sizeof(size_t) - sizeof(uint64_t)];
   };

   static_assert(sizeof(HeaderBlock) == CONTROL_BLOCK_SIZE);

   struct FreeNodesBlock
   {
      size_t free_nodes_offsets[FREE_NODES_BLOCK_NODES_COUNT];
      size_t next_free_nodes_block_offset;
   };

   static_assert(sizeof(FreeNodesBlock) == CONTROL_BLOCK_SIZE);

   using lock_guard = std::lock_guard<std::recursive_mutex>;

   int find_best_fit_size(size_t node_size);

   void save_header_block();

   void allocate_free_nodes_block(int i_size);
   void load_free_nodes_block(int i_size);
   void save_free_nodes_block(int i_size);
   void next_free_nodes_block(int i_size);

   void write_padding(size_t size);

   uint64_t to_node_id(int i_size, size_t offset);
   void from_node_id(uint64_t node_id, int& i_size, size_t& offset);

   std::fstream file;
   size_t file_size;
   std::recursive_mutex lock;
   HeaderBlock header_block;
   std::array<FreeNodesBlock, SIZES_COUNT> free_nodes_blocks;
};

#endif

