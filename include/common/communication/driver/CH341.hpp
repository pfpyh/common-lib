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
#include "common/Factory.hpp"
#include "common/communication/driver/BaseDriver.hpp"
#include "common/communication/driver/I2C.hpp"
#include "common/communication/driver/SPI.hpp"

#include <stdint.h>
#include <windows.h>
#include <vector>

namespace common::communication
{
struct CH341_I2CInfo : public DeviceInfo
{
    enum Speed : uint32_t
    {
        Low         = 0x00, // 20KHz
        Standard    = 0x01, // 100KHz (default)
        Fast        = 0x02, // 400KHz
        High        = 0x03, // 750KHz
    };

    CH341_I2CInfo() 
        : DeviceInfo(DeviceInfo::CH341_I2C) {}

    Speed _speed = Standard;
    uint8_t _devIndex = 0;
    uint8_t _devAddr = 0x00;
    uint8_t _regAddr = 0x00;
};

struct CH341_SpiInfo : public DeviceInfo
{
    CH341_SpiInfo() 
        : DeviceInfo(DeviceInfo::CH341_SPI) {}
};

class COMMON_LIB_API CH341 : public BaseDriver
                           , public Factory<CH341>
{
    friend class Factory<CH341>;

public :
    virtual ~CH341() = default;

public :
    virtual auto open() noexcept -> bool = 0;
    virtual auto close() noexcept -> void = 0;
    virtual auto read(unsigned char* buffer, size_t size) noexcept -> bool = 0;
    virtual auto write(const unsigned char* buffer, size_t size) noexcept -> bool = 0;

private :
    static auto __create(DeviceInfo* info) noexcept -> std::shared_ptr<CH341>;
};
} // namespace common::communication