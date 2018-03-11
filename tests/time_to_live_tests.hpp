#include <chrono>
#include <thread>

#include "storage.h"
#include "node.h"

using namespace hks;

BOOST_AUTO_TEST_SUITE(time_to_live_tests)

BOOST_AUTO_TEST_CASE(test_time_to_live)
{
   using namespace std::literals::chrono_literals;

   remove("volume");
   auto storage = std::make_unique<Storage>();
   auto volume = storage->open_volume("volume", true);
   storage->mount(volume, "");

   auto node1 = storage->add_node("", "node1");
   auto node2 = storage->add_node("", "node2");
   auto node3 = storage->add_node("node1", "node3");
   node1->set_time_to_live(200ms);
   node2->set_time_to_live(100ms);
   
   std::this_thread::sleep_for(100ms);
   BOOST_CHECK(!node1->is_deleted());
   BOOST_CHECK(node2->is_deleted());
   BOOST_CHECK(!node3->is_deleted());

   std::this_thread::sleep_for(100ms);
   BOOST_CHECK(node1->is_deleted());
   BOOST_CHECK(node2->is_deleted());
   BOOST_CHECK(node3->is_deleted());
}

BOOST_AUTO_TEST_SUITE_END()
