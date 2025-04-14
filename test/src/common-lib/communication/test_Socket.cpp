#include <gtest/gtest.h>

#include "common/communication/Socket.hpp"
#include "common/thread/Thread.hpp"

#include "common/logging/Logger.hpp"

namespace common::test
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
} // namespace common::test