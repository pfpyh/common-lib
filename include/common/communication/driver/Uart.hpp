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
#include "common/communication/driver/BaseDriver.hpp"

#include <stdint.h>
#include <string>
#include <memory>

namespace common::communication
{
class SerialHandler;

struct EscapeSequence
{
    enum type : uint8_t
    {
        NULL_END = 0,     /*  \0  */
        LINE_FEED,        /*  \n  */
        CARRIAGE_RETURN,  /* \r\n */
        MAX,
    };

    type _value = NULL_END;
};

struct UartInfo : public DeviceInfo
{
    enum Baudrate : uint8_t
    {
        _9600,
        _19200,
        _38400,
        _57600,
        _115200,
    };

    UartInfo() : DeviceInfo(DeviceInfo::UART) {}

    std::string _port;
    Baudrate _baudrate;
    uint8_t _mode;
};

class COMMON_LIB_API Uart : public BaseDriver
{
private :
    UartInfo _info;
    std::unique_ptr<SerialHandler> _handler;
    bool _isOpen = false;

public :
    explicit Uart(const UartInfo* const info, 
                  std::unique_ptr<SerialHandler> handler) noexcept
        : _info(*info)
        , _handler(std::move(handler)) {}
    explicit Uart(UartInfo* info);
    ~Uart();

public :
    auto open() noexcept -> bool override;
    auto close() noexcept -> void override;
    auto read(unsigned char* buffer, size_t size) noexcept -> bool override;
    auto write(const unsigned char* buffer, size_t size) noexcept -> bool override;
};
} // namespace common::communication