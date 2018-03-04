#include <memory>
#include <cassert>
#include <string>

#include "storage.h"
#include "node.h"

int main()
{
   remove("volume1");
   remove("volume2");

   auto storage = std::make_unique<Storage>();
   auto volume1 = storage->open_volume("volume1", true);
   auto volume2 = storage->open_volume("volume2", true);
   storage->mount(volume1, "volume1");
   storage->mount(volume2, "volume2.volume2");

   auto node1 = storage->get_node("volume1");
   node1->set_property("prop1", 1);
   node1->set_property("prop2", "string");

   unsigned u_value;
   bool res;
   res = node1->get_property("prop1", u_value);
   assert(res);
   assert(u_value == 1);

   std::string s_value;
   res = storage->get_property("volume1.prop2", s_value);
   assert(res);
   assert(s_value == "string");

   auto node2 = storage->get_node("volume2.volume2");
   auto node3 = node2->add_node("root_volume_node");
   node3->set_property("prop3", 1.25);
   float f_value;
   res = storage->get_property("volume2.volume2.root_volume_node.prop3", f_value);
   assert(res);
   assert(f_value == 1.25f);

   storage->mount(volume2, "volume2_node", "root_volume_node");
   res = storage->get_property("volume2_node.prop3", f_value);
   assert(res);
   assert(f_value == 1.25f);

   storage->set_property("volume2_node.prop3", 1LL);
   double d_value;
   res = node3->get_property("prop3", d_value);
   assert(res);
   assert(d_value == 1.0);

   return 0;
}