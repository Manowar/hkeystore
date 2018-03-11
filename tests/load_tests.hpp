#include "storage.h"
#include "node.h"
#include "errors.h"
#include <fstream>

using namespace hks;

size_t get_file_size(const char* filename)
{
   std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
   return in.tellg();
}

BOOST_AUTO_TEST_SUITE(load_tests)

BOOST_AUTO_TEST_CASE(test_a_lot_of_keys)
{
   remove("volume");
   auto storage = std::make_unique<Storage>();
   auto volume = storage->open_volume("volume", true);
   storage->mount(volume, "");

   std::vector<char> data;
   data.resize(100);
   for (int i = 0; i < data.size(); i++) {
      data[i] = i;
   }

   std::string path = "";
   for (int i = 0; i < 10; i++) {
      auto node1 = storage->add_node("", "node" + std::to_string(i));
      node1->set_property("float", 1.0f);
      node1->set_property("string", "This is string");
      node1->set_property("blob", data);

      for (int j = 0; j < 10; j++) {
         auto node2 = node1->add_child("node" + std::to_string(i) + "_" + std::to_string(j));
         node2->set_property("float", 1.0f);
         node2->set_property("string", "This is string");
         node2->set_property("blob", data);

         for (int k = 0; k < 10; k++) {
            auto node3 = node2->add_child("node" + std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k));
            node3->set_property("float", 1.0f);
            node3->set_property("string", "This is string");
            node3->set_property("blob", data);
         }
      }
   }
      
   storage->unmount(volume, "");

   BOOST_TEST_MESSAGE("Volume size is " << get_file_size("volume") << " bytes");
}

BOOST_AUTO_TEST_SUITE_END()