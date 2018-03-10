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
   storage->add_node("node1", "node2");
   BOOST_CHECK(storage->get_node("node1") != nullptr);
   BOOST_CHECK(storage->get_node("node1.node2") != nullptr);
   BOOST_CHECK(storage->get_node("node1")->get_child("node2") != nullptr);
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

BOOST_AUTO_TEST_CASE(test_delete_node)
{
   remove("volume");
   auto storage = std::make_unique<Storage>();
   auto volume = storage->open_volume("volume", true);
   storage->mount(volume, "");

   auto node1 = storage->add_node("", "node1");
   auto node2 = storage->add_node("node1", "node2");
   auto node3 = storage->add_node("node1.node2", "node3");

   BOOST_CHECK(storage->get_node("node1") != nullptr);
   BOOST_CHECK(storage->get_node("node1.node2") != nullptr);
   BOOST_CHECK(storage->get_node("node1.node2.node3") != nullptr);

   storage->remove_node("node1.node2");

   BOOST_CHECK(storage->get_node("node1") != nullptr);
   BOOST_CHECK(storage->get_node("node1.node2") == nullptr);
   BOOST_CHECK(storage->get_node("node1.node2.node3") == nullptr);

   BOOST_CHECK(!node1->is_deleted());
   BOOST_CHECK(node2->is_deleted());
   BOOST_CHECK(node3->is_deleted());
}

BOOST_AUTO_TEST_SUITE_END()
