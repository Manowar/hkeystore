#include "storage.h"
#include "node.h"

BOOST_AUTO_TEST_SUITE(logic_tests)

BOOST_AUTO_TEST_CASE(test_add_node)
{
   remove("volume");
   auto storage = std::make_unique<Storage>();
   auto volume = storage->open_volume("volume", true);
   storage->mount(volume, "");
   BOOST_CHECK(storage->get_node("node1") == nullptr);
   storage->add_node("", "node1");
   BOOST_CHECK(storage->get_node("node1") != nullptr);
}

BOOST_AUTO_TEST_CASE(test_rename_node)
{
   remove("volume");
   auto storage = std::make_unique<Storage>();
   auto volume = storage->open_volume("volume", true);
   storage->mount(volume, "");
   storage->add_node("", "node1");
   BOOST_CHECK(storage->get_node("node1") != nullptr);
   BOOST_CHECK(storage->get_node("node2") == nullptr);
   storage->rename_node("node1", "node2");
   BOOST_CHECK(storage->get_node("node1") == nullptr);
   BOOST_CHECK(storage->get_node("node2") != nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
