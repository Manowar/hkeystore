#include <fstream>
#include <cstring>
#include <string>
#include <cassert>
#include <algorithm>

#include <errors.h>

#include "volume_file.h"


const std::array<size_t, VolumeFile::SIZES_COUNT> VolumeFile::NODE_SIZES = { 32, 64, 128, 256, 512, 1024, 2048, 4096 };

static const int VERSION = 1;
static const char SIGNATURE[4] = { 'H', 'K', 'E', 'Y' };

static const size_t EMPTY_OFFSET = size_t(-1);


bool VolumeFile::volume_file_exists(const std::string& path)
{
   std::ofstream file(path.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::binary);
   return file.is_open();
}

void VolumeFile::create_new_volume_file(const std::string& path)
{
   std::ofstream file(path.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
   if (!file.is_open()) {
      throw IOError("Can't open file '" + path + "' for writing");
   }

   HeaderBlock header_block;
   memcpy(header_block.signature, SIGNATURE, sizeof(header_block.signature));
   header_block.version = VERSION;
   for (int i = 0; i < SIZES_COUNT; i++) {
      header_block.free_nodes_block_offsets[i] = EMPTY_OFFSET;
   }
   header_block.available_free_nodes_block_offset = EMPTY_OFFSET;
   header_block.root_node_id = uint64_t(-1);
   memset(header_block.padding, 0, sizeof(header_block.padding));
   file.write(reinterpret_cast<char*>(&header_block), sizeof(HeaderBlock));

   if (file.bad()) {
      throw IOError("Error writing to '" + path + "'");
   }
}

std::unique_ptr<VolumeFile> VolumeFile::open_volume_file(const std::string& path)
{
   std::unique_ptr<VolumeFile> volume_file(new VolumeFile());

   volume_file->file.open(path.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::binary);
   if (!volume_file->file.is_open()) {
      throw IOError("Can't open file '" + path + "'");
   }

   volume_file->file.seekg(0, std::ios::end);
   volume_file->file_size = volume_file->file.tellg();
   volume_file->file.seekg(0);

   volume_file->file.read(reinterpret_cast<char*>(&volume_file->header_block), CONTROL_BLOCK_SIZE);

   if (volume_file->file.fail()) {
      throw IOError("Can't read volume header");
   }
   if (memcmp(volume_file->header_block.signature, SIGNATURE, sizeof(SIGNATURE)) != 0) {
      throw IOError("File " + path + " is not a volume");
   }
   if (volume_file->header_block.version != VERSION) {
      throw IOError("Unsupported volume version");
   }

   for (int i = 0; i < SIZES_COUNT; i++) {
      volume_file->load_free_nodes_block(i);
   }

   return volume_file;
}

void VolumeFile::read_node(uint64_t node_id, std::function<void(std::istream&)> read)
{
   lock_guard locker(lock);

   int i_size;
   size_t offset;
   from_node_id(node_id, i_size, offset);

   file.seekg(offset);
   read(file);
}

void VolumeFile::write_node(uint64_t node_id, const void* data, size_t size)
{
   lock_guard locker(lock);

   int i_size;
   size_t offset;
   from_node_id(node_id, i_size, offset);

   file.seekp(offset);
   file.write(reinterpret_cast<const char*>(data), size);
}

uint64_t VolumeFile::get_root_node_id()
{
   lock_guard locker(lock);
   return header_block.root_node_id;
}

void VolumeFile::set_root_node_id(uint64_t root_node_id)
{
   lock_guard locker(lock);
   header_block.root_node_id = root_node_id;
   save_header_block();
}

uint64_t VolumeFile::allocate_node(const void* data, size_t size)
{
   lock_guard locker(lock);

   size_t offset = EMPTY_OFFSET;

   int i_size = find_best_fit_size(size);
   if (header_block.free_nodes_block_offsets[i_size] != EMPTY_OFFSET) {
      // Can re-use free block
      for (int i = FREE_NODES_BLOCK_NODES_COUNT - 1; i >= 0; i--) {
         size_t& cur_node_offset = free_nodes_blocks[i_size].free_nodes_offsets[i];
         if (cur_node_offset != EMPTY_OFFSET) {
            offset = cur_node_offset;
            cur_node_offset = EMPTY_OFFSET;
            break;
         }
      }
      assert(offset != EMPTY_OFFSET);

      bool switch_to_next_free_nodes_block = free_nodes_blocks[i_size].free_nodes_offsets[0] == EMPTY_OFFSET;

      if (switch_to_next_free_nodes_block) {
         next_free_nodes_block(i_size);
      } else {
         save_free_nodes_block(i_size);
      }

      file.seekp(offset);
      file.write(reinterpret_cast<const char*>(data), size);
   } else {
      // There is no free block, need to allocate a new one
      offset = file_size;
      file_size += NODE_SIZES[i_size];

      file.seekp(offset);
      file.write(reinterpret_cast<const char*>(data), size);
      write_padding(NODE_SIZES[i_size] - size);
   }

   return to_node_id(i_size, offset);
}

void VolumeFile::remove_node(uint64_t node_id)
{
   lock_guard locker(lock);

   int i_size;
   size_t offset;
   from_node_id(node_id, i_size, offset);

   if (header_block.free_nodes_block_offsets[i_size] == EMPTY_OFFSET) {
      allocate_free_nodes_block(i_size);
   }

   for (int i = 0; i < FREE_NODES_BLOCK_NODES_COUNT; i++) {
      size_t& cur_node_offset = free_nodes_blocks[i_size].free_nodes_offsets[i];
      if (cur_node_offset == EMPTY_OFFSET) {
         cur_node_offset = offset;
         save_free_nodes_block(i_size);
         return;
      }
   }

   allocate_free_nodes_block(i_size);
   free_nodes_blocks[i_size].free_nodes_offsets[0] = offset;
   save_free_nodes_block(i_size);
}

uint64_t VolumeFile::resize_node(uint64_t node_id, const void* data, size_t size)
{
   lock_guard locker(lock);

   int i_current_size;
   size_t offset;
   from_node_id(node_id, i_current_size, offset);

   int i_new_size = find_best_fit_size(size);

   if (i_new_size == i_current_size) {
      // leave node at the same place
      file.seekp(offset);
      file.write(reinterpret_cast<const char*>(data), size);
      return node_id;
   } 

   // move node to a new place
   remove_node(node_id);
   return allocate_node(data, size);
}

int VolumeFile::find_best_fit_size(size_t node_size)
{
   for (int i = 0; i < SIZES_COUNT; i++) {
      if (NODE_SIZES[i] >= node_size) {
         return i;
      }
   }
   throw TooLargeNode("Can't fit node with size " + std::to_string(node_size) + " in volume");
}

void VolumeFile::save_header_block()
{
   file.seekp(0);
   file.write(reinterpret_cast<char*>(&header_block), CONTROL_BLOCK_SIZE);
}

void VolumeFile::allocate_free_nodes_block(int i_size)
{
   if (header_block.available_free_nodes_block_offset != EMPTY_OFFSET) {
      size_t next_free_nodes_block_offset = header_block.free_nodes_block_offsets[i_size];

      header_block.free_nodes_block_offsets[i_size] = header_block.available_free_nodes_block_offset;
      load_free_nodes_block(i_size);
      header_block.available_free_nodes_block_offset = free_nodes_blocks[i_size].next_free_nodes_block_offset;
      free_nodes_blocks[i_size].next_free_nodes_block_offset = next_free_nodes_block_offset;
      save_header_block();
   } else {
      size_t offset = file_size;
      file_size += CONTROL_BLOCK_SIZE;
      for (int i = 0; i < FREE_NODES_BLOCK_NODES_COUNT; i++) {
         free_nodes_blocks[i_size].free_nodes_offsets[i] = EMPTY_OFFSET;
      }
      free_nodes_blocks[i_size].next_free_nodes_block_offset = header_block.free_nodes_block_offsets[i_size];
      header_block.free_nodes_block_offsets[i_size] = offset;
      save_header_block();
   }
}

void VolumeFile::load_free_nodes_block(int i_size)
{
   size_t offset = header_block.free_nodes_block_offsets[i_size];
   if (offset != EMPTY_OFFSET) {
      file.seekg(offset);
      file.read(reinterpret_cast<char*>(&free_nodes_blocks[i_size]), CONTROL_BLOCK_SIZE);
      if (file.fail()) {
         throw IOError("Can't read volume");
      }
   }
}

void VolumeFile::save_free_nodes_block(int i_size)
{
   file.seekp(header_block.free_nodes_block_offsets[i_size]);
   file.write(reinterpret_cast<char*>(&free_nodes_blocks[i_size]), CONTROL_BLOCK_SIZE);
}

void VolumeFile::next_free_nodes_block(int i_size)
{
   size_t next_free_nodes_block_offset = free_nodes_blocks[i_size].next_free_nodes_block_offset;

   free_nodes_blocks[i_size].next_free_nodes_block_offset = header_block.available_free_nodes_block_offset;
   save_free_nodes_block(i_size);

   header_block.available_free_nodes_block_offset = header_block.free_nodes_block_offsets[i_size];
   header_block.free_nodes_block_offsets[i_size] = next_free_nodes_block_offset;
   save_header_block();
}

void VolumeFile::write_padding(size_t size)
{
   const size_t BUF_SIZE = 65536;
   static char buf[BUF_SIZE];

   while (size > 0) {
      size_t to_write = std::min(size, BUF_SIZE);
      file.write(buf, to_write);
      size -= to_write;
   }
}


static const int NODE_ID_I_SIZE_SHIFT = 64 - 8;

uint64_t VolumeFile::to_node_id(int i_size, size_t offset)
{
   uint64_t res = (uint64_t(i_size) << NODE_ID_I_SIZE_SHIFT) + offset;
   return res;
}

void VolumeFile::from_node_id(uint64_t node_id, int& i_size, size_t& offset)
{
   i_size = static_cast<int>(node_id >> NODE_ID_I_SIZE_SHIFT);
   offset = static_cast<size_t>(node_id & ((uint64_t(1) << NODE_ID_I_SIZE_SHIFT) - 1));
}

