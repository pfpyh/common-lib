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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "common/communication/driver/CH341.hpp"

namespace common::communication::driver::test
{
// TEST(test_CH341, read)
// {
//     // given
//     CH341::I2C info;
//     info._index = 0;
//     info._address = 0x68;
//     info._speed = CH341::I2C::Standard;

//     auto device = CH341::create(&info);
//     ASSERT_TRUE(device->open());

//     // when
//     unsigned char buffer[14] = {0, };
//     device->read(0x3B, buffer, sizeof(buffer));

//     int16_t ax = (((int16_t)buffer[0]) << 8) | buffer[1];
//     int16_t ay = (((int16_t)buffer[2]) << 8) | buffer[3];
//     int16_t az = (((int16_t)buffer[4]) << 8) | buffer[5];
//     int16_t gx = (((int16_t)buffer[8]) << 8) | buffer[9];
//     int16_t gy = (((int16_t)buffer[10]) << 8) | buffer[11];
//     int16_t gz = (((int16_t)buffer[12]) << 8) | buffer[13];

//     std::cout << "ax=" << ax << ", ay=" << ay << ", az=" << az << std::endl;
//     std::cout << "gx=" << gx << ", gy=" << gy << ", gz=" << gz << std::endl;

//     // then
//     device->close();
// }
} // namespace common::communication::driver::test