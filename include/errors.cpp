#include <errors.h>

Exception::Exception(const std::string& error)
   : error(error)
{
}

const char* Exception::what() const
{
   return error.c_str();
}

LogicError::LogicError(const std::string& error)
   : Exception(error)
{
}

NodeAlreadyExists::NodeAlreadyExists(const std::string& error)
   : Exception(error)
{
}

NoSuchNode::NoSuchNode(const std::string& error)
   : Exception(error)
{
}
