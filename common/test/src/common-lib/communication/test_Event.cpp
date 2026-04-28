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
#include "common/asio/AsyncEventBroker.hpp"
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

class test_Event : public ::testing::Test
{
public:
    Connection conn;
    
    void SetUp() override
    {
        static const std::unordered_map<std::string_view, uint16_t> portMap = {
            {"async_event",                    38001},
            {"topic_isolation",                38002},
            {"multiple_subscribers_same_topic",38003},
            {"multiple_publishes",             38004},
            {"publish_before_connect",         38005},
            {"double_regist",                  38006},
            {"double_subscribe",               38007},
            {"unsubscribe",                    38008},
        };

        conn._protocol = Protocol::TCP;
        conn._address  = "127.0.0.1";
        auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        auto it = portMap.find(info->name());
        conn._port = (it != portMap.end()) ? it->second : 38000;

        asio::IOContext::get_instance()->run();
        broker = std::make_unique<asio::AsyncEventBroker>();
        broker->run(conn);
    }

    void TearDown() override
    {
        broker->stop();
        broker.reset();
        asio::IOContext::get_instance()->stop();
    }

    std::unique_ptr<asio::AsyncEventBroker> broker;
};

TEST_F(test_Event, async_event)
{
    // given
    auto promise = std::make_shared<std::promise<DataType>>();
    auto future  = promise->get_future();

    // when
    auto provider = EventPublisher<DataType>::create(conn, 10);
    provider->regist();

    auto consumer = EventSubscriber<DataType>::create(conn, 10);
    consumer->subscribe([promise](const DataType& data) {
        promise->set_value(data);
    }, [&provider]() {
        provider->publish(DataType());
    });

    // then
    const auto success = future.wait_for(std::chrono::seconds(1));
    ASSERT_TRUE(success == std::future_status::ready);
}

TEST_F(test_Event, topic_isolation)
{
    // given
    auto promise = std::make_shared<std::promise<void>>();
    auto future  = promise->get_future();

    auto provider = EventPublisher<DataType>::create(conn, 20);
    provider->regist();

    auto consumer = EventSubscriber<DataType>::create(conn, 10);
    consumer->subscribe([promise](const DataType&) { promise->set_value(); });

    // when
    // Use a dummy subscriber on topic 20 to guarantee publish happens after broker registration
    auto ready = std::make_shared<std::promise<void>>();
    auto dummy = EventSubscriber<DataType>::create(conn, 20);
    dummy->subscribe([](const DataType&) {}, [&provider, ready]() {
        provider->publish(DataType());
        ready->set_value();
    });
    ready->get_future().wait_for(std::chrono::seconds(1));

    // then
    const auto status = future.wait_for(std::chrono::milliseconds(200));
    ASSERT_EQ(status, std::future_status::timeout);
}

TEST_F(test_Event, multiple_subscribers_same_topic)
{
    // given
    auto promise1 = std::make_shared<std::promise<void>>();
    auto promise2 = std::make_shared<std::promise<void>>();
    auto future1  = promise1->get_future();
    auto future2  = promise2->get_future();

    auto provider = EventPublisher<DataType>::create(conn, 10);
    provider->regist();

    // when
    // Publish only after both subscribers have received ACK from the broker
    auto subCount    = std::make_shared<std::atomic<int>>(0);
    auto onSubscribed = [&provider, subCount]() {
        if(subCount->fetch_add(1) + 1 == 2) { provider->publish(DataType()); }
    };

    auto consumer1 = EventSubscriber<DataType>::create(conn, 10);
    consumer1->subscribe([promise1](const DataType&) { promise1->set_value(); }, onSubscribed);

    auto consumer2 = EventSubscriber<DataType>::create(conn, 10);
    consumer2->subscribe([promise2](const DataType&) { promise2->set_value(); }, onSubscribed);

    // then
    ASSERT_EQ(future1.wait_for(std::chrono::seconds(1)), std::future_status::ready);
    ASSERT_EQ(future2.wait_for(std::chrono::seconds(1)), std::future_status::ready);
}

