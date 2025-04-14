#pragma once

#include "CommonHeader.hpp"

#include <exception>
#include <string>

namespace common
{
class COMMON_LIB_API BaseException : public std::exception
{
protected :
    std::string _detail;

public :
    BaseException() = default;
    BaseException(const std::string& detail)
        : _detail(detail) { }
};

class COMMON_LIB_API AlreadyRunningException : public BaseException
{
public :
    AlreadyRunningException() = default;
    AlreadyRunningException(const std::string& detail)
        : BaseException(detail) { }

public :
    const char* what() const noexcept override
    {
        const std::string what = _detail.empty() ? 
            "already running" : "already running:" + _detail;
        return what.c_str();
    }
};

class COMMON_LIB_API OutOfRangeException : public BaseException
{
public :
    OutOfRangeException() = default;
    OutOfRangeException(const std::string& detail)
        : BaseException(detail) { }

public :
    const char* what() const noexcept override
    {
        const std::string what = _detail.empty() ? 
            "out of range" : "out of range:" + _detail;
        return what.c_str();
    }
};

class COMMON_LIB_API CommunicationException : public BaseException
{
public :
    CommunicationException() = default;
    CommunicationException(const std::string& detail)
        : BaseException(detail) { }

public :
    const char* what() const noexcept override
    {
        const std::string what = _detail.empty() ? 
            "communication error" : "communication error:" + _detail;
        return what.c_str();
    }
};
} // namespace common