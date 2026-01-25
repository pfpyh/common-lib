/**********************************************************************
MIT License

Copyright (c) 2026 Park Younghwan

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

#include "common/asio/IOContext.hpp"
#include "common/communication/Event.hpp"

namespace common::communication::test
{
struct DataType
{
    int32_t _1 = 0;
    uint8_t _2 = 0;
    unsigned char _3 = 'a';
};

DataType& operator<<(DataType& data, [[maybe_unused]] const Bytes& bytes)
{
    return data;
}

Bytes& operator<<(Bytes& bytes, [[maybe_unused]] const DataType& data)
{
    return bytes;
}

TEST(Event, async_event)
{
    // given
    Connection conn;
    conn._protocol = Protocol::TCP;
    conn._address = "127.0.0.1";
    conn._port = 38000;

    asio::IOContext::get_instance()->run();

    // when
    std::shared_ptr<std::promise<DataType>> promise = std::make_shared<std::promise<DataType>>();
    std::future<DataType> future = promise->get_future();

    auto provider = EventPublisher<DataType>::create(conn);
    provider->regist();

    auto consumer = EventSubscriber<DataType>::create(conn);
    consumer->subscribe([promise = promise](const DataType& data) {
        LogInfo << "Received";
        promise->set_value(data);
    }, [&provider]() { provider->publish(DataType()); });

    // then
    const auto success = future.wait_for(std::chrono::seconds(1));
    ASSERT_TRUE(success == std::future_status::ready);
    [[maybe_unused]] auto data = future.get();

    asio::IOContext::get_instance()->stop();
}
} // namespace common::communication::test