TEST_F(test_Event, multiple_publishes)
{
    // given
    constexpr int N  = 3;
    auto promise     = std::make_shared<std::promise<void>>();
    auto future      = promise->get_future();
    auto recvCount   = std::make_shared<std::atomic<int>>(0);

    auto provider = EventPublisher<DataType>::create(conn, 10);
    provider->regist();

    // when
    auto consumer = EventSubscriber<DataType>::create(conn, 10);
    consumer->subscribe([recvCount, promise, N](const DataType&) {
        if(recvCount->fetch_add(1) + 1 == N) { promise->set_value(); }
    }, [&provider, N]() {
        for(int i = 0; i < N; ++i) { provider->publish(DataType()); }
    });

    // then
    ASSERT_EQ(future.wait_for(std::chrono::seconds(1)), std::future_status::ready);
    ASSERT_EQ(recvCount->load(), N);
}

TEST_F(test_Event, publish_before_connect)
{
    // given
    auto provider = EventPublisher<DataType>::create(conn, 10);

    // when
    ASSERT_NO_THROW(provider->publish(DataType()));

    // then: normal communication still works after the ignored publish
    auto promise = std::make_shared<std::promise<void>>();
    auto future  = promise->get_future();

    provider->regist();
    auto consumer = EventSubscriber<DataType>::create(conn, 10);
    consumer->subscribe([promise](const DataType&) { promise->set_value(); },
                        [&provider]() { provider->publish(DataType()); });

    ASSERT_EQ(future.wait_for(std::chrono::seconds(1)), std::future_status::ready);
}

TEST_F(test_Event, double_regist)
{
    // given
    // Skipped in strict mode: the second regist() triggers std::abort inside
    // an IOContext worker thread, which cannot be caught via ASSERT_DEATH (fork-based)
#if !STRICT_MODE_ENABLED
    auto recvCount = std::make_shared<std::atomic<int>>(0);
    auto promise   = std::make_shared<std::promise<void>>();
    auto future    = promise->get_future();

    auto provider = EventPublisher<DataType>::create(conn, 10);
    provider->regist();

    // when
    auto consumer = EventSubscriber<DataType>::create(conn, 10);
    consumer->subscribe([recvCount](const DataType&) { recvCount->fetch_add(1); },
    [&provider, promise]() {
        provider->regist();
        provider->publish(DataType());
        promise->set_value();
    });

    // then
    ASSERT_EQ(future.wait_for(std::chrono::seconds(1)), std::future_status::ready);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(recvCount->load(), 1);
#endif
}

TEST_F(test_Event, double_subscribe)
{
    // given
    // Skipped in strict mode: the second subscribe() triggers std::abort inside
    // an IOContext worker thread, which cannot be caught via ASSERT_DEATH (fork-based)
#if !STRICT_MODE_ENABLED
    auto recvCount = std::make_shared<std::atomic<int>>(0);
    auto promise   = std::make_shared<std::promise<void>>();
    auto future    = promise->get_future();

    auto provider = EventPublisher<DataType>::create(conn, 10);
    provider->regist();

    // when
    auto consumer = EventSubscriber<DataType>::create(conn, 10);
    consumer->subscribe([recvCount](const DataType&) { recvCount->fetch_add(1); },
    [&consumer, &provider, promise, recvCount]() {
        consumer->subscribe([recvCount](const DataType&) { recvCount->fetch_add(1); });
        provider->publish(DataType());
        promise->set_value();
    });

    // then
    future.wait_for(std::chrono::seconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(recvCount->load(), 1);
#endif
}

TEST_F(test_Event, unsubscribe)
{
    // given
    auto recvCount   = std::make_shared<std::atomic<int>>(0);
    auto firstRecv   = std::make_shared<std::promise<void>>();
    auto firstFuture = firstRecv->get_future();

    auto provider = EventPublisher<DataType>::create(conn, 10);
    provider->regist();

    auto consumer = EventSubscriber<DataType>::create(conn, 10);
    consumer->subscribe([recvCount, firstRecv](const DataType&) {
        if(recvCount->fetch_add(1) == 0) { firstRecv->set_value(); }
    }, [&provider]() { provider->publish(DataType()); });

    ASSERT_EQ(firstFuture.wait_for(std::chrono::seconds(1)), std::future_status::ready);

    // when
    consumer->unsubscribe();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    provider->publish(DataType());

    // then
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_EQ(recvCount->load(), 1);
}
} // namespace common::communication::test