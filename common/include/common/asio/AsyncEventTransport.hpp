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
#pragma once

#include "common/communication/EventTransport.hpp"
#include "common/asio/AsyncTcp.hpp"
#include "common/Logger.hpp"

#include <shared_mutex>
#include <mutex>

namespace common::communication::asio
{
using namespace ::common::asio;

class TcpTransport : public EventTransport
                   , public std::enable_shared_from_this<TcpTransport>
{
private :
    std::shared_ptr<AsyncTcpListener> _listner;
    std::vector<std::shared_ptr<AsyncTcpSocket>> _subscribers;
    std::shared_mutex _lock;

public :
    ~TcpTransport() { disconnect(); }

    auto connect(const Connection& conn) -> void override
    {
        if(_listner)
        {
            LogDebug << "already connected";
            if constexpr (STRICT_MODE_ENABLED) { std::abort(); }
            return;
        }

        _listner = AsyncTcpListener::create(conn);
        auto self = shared_from_this();
        _listner->listen([self](std::shared_ptr<AsyncTcpSocket> subscriber) {
            std::unique_lock<std::shared_mutex> scopedLock(self->_lock);
            self->_subscribers.push_back(std::move(subscriber));
        });
    }

    auto connected() -> bool override
    {
        return _listner != nullptr;
    }

    auto disconnect() -> void override
    {
        if(!_listner) { return; }
        _listner->stop();
        _listner.reset();

        std::unique_lock<std::shared_mutex> lock(_lock);
        _subscribers.clear();
    }

    auto publish(const std::vector<uint8_t> bytes) -> void override
    {
        std::shared_lock<std::shared_mutex> scopedLock(_lock);
        auto self = shared_from_this();
        for(auto& subscriber : _subscribers) 
        { 
            auto weak = std::weak_ptr<AsyncTcpSocket>(subscriber);
            subscriber->send(bytes, nullptr,
                             [self, weak](const auto& ec) {
                LogError << "send error: " << ec.message();
                auto target = weak.lock();
                if (!target) { return; }

                std::unique_lock<std::shared_mutex> lock(self->_lock);
                self->_subscribers.erase(
                    std::remove(self->_subscribers.begin(), self->_subscribers.end(), target), 
                    self->_subscribers.end()
                );
            });
        }
    }
};

class TcpSubTransport : public EventTransport
                      , public std::enable_shared_from_this<TcpSubTransport>
{
private :
    std::shared_ptr<AsyncTcpSocket> _server;
    onMessage _handler;
    bool _connected = false;

public :
    ~TcpSubTransport() { disconnect(); }

    auto connect(const Connection& conn) -> void override
    {
        if(_server)
        {
            LogDebug << "already connected";
            if constexpr (STRICT_MODE_ENABLED) { std::abort(); }
            return;
        }

        _server = AsyncTcpSocket::create(conn);
    }

    auto connected() -> bool override
    {
        return _connected;
    }

    auto disconnect() -> void override
    {
        if(!_server) { return; }
        _server->disconnect();
        _server.reset();
        _connected = false;
    }

    auto subscribe(onMessage onMessageHandler, onSubscribed onSubscribedHandler) -> void override
    {
        if(!_server)
        {
            LogDebug << "socket is not created";
            if constexpr (STRICT_MODE_ENABLED) { std::abort(); }
            return;
        }

        _handler = std::move(onMessageHandler);
        auto self = shared_from_this();
        _server->connect([self, onSubscribedHandler = std::move(onSubscribedHandler)]() {
            if(!self->_server) { LogDebug << "connect operation is cancled"; }
            else 
            { 
                LogDebug << "connected"; 
                self->_connected = true;
                onSubscribedHandler();
            };
            self->_server->receive([self](const std::vector<uint8_t>& bytes) {
                self->_handler(bytes);
            }, [](const auto& ec) { LogError << "receive (" << ec.message() << ")"; });
        }, [](const auto& ec) { LogError << "connect (" << ec.message() << ")"; });
    }
};
} // common::communication::asio