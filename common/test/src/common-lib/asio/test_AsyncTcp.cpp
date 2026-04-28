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
#include "common/asio/AsyncTcp.hpp"
#include "common/Logger.hpp"

#include <atomic>
#include <chrono>
#include <numeric>

namespace common::asio::test
{
namespace
{
communication::Connection makeConn(uint32_t port)
{
    communication::Connection conn;
    conn._protocol = communication::Protocol::TCP;
    conn._address  = "127.0.0.1";
    conn._port     = port;
    return conn;
}
} // namespace

class test_AsyncTcp : public testing::Test
{
public:
    static auto SetUpTestSuite() -> void { IOContext::get_instance()->run(); }
    static auto TearDownTestSuite() -> void { IOContext::get_instance()->stop(); }
};

TEST_F(test_AsyncTcp, small_send_receive)
{
    const auto conn = makeConn(31000);
    const std::vector<uint8_t> sendData = {0x01, 0x02, 0x03, 0x04, 0x05};

    auto receivePromise = std::make_shared<std::promise<std::vector<uint8_t>>>();
    auto receiveFuture  = receivePromise->get_future();
    auto promiseSet     = std::make_shared<std::atomic<bool>>(false);

    auto listener = AsyncTcpListener::create(conn);
    listener->listen([receivePromise, promiseSet](std::shared_ptr<AsyncTcpSocket> client) {
        client->receive([receivePromise, promiseSet, client](const std::vector<uint8_t>& buffer) {
            if(!promiseSet->exchange(true)) { receivePromise->set_value(buffer); }
        });
    });

    auto sendPromise = std::make_shared<std::promise<void>>();
    auto sendFuture  = sendPromise->get_future();
    auto clientSocket = AsyncTcpSocket::create(conn);
    clientSocket->connect([clientSocket, sendData, sendPromise]() {
        clientSocket->send(sendData, [sendPromise]([[maybe_unused]] size_t bytes) {
            sendPromise->set_value();
        });
    });

    sendFuture.wait();
    EXPECT_EQ(receiveFuture.get(), sendData);

    listener->stop();
}

TEST_F(test_AsyncTcp, large_size_buffer)
{
    const auto conn = makeConn(31001);

    std::vector<uint8_t> sendData(10000);
    std::iota(sendData.begin(), sendData.end(), 0);

    auto receivePromise = std::make_shared<std::promise<std::vector<uint8_t>>>();
    auto receiveFuture  = receivePromise->get_future();
    auto promiseSet     = std::make_shared<std::atomic<bool>>(false);

    auto listener = AsyncTcpListener::create(conn);
    listener->listen([receivePromise, promiseSet](std::shared_ptr<AsyncTcpSocket> client) {
        client->receive([receivePromise, promiseSet, client](const std::vector<uint8_t>& buffer) {
            if(!promiseSet->exchange(true)) { receivePromise->set_value(buffer); }
        });
    });

    auto sendPromise  = std::make_shared<std::promise<void>>();
    auto sendFuture   = sendPromise->get_future();
    auto clientSocket = AsyncTcpSocket::create(conn);
    clientSocket->connect([clientSocket, sendData, sendPromise]() {
        clientSocket->send(sendData, [sendPromise]([[maybe_unused]] size_t bytes) {
            sendPromise->set_value();
        });
    });

    sendFuture.wait();
    EXPECT_EQ(receiveFuture.get(), sendData);

    listener->stop();
}

TEST_F(test_AsyncTcp, multiple_messages_ordered)
{
    const auto conn        = makeConn(31002);
    const int  messageCount = 5;

    auto receivedMessages = std::make_shared<std::vector<std::vector<uint8_t>>>();
    auto mu               = std::make_shared<std::mutex>();
    auto donePromise      = std::make_shared<std::promise<void>>();
    auto doneFuture       = donePromise->get_future();
    auto promiseSet       = std::make_shared<std::atomic<bool>>(false);

    auto listener = AsyncTcpListener::create(conn);
    listener->listen([receivedMessages, mu, donePromise, promiseSet, messageCount]
                     (std::shared_ptr<AsyncTcpSocket> client) {
        client->receive([receivedMessages, mu, donePromise, promiseSet, messageCount, client]
                        (const std::vector<uint8_t>& buffer) {
            std::lock_guard<std::mutex> lock(*mu);
            receivedMessages->push_back(buffer);
            if(static_cast<int>(receivedMessages->size()) == messageCount)
            {
                if(!promiseSet->exchange(true)) { donePromise->set_value(); }
            }
        });
    });

    auto clientSocket = AsyncTcpSocket::create(conn);
    auto allSentPromise = std::make_shared<std::promise<void>>();
    auto allSentFuture  = allSentPromise->get_future();

    clientSocket->connect([clientSocket, messageCount, allSentPromise]() {
        auto counter = std::make_shared<std::atomic<int>>(0);
        for(int i = 0; i < messageCount; ++i)
        {
            std::vector<uint8_t> msg = {static_cast<uint8_t>(i)};
            clientSocket->send(msg, [counter, messageCount, allSentPromise]
                               ([[maybe_unused]] size_t bytes) {
                if(counter->fetch_add(1) + 1 == messageCount)
                    allSentPromise->set_value();
            });
        }
    });

    allSentFuture.wait();
    doneFuture.wait();

    ASSERT_EQ(static_cast<int>(receivedMessages->size()), messageCount);
    for(int i = 0; i < messageCount; ++i)
    {
        EXPECT_EQ((*receivedMessages)[i], std::vector<uint8_t>{static_cast<uint8_t>(i)});
    }

    listener->stop();
}

TEST_F(test_AsyncTcp, connect_to_invalid_address)
{
    communication::Connection conn;
    conn._protocol = communication::Protocol::TCP;
    conn._address  = "127.0.0.1";
    conn._port     = 39999;

    auto errorPromise = std::make_shared<std::promise<bool>>();
    auto errorFuture  = errorPromise->get_future();
    auto promiseSet   = std::make_shared<std::atomic<bool>>(false);

    auto clientSocket = AsyncTcpSocket::create(conn);
    clientSocket->connect(
        []() { FAIL() << "Should not connect"; },
        [errorPromise, promiseSet](const auto&) {
            if(!promiseSet->exchange(true)) { errorPromise->set_value(true); }
        }
    );

    EXPECT_TRUE(errorFuture.get());
}

TEST_F(test_AsyncTcp, disconnect_triggers_eof_on_receiver)
{
    const auto conn = makeConn(31003);

    auto eofPromise = std::make_shared<std::promise<bool>>();
    auto eofFuture  = eofPromise->get_future();
    auto promiseSet = std::make_shared<std::atomic<bool>>(false);

    auto listener = AsyncTcpListener::create(conn);
    listener->listen([eofPromise, promiseSet](std::shared_ptr<AsyncTcpSocket> client) {
        client->receive(
            [client]([[maybe_unused]] const std::vector<uint8_t>& buffer) {},
            [eofPromise, promiseSet](const auto& ec) {
                LogInfo << "Called:" << ec;
                if(ec == ErrorCode::END_OF_FILE || ec == ErrorCode::CONNECTION_LOST)
                {
                    if(!promiseSet->exchange(true)) { eofPromise->set_value(true); }
                }
            }
        );
    });

    auto connectPromise = std::make_shared<std::promise<void>>();
    auto connectFuture  = connectPromise->get_future();
    auto clientSocket   = AsyncTcpSocket::create(conn);
    clientSocket->connect([connectPromise]() { connectPromise->set_value(); });
    connectFuture.wait();

    clientSocket->disconnect();

    EXPECT_TRUE(eofFuture.get());

    listener->stop();
}

TEST_F(test_AsyncTcp, double_connect_is_ignored)
{
    const auto conn = makeConn(31004);

    auto listener = AsyncTcpListener::create(conn);
    listener->listen([]([[maybe_unused]] std::shared_ptr<AsyncTcpSocket> client) {});

    auto connectCount = std::make_shared<std::atomic<int>>(0);
    auto firstConnect = std::make_shared<std::promise<void>>();
    auto firstFuture  = firstConnect->get_future();

    auto clientSocket = AsyncTcpSocket::create(conn);
    clientSocket->connect([clientSocket, connectCount, firstConnect, conn]() {
        connectCount->fetch_add(1);
        firstConnect->set_value();

        clientSocket->connect([connectCount]() {
            connectCount->fetch_add(1);
        });
    });

    firstFuture.wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_EQ(connectCount->load(), 1);

    listener->stop();
}
} // namespace common::asio::test