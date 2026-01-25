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

#include "common/asio/AsyncEventBroker.hpp"
#include "common/communication/EventFrame.hpp"
#include "common/Logger.hpp"

#include <mutex>

namespace common::asio
{
auto ClientSession::establish() -> void
{
    auto self = shared_from_this();
    _socket->receive([self](const Bytes& raw) {
        self->onReceive(raw);
    }, [self]([[maybe_unused]] const auto& ec) {
        LogDebug << "client disconnected: " << ec;
        self->_onUnsubscribe(self);
    });
}

auto ClientSession::send(const Bytes& bytesFrame) -> void
{
    _socket->send(bytesFrame, nullptr, [](const auto& ec) {
        LogError << "broker send error: " << ec;
    });
}

auto ClientSession::onReceive(const Bytes& raw) -> void
{
    using namespace common::communication;

    auto frame = Frame::parse(raw);
    switch(frame._type)
    {
    case Frame::REGIST:
        _onSubscribe(frame._topic, shared_from_this());
        break;
    case Frame::UNREGIST:
        _onUnsubscribe(shared_from_this());
        break;
    case Frame::DATA:
        _onEvent(frame._topic, frame.payload);
        break;
    default:
        LogError << "unknown frame type: " << static_cast<int>(frame._type);
    }
}

auto TopicRegistry::regist(uint16_t topic, std::weak_ptr<ClientSession> session) -> void
{
    std::unique_lock scopedLock(_lock);
    _sessions[topic].push_back(std::move(session));
}

auto TopicRegistry::unregist(const std::shared_ptr<ClientSession>& session) -> void
{
    std::unique_lock scopedLock(_lock);
    for(auto it = _sessions.begin(); it != _sessions.end(); )
    {
        auto& sessions = it->second;
        sessions.erase(
        std::remove_if(sessions.begin(), sessions.end(),
            [&session](const std::weak_ptr<ClientSession>& weak) {
                if(auto locked = weak.lock()) { return locked == session; }
                return false;
            }),
        sessions.end()
        );
        
        if(sessions.empty()) { it = _sessions.erase(it); }
        else { ++it; }
    }
}

auto TopicRegistry::route(uint16_t topic, const Bytes& payload) -> void
{
    using namespace common::communication;

    auto bytesFrame = Frame::make(topic, Frame::DATA, payload);

    std::shared_lock scopedLock(_lock);
    auto it = _sessions.find(topic);
    if(it == _sessions.end()) { return; }

    for(auto& weak : it->second)
    {
        if(auto session = weak.lock()) { session->send(bytesFrame); }
    }
}

auto AsyncEventBroker::run(const communication::Connection& conn) -> void
{
    _listener = AsyncTcpListener::create(conn);
    _listener->listen([this](std::shared_ptr<AsyncTcpSocket> socket) {
        onAccept(std::move(socket));
    });
    LogDebug << "EventBroker started on port " << conn._port;
}

auto AsyncEventBroker::stop() -> void
{
    if(!_listener) { return; }
    _listener->stop();
    _listener.reset();
}

auto AsyncEventBroker::onAccept(std::shared_ptr<AsyncTcpSocket> socket) -> void
{
    using namespace common::communication;

    auto session = std::make_shared<ClientSession>(
        std::move(socket),
        [registry = _registry](uint16_t topic, std::shared_ptr<ClientSession> session) {
            registry->regist(topic, session);
            auto frame = Frame::make(topic, Frame::ACK, Bytes());
            session->send(frame);
        },
        [registry = _registry](std::shared_ptr<ClientSession> session) {
            registry->unregist(session);
        },
        [registry = _registry](uint16_t topic, const Bytes& payload) {
            registry->route(topic, payload);
        }
    );
    session->establish();
}
} // namespace common::asio