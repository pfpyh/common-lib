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

#include "common/asio/IOContext.hpp"
#include "common/asio/AsyncUdp.hpp"

#include <atomic>
#include <chrono>
#include <numeric>
#include <thread>

namespace common::asio::test
{
namespace
{
communication::Connection makeConn(uint32_t port)
{
    communication::Connection conn;
    conn._protocol = communication::Protocol::UDP;
    conn._address  = "127.0.0.1";
    conn._port     = port;
    return conn;
}
} // namespace

class test_AsyncUdp : public testing::Test
{
public:
    static auto SetUpTestSuite() -> void { IOContext::get_instance()->run(); }
    static auto TearDownTestSuite() -> void { IOContext::get_instance()->stop(); }
};

TEST_F(test_AsyncUdp, small_send_receive)
{
    const auto serverConn = makeConn(32000);
    const auto clientConn = makeConn(32001);
    const std::vector<uint8_t> sendData = {0x01, 0x02, 0x03, 0x04, 0x05};

    auto receivePromise = std::make_shared<std::promise<std::vector<uint8_t>>>();
    auto receiveFuture  = receivePromise->get_future();
    auto promiseSet     = std::make_shared<std::atomic<bool>>(false);

    auto server = AsyncUdpSocket::create(serverConn);
    server->open();
    server->receive([receivePromise, promiseSet, server]
                    (const std::vector<uint8_t>& data, ::asio::ip::udp::endpoint /*sender*/) {
        if(!promiseSet->exchange(true)) { receivePromise->set_value(data); }
    });

    auto sendPromise = std::make_shared<std::promise<void>>();
    auto sendFuture  = sendPromise->get_future();
    auto client = AsyncUdpSocket::create(clientConn);
    client->open();
    client->send(serverConn, sendData, [sendPromise]([[maybe_unused]] size_t bytes) {
        sendPromise->set_value();
    });

    sendFuture.wait();
    EXPECT_EQ(receiveFuture.get(), sendData);

    server->close();
    client->close();
}

TEST_F(test_AsyncUdp, large_size_buffer)
{
    const auto serverConn = makeConn(32002);
    const auto clientConn = makeConn(32003);

    std::vector<uint8_t> sendData(1400);
    std::iota(sendData.begin(), sendData.end(), 0);

    auto receivePromise = std::make_shared<std::promise<std::vector<uint8_t>>>();
    auto receiveFuture  = receivePromise->get_future();
    auto promiseSet     = std::make_shared<std::atomic<bool>>(false);

    auto server = AsyncUdpSocket::create(serverConn);
    server->open();
    server->receive([receivePromise, promiseSet, server]
                    (const std::vector<uint8_t>& data, ::asio::ip::udp::endpoint /*sender*/) {
        if(!promiseSet->exchange(true)) { receivePromise->set_value(data); }
    });

    auto sendPromise = std::make_shared<std::promise<void>>();
    auto sendFuture  = sendPromise->get_future();
    auto client = AsyncUdpSocket::create(clientConn);
    client->open();
    client->send(serverConn, sendData, [sendPromise]([[maybe_unused]] size_t bytes) {
        sendPromise->set_value();
    });

    sendFuture.wait();
    EXPECT_EQ(receiveFuture.get(), sendData);

    server->close();
    client->close();
}

TEST_F(test_AsyncUdp, multiple_messages_ordered)
{
    const auto serverConn  = makeConn(32004);
    const auto clientConn  = makeConn(32005);
    const int  messageCount = 5;

    auto receivedMessages = std::make_shared<std::vector<std::vector<uint8_t>>>();
    auto mu               = std::make_shared<std::mutex>();
    auto donePromise      = std::make_shared<std::promise<void>>();
    auto doneFuture       = donePromise->get_future();
    auto promiseSet       = std::make_shared<std::atomic<bool>>(false);

    auto server = AsyncUdpSocket::create(serverConn);
    server->open();
    server->receive([receivedMessages, mu, donePromise, promiseSet, messageCount, server]
                    (const std::vector<uint8_t>& data, ::asio::ip::udp::endpoint /*sender*/) {
        std::lock_guard<std::mutex> lock(*mu);
        receivedMessages->push_back(data);
        if(static_cast<int>(receivedMessages->size()) == messageCount)
        {
            if(!promiseSet->exchange(true)) { donePromise->set_value(); }
        }
    });

    auto client = AsyncUdpSocket::create(clientConn);
    client->open();

    auto allSentPromise = std::make_shared<std::promise<void>>();
    auto allSentFuture  = allSentPromise->get_future();
    auto counter        = std::make_shared<std::atomic<int>>(0);

    for(int i = 0; i < messageCount; ++i)
    {
        std::vector<uint8_t> msg = {static_cast<uint8_t>(i)};
        client->send(serverConn, msg, [counter, messageCount, allSentPromise]
                     ([[maybe_unused]] size_t bytes) {
            if(counter->fetch_add(1) + 1 == messageCount)
                allSentPromise->set_value();
        });
    }

    allSentFuture.wait();
    doneFuture.wait();

    ASSERT_EQ(static_cast<int>(receivedMessages->size()), messageCount);
    for(int i = 0; i < messageCount; ++i)
    {
        EXPECT_EQ((*receivedMessages)[i], std::vector<uint8_t>{static_cast<uint8_t>(i)});
    }

    server->close();
    client->close();
}

TEST_F(test_AsyncUdp, sender_endpoint_is_delivered)
{
    const auto serverConn = makeConn(32006);
    const auto clientConn = makeConn(32007);

    using Endpoint = ::asio::ip::udp::endpoint;
    auto endpointPromise = std::make_shared<std::promise<Endpoint>>();
    auto endpointFuture  = endpointPromise->get_future();
    auto promiseSet      = std::make_shared<std::atomic<bool>>(false);

    auto server = AsyncUdpSocket::create(serverConn);
    server->open();
    server->receive([endpointPromise, promiseSet, server]
                    (const std::vector<uint8_t>& /*data*/, Endpoint sender) {
        if(!promiseSet->exchange(true)) { endpointPromise->set_value(sender); }
    });

    auto client = AsyncUdpSocket::create(clientConn);
    client->open();
    client->send(serverConn, {0x42});

    const auto senderEndpoint = endpointFuture.get();
    EXPECT_EQ(senderEndpoint.address().to_string(), "127.0.0.1");
    EXPECT_EQ(senderEndpoint.port(), clientConn._port);

    server->close();
    client->close();
}

TEST_F(test_AsyncUdp, close_stops_receive_loop)
{
    const auto serverConn = makeConn(32008);
    const auto clientConn = makeConn(32009);

    auto receiveCount  = std::make_shared<std::atomic<int>>(0);
    auto firstReceived = std::make_shared<std::promise<void>>();
    auto firstFuture   = firstReceived->get_future();
    auto promiseSet    = std::make_shared<std::atomic<bool>>(false);

    auto server = AsyncUdpSocket::create(serverConn);
    server->open();
    server->receive([receiveCount, firstReceived, promiseSet, server]
                    (const std::vector<uint8_t>& /*data*/, ::asio::ip::udp::endpoint /*sender*/) {
        receiveCount->fetch_add(1);
        if(!promiseSet->exchange(true)) { firstReceived->set_value(); }
    });

    auto client = AsyncUdpSocket::create(clientConn);
    client->open();
    client->send(serverConn, {0x01});

    firstFuture.wait();
    server->close();

    client->send(serverConn, {0x02});
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_EQ(receiveCount->load(), 1);

    client->close();
}

TEST_F(test_AsyncUdp, send_to_unbound_port_does_not_error)
{
    const auto clientConn = makeConn(32010);
    const auto destConn   = makeConn(32011);

    auto sendPromise = std::make_shared<std::promise<bool>>();
    auto sendFuture  = sendPromise->get_future();
    auto promiseSet  = std::make_shared<std::atomic<bool>>(false);

    auto client = AsyncUdpSocket::create(clientConn);
    client->open();
    client->send(destConn, {0x01, 0x02, 0x03},
        [sendPromise, promiseSet]([[maybe_unused]] size_t bytes) {
            if(!promiseSet->exchange(true)) { sendPromise->set_value(true); }
        },
        [sendPromise, promiseSet](const ::asio::error_code&) {
            if(!promiseSet->exchange(true)) { sendPromise->set_value(false); }
        }
    );

    EXPECT_TRUE(sendFuture.get());

    client->close();
}

TEST_F(test_AsyncUdp, buffer_reuse_no_corruption)
{
    const auto serverConn   = makeConn(32020);
    const auto clientConn   = makeConn(32021);
    const int  messageCount = 50;
    const int  payloadSize  = 64;

    auto receivedMessages = std::make_shared<std::vector<std::vector<uint8_t>>>();
    auto mu               = std::make_shared<std::mutex>();
    auto donePromise      = std::make_shared<std::promise<void>>();
    auto doneFuture       = donePromise->get_future();
    auto promiseSet       = std::make_shared<std::atomic<bool>>(false);

    auto server = AsyncUdpSocket::create(serverConn, payloadSize);
    server->open();
    server->receive(
        [receivedMessages, mu, donePromise, promiseSet, messageCount, server]
        (const std::vector<uint8_t>& data, ::asio::ip::udp::endpoint /*sender*/) {
            std::lock_guard<std::mutex> lock(*mu);
            receivedMessages->push_back(data);
            if(static_cast<int>(receivedMessages->size()) == messageCount)
                if(!promiseSet->exchange(true)) { donePromise->set_value(); }
        });

    auto client = AsyncUdpSocket::create(clientConn);
    client->open();

    auto allSentPromise = std::make_shared<std::promise<void>>();
    auto allSentFuture  = allSentPromise->get_future();
    auto sentCounter    = std::make_shared<std::atomic<int>>(0);

    for(int i = 0; i < messageCount; ++i)
    {
        std::vector<uint8_t> payload(payloadSize, static_cast<uint8_t>(i));
        client->send(serverConn, payload,
            [sentCounter, messageCount, allSentPromise]([[maybe_unused]] size_t bytes) {
                if(sentCounter->fetch_add(1) + 1 == messageCount)
                    allSentPromise->set_value();
            });
    }

    allSentFuture.wait();
    const auto status = doneFuture.wait_for(std::chrono::seconds(3));
    ASSERT_EQ(status, std::future_status::ready) << "Not all packets received within timeout";

    std::lock_guard<std::mutex> lock(*mu);
    ASSERT_EQ(static_cast<int>(receivedMessages->size()), messageCount);

    for(const auto& msg : *receivedMessages)
    {
        ASSERT_EQ(static_cast<int>(msg.size()), payloadSize);

        const uint8_t expected = msg[0];
        for(int j = 1; j < payloadSize; ++j)
        {
            EXPECT_EQ(msg[j], expected)
                << "Buffer corruption at byte " << j
                << ": expected 0x" << std::hex << static_cast<int>(expected)
                << " but got 0x" << static_cast<int>(msg[j]);
        }
    }

    server->close();
    client->close();
}
} // namespace common::asio::test