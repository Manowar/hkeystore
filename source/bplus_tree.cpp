#define _SCL_SECURE_NO_WARNINGS

#include <cassert>
#include <list>
#include <algorithm>
#include <chrono>
#include <vector>
#include <sstream>

#include "volume_file.h"
#include "bplus_tree.h"
#include "utility.h"
#include "node_to_remove_key.h"
#include "serialization.h"

namespace hks {

using std::swap;
using std::binary_search;
using std::lower_bound;
using std::upper_bound;

/* helper iterating function */
template<class T>
inline typename T::child_t begin(T &node) {
   return node.children;
}
template<class T>
inline typename T::child_t end(T &node) {
   return node.children + node.n;
}

template<class key_t, class value_t, class record_t>
struct record_t_compare
{
   bool operator () (const record_t& record, const key_t& key)
   {
      return record.key < key;
   }

   bool operator () (const key_t& key, const record_t& record)
   {
      return key < record.key;
   }
};

template<class key_t, class value_t>
BplusTree<key_t, value_t>::BplusTree(std::shared_ptr<VolumeFile> volume_file, record_id_t meta_record_id)
   : volume_file(volume_file)
   , meta_record_id(meta_record_id)
{
   map(&meta, meta_record_id);
}

template<class key_t, class value_t>
BplusTree<key_t, value_t>::BplusTree(std::shared_ptr<VolumeFile> volume_file)
   : volume_file(volume_file)
{
   // init default meta
   memset(&meta, 0, sizeof(meta_t));
   meta.order = BP_ORDER;
   meta.height = 1;

   // init root node
   internal_node_t root;
   root.next = root.prev = root.parent = 0;
   meta.root_offset = alloc(&root);

   // init empty leaf
   leaf_node_t leaf;
   leaf.next = leaf.prev = 0;
   leaf.parent = meta.root_offset;
   root.children[0].child = alloc(&leaf);

   // save
   meta_record_id = EMPTY_RECORD_ID;
   unmap(&leaf, root.children[0].child);
   unmap(&root, meta.root_offset);
   unmap(&meta, meta_record_id);
}

template<class key_t>
inline int keycmp(const key_t &a, const key_t &b) {
   if (a < b) {
      return -1;
   }
   if (b < a) {
      return 1;
   }
   return 0;
}

template<class key_t, class value_t>
bool BplusTree<key_t, value_t>::get_first(key_t* key, value_t* value) const
{
   leaf_node_t leaf;
   map(&leaf, search_leaf(key_t()));
   if (leaf.n == 0) {
      return false;
   }

   *key = leaf.children[0].key;
   *value = leaf.children[0].value;
   return true;
}

template<class key_t, class value_t>
int BplusTree<key_t, value_t>::search(const key_t& key, value_t* value) const
{
   leaf_node_t leaf;
   map(&leaf, search_leaf(key));

   // finding the record
   record_t* record = find(leaf, key);
   if (record != leaf.children + leaf.n) {
      // always return the lower bound
      *value = record->value;

      return keycmp(record->key, key);
   }
   else {
      return -1;
   }
}

template<class key_t, class value_t>
int BplusTree<key_t, value_t>::remove(const key_t& key)
{
   internal_node_t parent;
   leaf_node_t leaf;

   // find parent node
   record_id_t parent_off = search_index(key);
   map(&parent, parent_off);

   // find current node
   index_t *where = find(parent, key);
   record_id_t offset = where->child;
   map(&leaf, offset);

   // verify
   if (!binary_search(begin(leaf), end(leaf), key, record_t_compare<key_t, value_t, record_t>()))
      return -1;

   size_t min_n = meta.leaf_node_num == 1 ? 0 : meta.order / 2;
   assert(leaf.n >= min_n && leaf.n <= meta.order);

   // delete the key
   record_t *to_delete = find(leaf, key);
   std::copy(to_delete + 1, end(leaf), to_delete);
   leaf.n--;

   // merge or borrow
   if (leaf.n < min_n) {
      // first borrow from left
      bool borrowed = false;
      if (leaf.prev != 0)
         borrowed = borrow_key(false, leaf);

      // then borrow from right
      if (!borrowed && leaf.next != 0)
         borrowed = borrow_key(true, leaf);

      // finally we merge
      if (!borrowed) {
         assert(leaf.next != 0 || leaf.prev != 0);

         key_t index_key;

         if (where == end(parent) - 1) {
            // if leaf is last element then merge | prev | leaf |
            assert(leaf.prev != 0);
            leaf_node_t prev;
            map(&prev, leaf.prev);
            index_key = begin(prev)->key;

            merge_leafs(&prev, &leaf);
            node_remove(&prev, &leaf);
            unmap(&prev, leaf.prev);
         }
         else {
            // else merge | leaf | next |
            assert(leaf.next != 0);
            leaf_node_t next;
            map(&next, leaf.next);
            index_key = begin(leaf)->key;

            merge_leafs(&leaf, &next);
            node_remove(&leaf, &next);
            unmap(&leaf, offset);
         }

         // remove parent's key
         remove_from_index(parent_off, parent, index_key);
      }
      else {
         unmap(&leaf, offset);
      }
   }
   else {
      unmap(&leaf, offset);
   }

   return 0;
}

template<class key_t, class value_t>
int BplusTree<key_t, value_t>::insert(const key_t& key, const value_t& value)
{
   record_id_t parent = search_index(key);
   record_id_t offset = search_leaf(parent, key);
   leaf_node_t leaf;
   map(&leaf, offset);

   // check if we have the same key
   if (binary_search(begin(leaf), end(leaf), key, record_t_compare<key_t, value_t, record_t>()))
      return 1;

   if (leaf.n == meta.order) {
      // split when full

      // new sibling leaf
      leaf_node_t new_leaf;
      node_create(offset, &leaf, &new_leaf);

      // find even split point
      size_t point = leaf.n / 2;
      bool place_right = keycmp(key, leaf.children[point].key) > 0;
      if (place_right)
         ++point;

      // split
      std::copy(leaf.children + point, leaf.children + leaf.n,
         new_leaf.children);
      new_leaf.n = leaf.n - point;
      leaf.n = point;

      // which part do we put the key
      if (place_right)
         insert_record_no_split(&new_leaf, key, value);
      else
         insert_record_no_split(&leaf, key, value);

      // save leafs
      unmap(&leaf, offset);
      unmap(&new_leaf, leaf.next);

      // insert new index key
      insert_key_to_index(parent, new_leaf.children[0].key,
         offset, leaf.next);
   }
   else {
      insert_record_no_split(&leaf, key, value);
      unmap(&leaf, offset);
   }

   return 0;
}

template<typename key_t, typename value_t>
record_id_t BplusTree<key_t, value_t>::get_record_id() const
{
   return meta_record_id;
}

template<class key_t, class value_t>
void BplusTree<key_t, value_t>::remove_from_index(record_id_t offset, internal_node_t& node, const key_t& key)
{
   size_t min_n = meta.root_offset == offset ? 1 : meta.order / 2;
   assert(node.n >= min_n && node.n <= meta.order);

   // remove key
   key_t index_key = begin(node)->key;
   index_t *to_delete = find(node, key);
   if (to_delete != end(node)) {
      (to_delete + 1)->child = to_delete->child;
      std::copy(to_delete + 1, end(node), to_delete);
   }
   node.n--;

   // remove to only one key
   if (node.n == 1 && meta.root_offset == offset &&
      meta.internal_node_num != 1)
   {
      unalloc(&node, meta.root_offset);
      meta.height--;
      meta.root_offset = node.children[0].child;
      unmap(&meta, meta_record_id);
      return;
   }

   // merge or borrow
   if (node.n < min_n) {
      internal_node_t parent;
      map(&parent, node.parent);

      // first borrow from left
      bool borrowed = false;
      if (offset != begin(parent)->child)
         borrowed = borrow_key(false, node, offset);

      // then borrow from right
      if (!borrowed && offset != (end(parent) - 1)->child)
         borrowed = borrow_key(true, node, offset);

      // finally we merge
      if (!borrowed) {
         assert(node.next != 0 || node.prev != 0);

         if (offset == (end(parent) - 1)->child) {
            // if leaf is last element then merge | prev | leaf |
            assert(node.prev != 0);
            internal_node_t prev;
            map(&prev, node.prev);

            // merge
            index_t *where = find(parent, begin(prev)->key);
            reset_index_children_parent(begin(node), end(node), node.prev);
            merge_keys(where, prev, node);
            unmap(&prev, node.prev);
         }
         else {
            // else merge | leaf | next |
            assert(node.next != 0);
            internal_node_t next;
            map(&next, node.next);

            // merge
            index_t *where = find(parent, index_key);
            reset_index_children_parent(begin(next), end(next), offset);
            merge_keys(where, node, next);
            unmap(&node, offset);
         }

         // remove parent's key
         remove_from_index(node.parent, parent, index_key);
      }
      else {
         unmap(&node, offset);
      }
   }
   else {
      unmap(&node, offset);
   }
}

template<class key_t, class value_t>
bool BplusTree<key_t, value_t>::borrow_key(bool from_right, internal_node_t &borrower, record_id_t offset)
{
   typedef typename internal_node_t::child_t child_t;

   record_id_t lender_off = from_right ? borrower.next : borrower.prev;
   internal_node_t lender;
   map(&lender, lender_off);

   assert(lender.n >= meta.order / 2);
   if (lender.n != meta.order / 2) {
      child_t where_to_lend, where_to_put;

      internal_node_t parent;

      // swap keys, draw on paper to see why
      if (from_right) {
         where_to_lend = begin(lender);
         where_to_put = end(borrower);

         map(&parent, borrower.parent);
         child_t where = lower_bound(begin(parent), end(parent) - 1, (end(borrower) - 1)->key, record_t_compare<key_t, value_t, index_t>());
         where->key = where_to_lend->key;
         unmap(&parent, borrower.parent);
      }
      else {
         where_to_lend = end(lender) - 1;
         where_to_put = begin(borrower);

         map(&parent, lender.parent);
         child_t where = find(parent, begin(lender)->key);
         where_to_put->key = where->key;
         where->key = (where_to_lend - 1)->key;
         unmap(&parent, lender.parent);
      }

      // store
      std::copy_backward(where_to_put, end(borrower), end(borrower) + 1);
      *where_to_put = *where_to_lend;
      borrower.n++;

      // erase
      reset_index_children_parent(where_to_lend, where_to_lend + 1, offset);
      std::copy(where_to_lend + 1, end(lender), where_to_lend);
      lender.n--;
      unmap(&lender, lender_off);
      return true;
   }

   return false;
}

template<class key_t, class value_t>
bool BplusTree<key_t, value_t>::borrow_key(bool from_right, leaf_node_t &borrower)
{
   record_id_t lender_off = from_right ? borrower.next : borrower.prev;
   leaf_node_t lender;
   map(&lender, lender_off);

   assert(lender.n >= meta.order / 2);
   if (lender.n != meta.order / 2) {
      typename leaf_node_t::child_t where_to_lend, where_to_put;

      // decide offset and update parent's index key
      if (from_right) {
         where_to_lend = begin(lender);
         where_to_put = end(borrower);
         change_parent_child(borrower.parent, begin(borrower)->key,
            lender.children[1].key);
      }
      else {
         where_to_lend = end(lender) - 1;
         where_to_put = begin(borrower);
         change_parent_child(lender.parent, begin(lender)->key,
            where_to_lend->key);
      }

      // store
      std::copy_backward(where_to_put, end(borrower), end(borrower) + 1);
      *where_to_put = *where_to_lend;
      borrower.n++;

      // erase
      std::copy(where_to_lend + 1, end(lender), where_to_lend);
      lender.n--;
      unmap(&lender, lender_off);
      return true;
   }

   return false;
}

template<class key_t, class value_t>
void BplusTree<key_t, value_t>::change_parent_child(record_id_t parent, const key_t& o, const key_t& n)
{
   internal_node_t node;
   map(&node, parent);

   index_t *w = find(node, o);
   assert(w != node.children + node.n);

   w->key = n;
   unmap(&node, parent);
   if (w == node.children + node.n - 1) {
      change_parent_child(node.parent, o, n);
   }
}

template<class key_t, class value_t>
void BplusTree<key_t, value_t>::merge_leafs(leaf_node_t* left, leaf_node_t* right)
{
   std::copy(begin(*right), end(*right), end(*left));
   left->n += right->n;
}

template<class key_t, class value_t>
void BplusTree<key_t, value_t>::merge_keys(index_t* where, internal_node_t& node, internal_node_t& next)
{
   std::copy(begin(next), end(next), end(node));
   node.n += next.n;
   node_remove(&node, &next);
}

template<class key_t, class value_t>
void BplusTree<key_t, value_t>::insert_record_no_split(leaf_node_t* leaf, const key_t& key, const value_t& value)
{
   record_t* where = upper_bound(begin(*leaf), end(*leaf), key, record_t_compare<key_t, value_t, record_t>());
   std::copy_backward(where, end(*leaf), end(*leaf) + 1);

   where->key = key;
   where->value = value;
   leaf->n++;
}

template<class key_t, class value_t>
void BplusTree<key_t, value_t>::insert_key_to_index(record_id_t offset, const key_t& key, record_id_t old, record_id_t after)
{
   if (offset == 0) {
      // create new root node
      internal_node_t root;
      root.next = root.prev = root.parent = 0;
      meta.root_offset = alloc(&root);
      meta.height++;

      // insert `old` and `after`
      root.n = 2;
      root.children[0].key = key;
      root.children[0].child = old;
      root.children[1].child = after;

      unmap(&meta, meta_record_id);
      unmap(&root, meta.root_offset);

      // update children's parent
      reset_index_children_parent(begin(root), end(root), meta.root_offset);
      return;
   }

   internal_node_t node;
   map(&node, offset);
   assert(node.n <= meta.order);

   if (node.n == meta.order) {
      // split when full

      internal_node_t new_node;
      node_create(offset, &node, &new_node);

      // find even split point
      size_t point = (node.n - 1) / 2;
      bool place_right = keycmp(key, node.children[point].key) > 0;
      if (place_right)
         ++point;

      // prevent the `key` being the right `middle_key`
      // example: insert 48 into |42|45| 6|  |
      if (place_right && keycmp(key, node.children[point].key) < 0)
         point--;

      key_t middle_key = node.children[point].key;

      // split
      std::copy(begin(node) + point + 1, end(node), begin(new_node));
      new_node.n = node.n - point - 1;
      node.n = point + 1;

      // put the new key
      if (place_right)
         insert_key_to_index_no_split(new_node, key, after);
      else
         insert_key_to_index_no_split(node, key, after);

      unmap(&node, offset);
      unmap(&new_node, node.next);

      // update children's parent
      reset_index_children_parent(begin(new_node), end(new_node), node.next);

      // give the middle key to the parent
      // note: middle key's child is reserved
      insert_key_to_index(node.parent, middle_key, offset, node.next);
   }
   else {
      insert_key_to_index_no_split(node, key, after);
      unmap(&node, offset);
   }
}

template<class key_t, class value_t>
void BplusTree<key_t, value_t>::insert_key_to_index_no_split(internal_node_t &node, const key_t &key, record_id_t value)
{
   index_t *where = upper_bound(begin(node), end(node) - 1, key, record_t_compare<key_t, value_t, index_t>());

   // move later index forward
   std::copy_backward(where, end(node), end(node) + 1);

   // insert this key
   where->key = key;
   where->child = (where + 1)->child;
   (where + 1)->child = value;

   node.n++;
}

template<class key_t, class value_t>
void BplusTree<key_t, value_t>::reset_index_children_parent(index_t* begin, index_t* end, record_id_t parent)
{
   // this function can change both internal_node_t and leaf_node_t's parent
   // field, but we should ensure that:
   // 1. sizeof(internal_node_t) <= sizeof(leaf_node_t)
   // 2. parent field is placed in the beginning and have same size
   internal_node_t node;
   while (begin != end) {
      map(&node, begin->child);
      node.parent = parent;
      unmap(&node, begin->child);
      ++begin;
   }
}

template<class key_t, class value_t>
record_id_t BplusTree<key_t, value_t>::search_index(const key_t &key) const
{
   record_id_t org = meta.root_offset;
   size_t height = meta.height;
   while (height > 1) {
      internal_node_t node;
      map(&node, org);

      index_t *i = upper_bound(begin(node), end(node) - 1, key, record_t_compare<key_t, value_t, index_t>());
      org = i->child;
      --height;
   }

   return org;
}

template<class key_t, class value_t>
record_id_t BplusTree<key_t, value_t>::search_leaf(record_id_t index, const key_t &key) const
{
   internal_node_t node;
   map(&node, index);

   index_t* i = upper_bound(begin(node), end(node) - 1, key, record_t_compare<key_t, value_t, index_t>());
   return i->child;
}

template<class key_t, class value_t>
template<class T>
void BplusTree<key_t, value_t>::node_create(record_id_t offset, T* node, T* next)
{
   // new sibling node
   next->parent = node->parent;
   next->next = node->next;
   next->prev = offset;
   node->next = alloc(next);
   // update next node's prev
   if (next->next != 0) {
      T old_next;
      map(&old_next, next->next);
      old_next.prev = node->next;
      unmap(&old_next, next->next);
   }
   unmap(&meta, meta_record_id);
}

template<class key_t, class value_t>
template<class T>
void BplusTree<key_t, value_t>::node_remove(T *prev, T *node)
{
   unalloc(node, prev->next);
   prev->next = node->next;
   if (node->next != 0) {
      T next;
      map(&next, node->next);
      next.prev = node->prev;
      unmap(&next, node->next);
   }
   unmap(&meta, meta_record_id);
}

template<class key_t, class value_t>
template<class T>
void BplusTree<key_t, value_t>::map(T* block, record_id_t record_id) const
{
   volume_file->read_record(record_id, [&](std::istream& is) {
      deserialize(is, *block);
   });
}

template<class key_t, class value_t>
template<class T>
void BplusTree<key_t, value_t>::unmap(T* block, record_id_t& record_id)
{
   std::ostringstream os;
   serialize(os, *block);
   std::string data = os.str();

   if (record_id == EMPTY_RECORD_ID) {
      record_id = volume_file->allocate_record(data.c_str(), data.length());
   } else {
      record_id = volume_file->resize_record(record_id, data.c_str(), data.length());
   }
}

template<class key_t, class value_t>
typename BplusTree<key_t, value_t>::index_t* BplusTree<key_t, value_t>::find(internal_node_t& node, const key_t& key) {
   return upper_bound(begin(node), end(node) - 1, key, record_t_compare<key_t, value_t, index_t>());
}

template<class key_t, class value_t>
typename BplusTree<key_t, value_t>::record_t* BplusTree<key_t, value_t>::find(leaf_node_t& node, const key_t& key) {
   return lower_bound(begin(node), end(node), key, record_t_compare<key_t, value_t, record_t>());
}

template<typename key_t, typename value_t>
inline void BplusTree<key_t, value_t>::index_t::serialize(std::ostream& os) const
{
   hks::serialize(os, key);
   hks::serialize(os, child);
}

template<typename key_t, typename value_t>
inline void BplusTree<key_t, value_t>::index_t::deserialize(std::istream& is)
{
   hks::deserialize(is, key);
   hks::deserialize(is, child);
}

template<typename key_t, typename value_t>
inline void BplusTree<key_t, value_t>::record_t::serialize(std::ostream& os) const
{
   hks::serialize(os, key);
   hks::serialize(os, value);
}

template<typename key_t, typename value_t>
inline void BplusTree<key_t, value_t>::record_t::deserialize(std::istream& is)
{
   hks::deserialize(is, key);
   hks::deserialize(is, value);
}

template<typename key_t, typename value_t>
void BplusTree<key_t, value_t>::meta_t::deserialize(std::istream& is)
{
   hks::deserialize(is, order);
   hks::deserialize(is, internal_node_num);
   hks::deserialize(is, leaf_node_num);
   hks::deserialize(is, height);
   hks::deserialize(is, root_offset);
}

template<typename key_t, typename value_t>
void BplusTree<key_t, value_t>::meta_t::serialize(std::ostream& os) const
{
   hks::serialize(os, order);
   hks::serialize(os, internal_node_num);
   hks::serialize(os, leaf_node_num);
   hks::serialize(os, height);
   hks::serialize(os, root_offset);
}

template<typename key_t, typename value_t>
void BplusTree<key_t, value_t>::internal_node_t::deserialize(std::istream& is)
{
   hks::deserialize(is, parent);
   hks::deserialize(is, next);
   hks::deserialize(is, prev);
   hks::deserialize(is, n);
   hks::deserialize(is, children);
}

template<typename key_t, typename value_t>
void BplusTree<key_t, value_t>::internal_node_t::serialize(std::ostream& os) const
{
   hks::serialize(os, parent);
   hks::serialize(os, next);
   hks::serialize(os, prev);
   hks::serialize(os, n);
   hks::serialize(os, children);
}

template<typename key_t, typename value_t>
void BplusTree<key_t, value_t>::leaf_node_t::deserialize(std::istream& is)
{
   hks::deserialize(is, parent);
   hks::deserialize(is, next);
   hks::deserialize(is, prev);
   hks::deserialize(is, n);
   hks::deserialize(is, children);
}

template<typename key_t, typename value_t>
void BplusTree<key_t, value_t>::leaf_node_t::serialize(std::ostream& os) const
{
   hks::serialize(os, parent);
   hks::serialize(os, next);
   hks::serialize(os, prev);
   hks::serialize(os, n);
   hks::serialize(os, children);
}


template BplusTree<node_to_remove_key_t, std::vector<node_id_t>>;

}
