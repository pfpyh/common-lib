#pragma once

#include "CommonHeader.hpp"
#include "common/NonCopyable.hpp"
#include "common/Factory.hpp"

#include <stdint.h>
#include <string>
#include <memory>

namespace common
{
class COMMON_LIB_API SocketType
{
public :
    enum type : uint8_t
    {
        SERVER,
        CLIENT,
    };
};

class COMMON_LIB_API Socket : public NonCopyable
                            , public Factory<Socket>
{
    friend class Factory<Socket>;
    
public :
    virtual ~Socket() = default;

public :
    static auto __create(SocketType::type socketType) -> std::shared_ptr<Socket>;

public :
    virtual auto prepare(const std::string& address, const int32_t port) -> void = 0;
    virtual auto open() -> void = 0;
    virtual auto close() -> void = 0;

    virtual auto read(void* buffer, const size_t size) -> size_t = 0;
    virtual auto read(char* buffer, const size_t size) -> size_t = 0;
    virtual auto read(void* buffer, const size_t size, const uint32_t millisecond) -> size_t = 0;
    virtual auto read(char* buffer, const size_t size, const uint32_t millisecond) -> size_t = 0;

    virtual auto send(void* buffer, const size_t size) -> void = 0;
    virtual auto send(const char* buffer, const size_t size) -> void = 0;
};
} // namespace common