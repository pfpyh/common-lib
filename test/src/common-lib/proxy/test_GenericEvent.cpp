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

#include "common/proxy/GenericEvent.hpp"

namespace common::test
{
TEST(test_GenericEvent, DISABLED_publishTestMessage)
{
    // given
    struct DataType
    {
        int32_t _1 = 0;
        int32_t _2 = 0;
    };

    GenericEventBus bus;
    DataType data{100, -50};

    // when
    DataType recvData;
    const auto subId = bus.subscribe<DataType>("Test", 
                                               [&recvData](const DataType& data){
        recvData = data;
    });
    bus.publish<DataType>("Test", data);
    bus.unsubscribe(subId);

    // then
    ASSERT_EQ(recvData._1, data._1);
    ASSERT_EQ(recvData._2, data._2);
}
} // namespace common::test