#ifndef HKEYSTORE_BPLUS_TREE_H
#define HKEYSTORE_BPLUS_TREE_H

// Code is taken from https://github.com/zcbenz/BPlusTree and adapted to work with VolumeFile instead of separate file
// Also added support for variable-size values

#include <memory>
#include "volume_file.h"

template<typename key_t, typename value_t>
class BplusTree {
   /* internal nodes' index segment */
   struct index_t {
      key_t key;
      record_id_t child;

      template<class Archive>
      void serialize(Archive & ar, unsigned version)
      {
         ar & key;
         ar & child;
      }
   };

   /* the final record of value */
   struct record_t {
      key_t key;
      value_t value;

      template<class Archive>
      void serialize(Archive & ar, unsigned version)
      {
         ar & key;
         ar & value;
      }
   };

   static const int BP_ORDER = 100;

   /* meta information of B+ tree */
   typedef struct {
      size_t order; /* `order` of B+ tree */
      size_t internal_node_num; /* how many internal nodes */
      size_t leaf_node_num;     /* how many leafs */
      size_t height;            /* height of tree (exclude leafs) */
      record_id_t root_offset; /* where is the root of internal nodes */

      template<class Archive> 
      void serialize(Archive & ar, unsigned version)
      {
         ar & order;
         ar & internal_node_num;
         ar & leaf_node_num;
         ar & height;
         ar & root_offset;
      }
   } meta_t;

   /***
   * internal node block
   ***/
   struct internal_node_t {
      using child_t = index_t*;

      record_id_t parent;
      record_id_t next;
      record_id_t prev;
      size_t n; /* how many children */
      index_t children[BP_ORDER];

      template<class Archive>
      void serialize(Archive & ar, unsigned version)
      {
         ar & parent;
         ar & next;
         ar & prev;
         ar & n;
         ar & children;
      }
   };

   /* leaf node block */
   struct leaf_node_t {
      using child_t = record_t*;

      record_id_t parent; /* parent node offset */
      record_id_t next;
      record_id_t prev;
      size_t n;
      record_t children[BP_ORDER];

      template<class Archive>
      void serialize(Archive & ar, unsigned version)
      {
         ar & parent;
         ar & next;
         ar & prev;
         ar & n;
         ar & children;
      }
   };

   static const record_id_t EMPTY_RECORD_ID = record_id_t(-1);

public:
   BplusTree(std::shared_ptr<VolumeFile> volume_file, record_id_t meta_record_id);

   /* init empty tree */
   BplusTree(std::shared_ptr<VolumeFile> volume_file);

   /* abstract operations */
   int search(const key_t& key, value_t* value) const;
   int remove(const key_t& key);
   int insert(const key_t& key, const value_t& value);

   record_id_t get_record_id() const;

private:
   meta_t meta;
   record_id_t meta_record_id;
   mutable std::shared_ptr<VolumeFile> volume_file;

   /* find index */
   record_id_t search_index(const key_t& key) const;

   /* find leaf */
   record_id_t search_leaf(record_id_t index, const key_t& key) const;
   record_id_t search_leaf(const key_t& key) const
   {
      return search_leaf(search_index(key), key);
   }

   /* remove internal node */
   void remove_from_index(record_id_t offset, internal_node_t& node, const key_t& key);

   /* borrow one key from other internal node */
   bool borrow_key(bool from_right, internal_node_t& borrower, record_id_t offset);

   /* borrow one record from other leaf */
   bool borrow_key(bool from_right, leaf_node_t& borrower);

   /* change one's parent key to another key */
   void change_parent_child(record_id_t parent, const key_t& o, const key_t& n);

   /* merge right leaf to left leaf */
   void merge_leafs(leaf_node_t* left, leaf_node_t* right);

   void merge_keys(index_t* where, internal_node_t& left, internal_node_t& right);

   /* insert into leaf without split */
   void insert_record_no_split(leaf_node_t *leaf, const key_t& key, const value_t& value);

   /* add key to the internal node */
   void insert_key_to_index(record_id_t offset, const key_t& key, record_id_t value, record_id_t after);
   void insert_key_to_index_no_split(internal_node_t& node, const key_t& key, record_id_t value);

   /* change children's parent */
   void reset_index_children_parent(index_t* begin, index_t* end, record_id_t parent);

   template<class T>
   void node_create(record_id_t offset, T* node, T* next);

   template<class T>
   void node_remove(T* prev, T* node);

   record_id_t alloc(leaf_node_t* leaf)
   {
      leaf->n = 0;
      meta.leaf_node_num++;
      return EMPTY_RECORD_ID;
   }

   record_id_t alloc(internal_node_t* node)
   {
      node->n = 1;
      meta.internal_node_num++;
      return EMPTY_RECORD_ID;
   }

   void unalloc(leaf_node_t* leaf, record_id_t record_id)
   {
      --meta.leaf_node_num;
      volume_file->delete_record(record_id);
   }

   void unalloc(internal_node_t* node, record_id_t record_id)
   {
      --meta.internal_node_num;
      volume_file->delete_record(record_id);
   }

   template<class T>
   void map(T* block, record_id_t record_id) const;

   template<class T>
   void unmap(T* block, record_id_t& record_id);

   static index_t* find(internal_node_t& node, const key_t& key);
   static record_t* find(leaf_node_t& node, const key_t& key);
};



#endif 