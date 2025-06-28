/**********************************************************************
MIT License

Copyright (c) 2025 Park Younghwan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************************************************/

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