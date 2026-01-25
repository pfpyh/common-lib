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

#include "common/asio/AsyncEventTransport.hpp"
#include "common/communication/EventFrame.hpp"
#include "common/Logger.hpp"

namespace common::asio
{
auto TcpTransport::connect() -> void
{
    if(_socket)
    {
        LogDebug << "connection already established";
        if constexpr (STRICT_MODE_ENABLED) { std::abort(); }
        return;
    }
    _socket = AsyncTcpSocket::create(_conn);

    auto self = shared_from_this();
    _socket->connect([self]() {
        LogDebug << "connected to broker";
        self->_connected.store(true);
    }, [](const auto& ec) { LogError << "connect: " << ec; });
}

auto TcpTransport::connected() -> bool
{
    return _connected.load();
}

auto TcpTransport::disconnect() -> void
{
    using namespace common::communication;

    ::asio::post(_strand, [self = shared_from_this()]() {
        if(!self->_socket) { return; }

        self->_socket->disconnect();
        self->_socket.reset();
        self->_connected.store(false);
    });
}

auto TcpTransport::publish(const Bytes& payload) -> void
{
    using namespace common::communication;

    auto self = shared_from_this();
    ::asio::post(_strand, [self, payload]() {
        if(!self->_connected.load())
        { 
            LogDebug << "event is not connected";
            return; 
        }

        self->_socket->send(Frame::make(self->_topic, Frame::DATA, payload), 
                            nullptr, [](const auto& ec) {
            if (ec) { LogError << "send error: " << ec; }
        });
    });
}

auto TcpTransport::subscribe(communication::onMessage onMessageHandler, 
                             communication::onSubscribed onSubscribedHandler) -> void
{
    using namespace common::communication;

    if(_socket)
    {
        LogDebug << "connection already established";
        if constexpr (STRICT_MODE_ENABLED) { std::abort(); }
        return;
    }
    _socket = AsyncTcpSocket::create(_conn);

    auto self = shared_from_this();
    _socket->connect([self, onMessage = std::move(onMessageHandler), onSubscribed = std::move(onSubscribedHandler)]() mutable {
        self->_connected.store(true);
        self->_socket->send(Frame::make(self->_topic, Frame::REGIST));
        self->_socket->receive([onMessage = std::move(onMessage),
                                onSubscribed = std::move(onSubscribed)]
                                (const Bytes& raw) {
            auto frame = Frame::parse(raw);
            switch(frame._type)
            {
                case Frame::ACK: // Subscribed
                    onSubscribed();
                    break;
                case Frame::DATA:
                    onMessage(frame.payload);
                    break;
                default:
                    LogError << "undefined message type received";
                    break;
            }
        }, [](const auto& ec) { LogError << "receive: " << ec; });

    }, [](const auto& ec) { LogError << "connect: " << ec; });
}
} // namespace common::asio