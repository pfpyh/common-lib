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

namespace common::communication
{
struct DeviceInfo
{
    enum DeviceType : uint8_t
    {
        NONE = 0,
        UART,
        I2C,
        SPI,
        CH341_I2C,
        CH341_SPI,
    };

    explicit DeviceInfo(DeviceType type) noexcept : _type(type) {}
    virtual ~DeviceInfo() = default;

    const DeviceType _type;
};

class BaseDriver
{
public :
    virtual ~BaseDriver() = default;

public :
    virtual auto open() noexcept -> bool = 0;
    virtual auto close() noexcept -> void = 0;
    virtual auto read(unsigned char* buffer, size_t size) noexcept -> bool = 0;
    virtual auto write(const unsigned char* buffer, size_t size) noexcept -> bool = 0;
};
} // namespace common::communication