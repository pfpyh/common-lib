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
#include <asio/ip/udp.hpp>

#include <functional>

namespace common::asio
{
/**
 * @class AsyncUdpSocket
 * @brief Asio-based asynchronous UDP socket
 *
 * Provides connectionless, datagram-oriented communication over UDP.
 * An internal receive buffer is allocated once at construction time and
 * reused across all incoming datagrams, so there is no per-packet heap
 * allocation on the receive path.
 *
 * Each call to receive() registers a single async_receive_from and
 * automatically re-registers itself after a successful delivery, forming
 * a continuous receive loop until close() is called or a fatal error occurs.
 *
 * @note IOContext::run() must be called before using this class.
 * @note NonCopyable - pass and store instances via shared_ptr only.
 * @note UDP does not guarantee delivery or ordering. The caller is
 *       responsible for any sequencing or reliability requirements.
 * @note If @p datagramMaxSize is smaller than the largest datagram
 *       the peer may send, the excess bytes are silently discarded by
 *       the OS and the received data will be truncated.
 */
class COMMON_LIB_API AsyncUdpSocket : public NonCopyable
                                    , public Factory<AsyncUdpSocket>
{
    friend class Factory<AsyncUdpSocket>;

public :
    using onReceive = std::function<void(const std::vector<uint8_t>&, ::asio::ip::udp::endpoint)>;
    using onSend    = std::function<void(size_t bytes)>;
    using onError   = std::function<void(const ::asio::error_code& ec)>;

    virtual ~AsyncUdpSocket() = default;

public :
    /**
     * @brief Opens and binds the socket to the port specified at construction.
     *
     * Enables SO_REUSEADDR so that the port can be reused immediately after
     * the previous owner releases it.
     *
     * @note Must be called before receive() or send().
     * @note Calling open() more than once on the same socket is undefined
     *       behaviour; close() first if re-binding is required.
     */
    virtual auto open() noexcept -> void = 0;

    /**
     * @brief Closes the socket and cancels all pending async operations.
     *
     * Any in-flight async_receive_from or async_send_to handlers will
     * complete with asio::error::operation_aborted. The receive loop
     * terminates silently on that error.
     *
     * Also called automatically from the destructor.
     */
    virtual auto close() noexcept -> void = 0;

    /**
     * @brief Starts an asynchronous receive loop.
     * @param onReceiveHandler Callback invoked for each received datagram.
     *                         The first argument is a copy of the received
     *                         bytes (sized to the actual datagram length);
     *                         the second is the sender's endpoint.
     * @param onErrorHandler   Callback invoked on I/O errors (optional).
     *                         The loop continues after non-fatal errors.
     *
     * @note Do not call receive() more than once on the same socket;
     *       concurrent loops would race for the shared internal buffer.
     * @note The data vector passed to @p onReceiveHandler is a temporary
     *       copy valid only for the duration of the callback. Do not
     *       capture the const-reference for deferred async use.
     * @note open() must be called before receive().
     */
    virtual auto receive(onReceive onReceiveHandler, onError onErrorHandler = nullptr) noexcept -> void = 0;

    /**
     * @brief Sends a datagram asynchronously to the specified destination.
     * @param dest           Target address and port.
     * @param data           Payload to transmit. Copied internally, so the
     *                       caller may release the buffer immediately.
     * @param onSendHandler  Callback invoked on completion with the number
     *                       of bytes sent (optional).
     * @param onErrorHandler Callback invoked on send error (optional).
     *                       The socket remains usable after a send error.
     *
     * @note UDP datagrams have a maximum payload of 65,507 bytes over IPv4.
     *       Attempting to send larger data will fail with an error.
     * @note open() must be called before send().
     */
    virtual auto send(const communication::Connection& dest,
                      const std::vector<uint8_t>& data,
                      onSend onSendHandler = nullptr,
                      onError onErrorHandler = nullptr) noexcept -> void = 0;

private :
    /**
     * @brief Factory entry point.
     * @param conn            Local bind address and port.
     * @param datagramMaxSize Internal receive buffer size in bytes.
     *                        Defaults to 65,507 (maximum UDP payload over IPv4).
     *                        Set to the largest expected datagram size to
     *                        avoid silent truncation of oversized packets.
     */
    static auto __create(const communication::Connection& conn,
                         size_t datagramMaxSize = 65507) noexcept -> std::shared_ptr<AsyncUdpSocket>;
};
} // namespace common::asio