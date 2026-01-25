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

#include "common/CommonHeader.hpp"
#include "common/asio/AsyncTcp.hpp"

#include <memory>
#include <unordered_map>
#include <shared_mutex>

namespace common::asio
{
/**
 * @brief Represents a single connected subscriber client.
 *
 * Owns the TCP socket and dispatches received frames to the broker via callbacks.
 * The session removes itself from the registry on EOF.
 */
class COMMON_LIB_API ClientSession : public std::enable_shared_from_this<ClientSession>
{
    using onSubscribe = std::function<void(uint16_t, std::shared_ptr<ClientSession>)>;
    using onUnsubscribe = std::function<void(std::shared_ptr<ClientSession>)>;
    using onEvent = std::function<void(uint16_t, const Bytes&)>;

private :
    std::shared_ptr<asio::AsyncTcpSocket> _socket;
    onSubscribe _onSubscribe;
    onUnsubscribe _onUnsubscribe;
    onEvent _onEvent;

public :
    explicit ClientSession(std::shared_ptr<asio::AsyncTcpSocket> socket,
                           onSubscribe onSubscribe,
                           onUnsubscribe onUnsubscribe,
                           onEvent onEvent)
        : _socket(std::move(socket))
        , _onSubscribe(std::move(onSubscribe))
        , _onUnsubscribe(std::move(onUnsubscribe))
        , _onEvent(std::move(onEvent)) {}

    /**
     * @brief Starts the async receive loop for this session.
     * @note Must be called immediately after construction. @c shared_from_this() is captured
     *       in the read handler, keeping the session alive for the duration of the connection.
     */
    auto establish() -> void;

    /**
     * @brief Sends a serialized frame to the connected client.
     * @param bytesFrame Serialized frame produced by @c Frame::make().
     */
    auto send(const Bytes& bytesFrame) -> void;

private :
    auto onReceive(const Bytes& raw) -> void;
};

/**
 * @brief Thread-safe topic-to-session routing table.
 *
 * Uses @c std::shared_mutex for concurrent-read, exclusive-write access.
 */
class COMMON_LIB_API TopicRegistry
{
private :
    std::unordered_map<uint16_t, std::vector<std::weak_ptr<ClientSession>>> _sessions;
    std::shared_mutex _lock;

public :
    /**
     * @brief Registers a session under a topic.
     * @param topic Topic identifier.
     * @param session Weak pointer to the session.
     * @note Expired weak_ptr entries from prior sessions are not pruned at registration time.
     */
    auto regist(uint16_t topic, std::weak_ptr<ClientSession> session) -> void;

    /**
     * @brief Removes a session from all topics and erases any topic entry that becomes empty.
     * @param session Session to remove.
     * @note Expired weak_ptr entries from other sessions are not pruned during this call.
     */
    auto unregist(const std::shared_ptr<ClientSession>& session) -> void;

    /**
     * @brief Forwards a DATA frame to all sessions registered under @p topic.
     * @param topic Topic to route to.
     * @param payload Event payload bytes.
     */
    auto route(uint16_t topic, const Bytes& payload) -> void;
};

/**
 * @brief TCP event broker that accepts subscriber connections and routes DATA frames.
 *
 * Only one instance may exist at a time. The @c SINGLE_INSTANCE_ONLY macro enforces
 * this at runtime; violation aborts the process. The @c InstanceGuard destructor
 * resets the flag, allowing re-creation after the previous instance is destroyed.
 */
class COMMON_LIB_API AsyncEventBroker : public NonCopyable
{
    SINGLE_INSTANCE_ONLY(AsyncEventBroker)

private :
    std::shared_ptr<AsyncTcpListener> _listener;
    std::shared_ptr<TopicRegistry> _registry = std::make_shared<TopicRegistry>();

public :
    /**
     * @brief Starts accepting connections on the given address and port.
     * @param conn Connection parameters; protocol must be TCP.
     */
    auto run(const communication::Connection& conn) -> void;

    /**
     * @brief Stops the listener and releases its resources.
     * @note No-op if already stopped or never started.
     */
    auto stop() -> void;

private :
    auto onAccept(std::shared_ptr<AsyncTcpSocket> socket) -> void;
};
} // common::asio