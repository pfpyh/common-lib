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

#include "common/communication/Socket.hpp"
#include "common/thread/Thread.hpp"
#include "common/logging/Logger.hpp"

#include <thread>

namespace common::communication::test
{
TEST(test_Socket, send_recv)
{
    // given
    const std::string server_msg("SERVER MSG");
    std::string server_rcv_msg;

    const std::string client_msg("CLIENT MSG");
    std::string client_rcv_msg;

    auto server_future = Thread::async([&server_msg, &server_rcv_msg](){ 
        auto server = Socket::create(SocketType::SERVER);
        server->prepare("127.0.0.1", 8080);
        server->open();        

        const size_t bufferSize = 11;
        char buffer[bufferSize] = {0,};
        auto readSize = server->read(buffer, bufferSize);
        server_rcv_msg.assign(buffer, 0, readSize);

        server->send(server_msg.c_str(), server_msg.size());
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto client_future = Thread::async([&client_msg, &client_rcv_msg](){ 
        auto client = Socket::create(SocketType::CLIENT);
        client->prepare("127.0.0.1", 8080);
        client->open();

        client->send(client_msg.c_str(), client_msg.size());

        const size_t bufferSize = 11;
        char buffer[bufferSize] = {0,};
        auto readSize = client->read(buffer, bufferSize);
        client_rcv_msg.assign(buffer, 0, readSize);
    });    

    // when
    server_future.wait();
    client_future.wait();
        
    // then
    EXPECT_EQ(server_rcv_msg, client_msg);
    EXPECT_EQ(client_rcv_msg, server_msg);
}
} // namespace common::communication::test
