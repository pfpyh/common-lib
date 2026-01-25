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

#pragma once

#include "common/CommonHeader.hpp"
#include "common/communication/Socket.hpp"

#include <asio/error_code.hpp>

#include <functional>

namespace common::asio
{
/**
 * @class AsyncTcpSocket
 * @brief Asio-based asynchronous TCP socket
 *
 * Created either by an active connect() call or as the result of an accept
 * via AsyncTcpListener::listen(). Internally uses length-prefix framing
 * (4-byte header + payload), so send/receive must only be used between
 * two AsyncTcpSocket instances sharing the same framing protocol.
 *
 * @note IOContext::run() must be called before using this class.
 * @note NonCopyable - pass and store instances via shared_ptr only.
 */
class AsyncTcpSocket : public NonCopyable
                     , public Factory<AsyncTcpSocket>
{
    friend class Factory;

public :
    using onConnect = std::function<void()>;
    using onReceive = std::function<void(const std::vector<uint8_t>&)>;
    using onSend    = std::function<void(size_t bytes)>;
    using onError   = std::function<void(const ::asio::error_code& ec)>;

    virtual ~AsyncTcpSocket() = default;

public :
    /**
     * @brief Initiates an asynchronous connection to the server.
     * @param onConnectHandler Callback invoked on successful connection.
     * @param onErrorHandler   Callback invoked on connection failure (optional).
     *
     * @note If the socket is already connected, the call is silently ignored.
     * @note Must not be called on a socket created by accept; its _conn is empty.
     */
    virtual auto connect(onConnect onConnectHandler, onError onErrorHandler = nullptr) -> void = 0;

    /**
     * @brief Closes the socket connection.
     *
     * Also called automatically from the destructor. Closing the socket causes
     * the peer's receive loop to receive an eof error.
     */
    virtual auto disconnect() -> void = 0;

    /**
     * @brief Starts an asynchronous receive loop.
     * @param onReceiveHandler Callback invoked for each received message with the complete payload.
     * @param onErrorHandler   Callback invoked on receive error, including eof (optional).
     *
     * Reads a 4-byte length-prefix header first, then reads exactly that many bytes,
     * guaranteeing message boundaries. The loop stops on any error.
     *
     * @note Do not call receive more than once on the same socket.
     */
    virtual auto receive(onReceive onReceiveHandler, onError onErrorHandler = nullptr) -> void = 0;

    /**
     * @brief Sends data asynchronously.
     * @param data           Byte array to transmit.
     * @param onSendHandler  Callback invoked on completion; bytes includes the 4-byte header (optional).
     * @param onErrorHandler Callback invoked on send error (optional).
     *
     * Prepends a 4-byte length-prefix header before transmission.
     * data is copied internally, so it is safe to release the buffer immediately after the call.
     */
    virtual auto send(const std::vector<uint8_t>& data, 
                      onSend onSendHandler = nullptr, 
                      onError onErrorHandler = nullptr) -> void = 0;

private :
    static auto __create(const communication::Connection& conn) noexcept -> std::shared_ptr<AsyncTcpSocket>;
};

/**
 * @class AsyncTcpListener
 * @brief Asynchronous TCP connection acceptor
 *
 * Repeatedly accepts incoming client connections on the specified address and port.
 * Each accepted socket is delivered as an AsyncTcpSocket via the onAccept callback.
 *
 * @note IOContext::run() must be called before using this class.
 * @note TCP only. For UDP, use a separate implementation.
 */
class AsyncTcpListener : public NonCopyable
                       , public Factory<AsyncTcpListener>
{
    friend class Factory;

public :
    using onAccept = std::function<void(std::shared_ptr<AsyncTcpSocket>)>;

    virtual ~AsyncTcpListener() = default;

    /**
     * @brief Starts the accept loop on the configured address and port.
     * @param onAcceptHandler Callback invoked for each accepted client socket.
     *
     * The callback is called repeatedly for every new connection.
     * Ownership of the delivered AsyncTcpSocket belongs to the callback caller.
     *
     * @note The loop runs until stop() is called or IOContext is stopped.
     * @note Do not call listen() more than once on the same instance.
     */
    virtual auto listen(onAccept onAcceptHandler) noexcept -> void = 0;

    /**
     * @brief Stops the accept loop.
     *
     * Also called automatically from the destructor.
     * Already accepted sockets are not affected.
     */
    virtual auto stop() noexcept -> void = 0;

private :
    static auto __create(const communication::Connection& conn) noexcept -> std::shared_ptr<AsyncTcpListener>;
};
} // namespace common::asio