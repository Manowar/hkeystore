#ifndef HKEYSTORE_ERRORS_H
#define HKEYSTORE_ERRORS_H

#include <exception>
#include <string>

class Exception : public std::exception
{
public:
   explicit Exception(const std::string& error);
   const char* what() const override;
private:
   std::string error;
};

class LogicError : public Exception
{
public:
   explicit LogicError(const std::string& error);
};

class NodeAlreadyExists : public Exception
{
public:
   explicit NodeAlreadyExists(const std::string& error);
};

class NoSuchNode : public Exception
{
public:
   explicit NoSuchNode(const std::string& error);
};

class IOError : public Exception
{
public:
   explicit IOError(const std::string& error);
};

class TooLargeNode : public Exception
{
public:
   explicit TooLargeNode(const std::string& error);
};

#endif
