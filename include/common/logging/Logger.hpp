#pragma once

#include "CommonHeader.hpp"

#include <memory>
#include <string>
#include <cstring>
#include <iostream>
#include <stdexcept>

#define VA_ARGS(...) , ##__VA_ARGS__
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define _INFO_(format, ...) ::common::__logger.info(format + std::string(" (%s:%d)"), ##__VA_ARGS__, __FILENAME__, __LINE__);
#define _DEBUG_(format, ...) ::common::__logger.debug(format + std::string(" (%s:%d)"), ##__VA_ARGS__, __FILENAME__, __LINE__);
#define _ERROR_(format, ...) ::common::__logger.error(format + std::string(" (%s:%d)"), ##__VA_ARGS__, __FILENAME__, __LINE__);

namespace common
{
class COMMON_LIB_API Logger
{
private :
    template <typename ... Args>
    auto string_format(const std::string& format, Args ... args) -> std::string
    {
        const int32_t size = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1;\
        std::unique_ptr<char[]> buf( new char[ size ] );
        std::snprintf( buf.get(), size, format.c_str(), args ... );\
        return std::string( buf.get(), buf.get() + size - 1 );
    }

public :
    template <typename ... Args>
    auto info(const std::string& format, Args ... args) -> void
    {
        info(string_format(format, args ...));
    }
    auto info(const std::string& format) -> void;

    template <typename ... Args>
    auto debug(const std::string& format, Args ... args) -> void
    {
        debug(string_format(format, args ...));
    }
    auto debug(const std::string& format) -> void;

    template <typename ... Args>
    auto error(const std::string& format, Args ... args) -> void
    {
        error(string_format(format, args ...));
    }
    auto error(const std::string& format) -> void;
};

static Logger __logger;
} // namespace common
