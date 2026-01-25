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
#include "common/communication/EventTransport.hpp"
#include "common/asio/AsyncTcp.hpp"
#include "common/asio/IOContext.hpp"

#include <asio/strand.hpp>
#include <asio/post.hpp>

#include <shared_mutex>
#include <mutex>
#include <atomic>

namespace common::asio
{
/**
 * @brief Asio-based TCP implementation of EventTransport.
 *
 * All socket operations are dispatched through an @c asio::strand to prevent data races.
 * The destructor is intentionally empty; socket cleanup is managed exclusively by
 * strand-posted tasks via disconnect().
 */
class COMMON_LIB_API TcpTransport : public communication::EventTransport
                                  , public std::enable_shared_from_this<TcpTransport>
{
private :
    const communication::Connection _conn;
    const uint16_t _topic;

    std::shared_ptr<AsyncTcpSocket> _socket;
    std::atomic<bool> _connected{false};
    ::asio::strand<::asio::io_context::executor_type> _strand;

public :
    explicit TcpTransport(const communication::Connection& conn, uint16_t topic)
        : _conn(conn)
        , _topic(topic)
        , _strand(::asio::make_strand(IOContext::get_instance()->get_context())) {}

    ~TcpTransport() {}

    /**
     * @brief Opens a socket and asynchronously connects to the broker.
     * @note Aborts in STRICT_MODE if socket already exists (already connected).
     */
    auto connect() -> void override;

    /// @brief Returns whether the transport is currently connected.
    auto connected() -> bool override;

    /**
     * @brief Posts a disconnect task to the strand.
     * @note Non-blocking. The socket is closed and reset inside the strand executor.
     */
    auto disconnect() -> void override;

    /**
     * @brief Posts a DATA frame send to the strand.
     * @param payload Serialized event payload bytes.
     * @note Silently dropped if @c _connected is false when the strand task runs.
     */
    auto publish(const Bytes& payload) -> void override;

    /**
     * @brief Opens a socket, connects to the broker, sends REGIST, and starts the receive loop.
     *
     * On ACK receipt, @p onSubscribedHandler is called. On DATA receipt, @p onMessageHandler
     * is called. Both callbacks are invoked in an IOContext worker thread.
     *
     * @param onMessageHandler Called with raw payload bytes on DATA frame receipt.
     * @param onSubscribedHandler Called once the broker acknowledges the subscription (ACK).
     * @note Aborts in STRICT_MODE if socket already exists (already subscribed).
     */
    auto subscribe(communication::onMessage onMessageHandler, 
                   communication::onSubscribed onSubscribedHandler) -> void override;
};
} // common::asio