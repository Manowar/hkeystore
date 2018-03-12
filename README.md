# hkeystore
Hierarchical key/value storage written in C++ 17.

- Nodes are organized in a hierarchy
- Supports adding/removing/deleting/renaming nodes
- Supports automatic nodes removal, if time to live is set for a node
- For properties, primitive types, std::string and blob value types are supported
- Physical volumes may be mounted to different places in a storage
- Full multi-threading support
- Volumes are backed up by disk files. Volume sizes could be large, and only needed volume parts are loaded in memory 

Currently build projects are provided only for MS VC 2017. However code should work under any C++ 17 compliant compiler.
