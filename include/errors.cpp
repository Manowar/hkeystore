#include <errors.h>

namespace hks {

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

IOError::IOError(const std::string& error)
   : Exception(error)
{
}

TooLargeNode::TooLargeNode(const std::string& error)
   : Exception(error)
{
}

}
