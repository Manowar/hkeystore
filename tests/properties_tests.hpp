#include "storage.h"
#include "node.h"

BOOST_AUTO_TEST_SUITE(properties_tests)

template<typename T> 
void check_property_value(Storage* storage, const std::string& property_path, const T& expected_value)
{
   T value;
   BOOST_CHECK(storage->get_property(property_path, value));
   BOOST_CHECK(value == expected_value);
}

template<typename T>
void check_property_inaccessible(Storage* storage, const std::string& property_path)
{
   T value;
   BOOST_CHECK(!storage->get_property(property_path, value));
}

BOOST_AUTO_TEST_CASE(test_conversions_double)
{
   remove("volume");
   auto storage = std::make_unique<Storage>();
   auto volume = storage->open_volume("volume", true);
   storage->mount(volume, "");

   storage->add_node("", "node");
   storage->set_property("node.property", 3.5);

   check_property_value(storage.get(), "node.property", 3);
   check_property_value(storage.get(), "node.property", 3U);
   check_property_value(storage.get(), "node.property", 3LL);
   check_property_value(storage.get(), "node.property", 3ULL);
   check_property_value(storage.get(), "node.property", 3.5f);
   check_property_value(storage.get(), "node.property", 3.5);
   check_property_value(storage.get(), "node.property", 3.5l);
   check_property_inaccessible<std::string>(storage.get(), "node.property");
   check_property_inaccessible<std::vector<char>>(storage.get(), "node.property");
}

BOOST_AUTO_TEST_CASE(test_conversions_int)
{
   remove("volume");
   auto storage = std::make_unique<Storage>();
   auto volume = storage->open_volume("volume", true);
   storage->mount(volume, "");

   storage->add_node("", "node");
   storage->set_property("node.property", 3);

   check_property_value(storage.get(), "node.property", 3);
   check_property_value(storage.get(), "node.property", 3U);
   check_property_value(storage.get(), "node.property", 3LL);
   check_property_value(storage.get(), "node.property", 3ULL);
   check_property_value(storage.get(), "node.property", 3.0f);
   check_property_value(storage.get(), "node.property", 3.0);
   check_property_value(storage.get(), "node.property", 3.0l);
   check_property_inaccessible<std::string>(storage.get(), "node.property");
   check_property_inaccessible<std::vector<char>>(storage.get(), "node.property");
}

BOOST_AUTO_TEST_CASE(test_conversions_string)
{
   remove("volume");
   auto storage = std::make_unique<Storage>();
   auto volume = storage->open_volume("volume", true);
   storage->mount(volume, "");

   storage->add_node("", "node");
   storage->set_property("node.property", "abc");

   check_property_value(storage.get(), "node.property", std::string("abc"));
   check_property_inaccessible<int>(storage.get(), "node.property");
   check_property_inaccessible<float>(storage.get(), "node.property");
   check_property_inaccessible<std::vector<char>>(storage.get(), "node.property");
}

BOOST_AUTO_TEST_CASE(test_conversions_blob)
{
   remove("volume");
   auto storage = std::make_unique<Storage>();
   auto volume = storage->open_volume("volume", true);
   storage->mount(volume, "");

   std::vector<char> data;
   data.resize(5);
   memcpy(&data[0], "abcde", 5);

   storage->add_node("", "node");
   storage->set_property("node.property", data);

   check_property_value(storage.get(), "node.property", data);
   check_property_inaccessible<int>(storage.get(), "node.property");
   check_property_inaccessible<float>(storage.get(), "node.property");
   check_property_inaccessible<std::string>(storage.get(), "node.property");
}


BOOST_AUTO_TEST_SUITE_END()


