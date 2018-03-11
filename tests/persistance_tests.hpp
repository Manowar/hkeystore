#include "storage.h"
#include "node.h"

using namespace hks;


BOOST_AUTO_TEST_SUITE(persistance_tests)

BOOST_AUTO_TEST_CASE(load_volume)
{
   remove("volume");
   {
      auto storage = std::make_unique<Storage>();
      auto volume = storage->open_volume("volume", true);
      storage->mount(volume, "");
      storage->add_node("", "node1");
      storage->set_property("node1.float", 1.0f);
      storage->set_property("node1.string", "abc");
      storage->unmount(volume, "");
   }
   {
      auto storage = std::make_unique<Storage>();
      auto volume = storage->open_volume("volume", false);
      storage->mount(volume, "");

      float f_value;
      BOOST_CHECK(storage->get_property("node1.float", f_value));
      BOOST_CHECK(f_value == 1.0f);

      std::string s_value;
      BOOST_CHECK(storage->get_property("node1.string", s_value));
      BOOST_CHECK(s_value == "abc");

      BOOST_CHECK(!storage->get_property("node2.float", f_value));

      BOOST_CHECK(!storage->get_property("node1.float2", f_value));
   }
}

BOOST_AUTO_TEST_SUITE_END()
