#include <chrono>
#include <thread>

#include "storage.h"
#include "node.h"

BOOST_AUTO_TEST_SUITE(time_to_live_tests)

BOOST_AUTO_TEST_CASE(test_time_to_live)
{
   using namespace std::literals::chrono_literals;

   remove("volume");
   auto storage = std::make_unique<Storage>();
   auto volume = storage->open_volume("volume", true);
   storage->mount(volume, "");

   auto node = storage->add_node("", "node");
   node->set_time_to_live(1s);
   
   std::this_thread::sleep_for(2s);
   BOOST_CHECK(node->is_deleted());
}

BOOST_AUTO_TEST_SUITE_END()
