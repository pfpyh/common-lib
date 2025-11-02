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

#include "common/communication/driver/CH341.hpp"
#include "common/logging/Logger.hpp"

#include "libs/CH341/CH341DLL.H"

namespace common::communication
{
namespace detail
{
class I2C final : public CH341
{
private :
    bool _isOpen = false;
    HANDLE _handle;

    const CH341_I2CInfo _info;

public :
    I2C(CH341_I2CInfo* info)
        : _info(*info) 
    {
        if(_info._devAddr >= 0x80)
        {
            _DEBUG_("CH341::Invalid I2C address");
            if constexpr (STRICT_MODE_ENABLED)
            {
                assert(false && "Invalid I2C address");
            }
        }
    }

public :
    auto open() noexcept -> bool override
    {
        _handle = CH341OpenDevice(_info._devIndex);
        if (_handle == INVALID_HANDLE_VALUE) 
        { 
            _DEBUG_("CH341::I2C CH341OpenDevice failure");
            return false; 
        }

        auto speedMode = (static_cast<uint32_t>(_info._speed) | 0x03);
        if(!CH341SetStream(_info._devIndex, speedMode))
        {
            close();
            _DEBUG_("CH341::I2C CH341SetStream failure");
            return false;
        }

        return true;
    }

    auto close() noexcept -> void override
    {
        if (_handle == INVALID_HANDLE_VALUE) { return; }

        CH341CloseDevice(_info._devIndex);
        _handle = INVALID_HANDLE_VALUE;
    }

    auto read(unsigned char* buffer, size_t size) noexcept -> bool override
    {
        if constexpr (STRICT_MODE_ENABLED)
        {
            if(!_isOpen)
            {
                _ERROR_("CH341::Not opened");
                assert(false && "Not opened");
            }
        }

        if(!check_dev()) 
        { 
            _DEBUG_("CH341::I2C check device error");
            return false; 
        }

        uint8_t command[2] = {0, };
        command[0] = (_info._devAddr << 1);
        command[1] = _info._regAddr;

        if(!CH341StreamI2C(_info._devIndex, sizeof(command), command, size, buffer))
        {
            _DEBUG_("CH341::I2C check CH341StreamI2C failure");
            return false;
        }
        return true;
    }

    auto write(const unsigned char* buffer, size_t size) noexcept -> bool override
    {
        return false;
    }

private :
    auto check_dev() -> bool
    {
        uint8_t shifted_addr = (_info._devAddr << 1);
        bool rtn = true;

        if(!issue_start())
        {
            _DEBUG_("CH341::I2C start error");
            return false;
        }

        if(!check_ack(shifted_addr)) 
        { 
            _DEBUG_("CH341::I2C check ack error");
            rtn = false;
        }

        issue_stop();
        return rtn;
    }

    auto issue_start() noexcept -> bool
    {
        ULONG len = 3;
        uint8_t command[mCH341_PACKET_LENGTH] = {0, };
        command[0] = mCH341A_CMD_I2C_STREAM;
        command[1] = mCH341A_CMD_I2C_STM_STA;
        command[2] = mCH341A_CMD_I2C_STM_END;

        return CH341WriteData(_info._devIndex, command, &len);
    }

    auto issue_stop() noexcept -> bool
    {
        ULONG len = 3;
        uint8_t command[mCH341_PACKET_LENGTH] = {0, };
        command[0] = mCH341A_CMD_I2C_STREAM;
        command[1] = mCH341A_CMD_I2C_STM_STO;
        command[2] = mCH341A_CMD_I2C_STM_END;

        return CH341WriteData(_info._devIndex, command, &len);
    }

    auto check_ack(uint8_t outByte) noexcept -> bool
    {
        ULONG len = 4;
        uint8_t command[mCH341_PACKET_LENGTH];
        command[0] = mCH341A_CMD_I2C_STREAM;
        command[1] = mCH341A_CMD_I2C_STM_OUT;
        command[2] = outByte;
        command[3] = mCH341A_CMD_I2C_STM_END;
        
        ULONG bufLen = 0;
        uint8_t buffer[16] = {0, };

        if(CH341WriteRead(_info._devIndex, len, command, mCH341A_CMD_I2C_STM_MAX, 1, &bufLen, buffer )) 
        {
            // Bit 7 of the returned data represents the ACK response bit, ACK=0 is valid
            if (bufLen && (buffer[bufLen - 1] & 0x80) == 0) { return true; }
        }
        return false;
    }
};
} // namespace detail

auto CH341::__create(DeviceInfo* info) noexcept -> std::shared_ptr<CH341>
{
    switch(info->_type)
    {
        case DeviceInfo::CH341_I2C:
            return std::make_shared<detail::I2C>(reinterpret_cast<CH341_I2CInfo*>(info));
        case DeviceInfo::CH341_SPI:
            return nullptr;
        default :
            if constexpr (STRICT_MODE_ENABLED)
            {
                assert(false && "Invalid DeviceType");
            }
            return nullptr;
    }
}
} // namespace common::communication