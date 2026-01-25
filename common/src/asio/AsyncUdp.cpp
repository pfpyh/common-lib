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

#include "common/asio/AsyncUdp.hpp"
#include "common/asio/IOContext.hpp"

#include <asio/ip/udp.hpp>

namespace common::asio
{
namespace detail
{
class AsyncUdpSocketImpl final : public AsyncUdpSocket
{
private :
    communication::Connection _conn;
    ::asio::ip::udp::socket _socket;
    std::vector<uint8_t> _buffer;
    ::asio::ip::udp::endpoint _senderEndpoint;

public :
    explicit AsyncUdpSocketImpl(const communication::Connection& conn,
                                size_t bufferMaxSize) noexcept
        : _conn(conn)
        , _socket(IOContext::get_instance()->get_context())
        , _buffer(bufferMaxSize) {}

    ~AsyncUdpSocketImpl() noexcept { close(); }

public :
    auto open() noexcept -> void override
    {
        ::asio::ip::udp::endpoint local(::asio::ip::udp::v4(), _conn._port);
        _socket.open(local.protocol());
        _socket.set_option(::asio::ip::udp::socket::reuse_address(true));
        _socket.bind(local);
    }

    auto close() noexcept -> void override
    {
        if(_socket.is_open()) { _socket.close(); }
    }

    auto receive(onReceive onReceiveHandler, onError onErrorHandler /*= nullptr*/) noexcept -> void override
    {
        _socket.async_receive_from(::asio::buffer(_buffer), _senderEndpoint,
            [this, onReceive = std::move(onReceiveHandler), onError = std::move(onErrorHandler)]
            (const auto& ec, std::size_t bytes) mutable {
                if(!ec)
                {
                    std::vector<uint8_t> recvBuffer(_buffer.begin(), _buffer.begin() + bytes);
                    onReceive(recvBuffer, _senderEndpoint);
                    receive(std::move(onReceive), std::move(onError));
                }
                else if(ec == ::asio::error::operation_aborted)
                {
                    LogDebug << "Receive loop stopped";
                }
                else
                {
                    LogDebug << "Receive error: " << ec.message();
                    if(onError) { onError(ec); }
                    receive(std::move(onReceive), std::move(onError));
                }
        });
    }

    auto send(const communication::Connection& dest,
              const std::vector<uint8_t>& data,
              onSend onSendHandler /*= nullptr*/,
              onError onErrorHandler /*= nullptr*/ ) noexcept -> void override
    {
        auto buffer = std::make_shared<std::vector<uint8_t>>(data);
        ::asio::ip::udp::endpoint endpoint(::asio::ip::make_address(dest._address), dest._port);
        _socket.async_send_to(::asio::buffer(*buffer), endpoint,
            [buffer, onSend = std::move(onSendHandler), onError = std::move(onErrorHandler)]
            (const auto& ec, std::size_t bytes) {
                if(!ec) { if(onSend) { onSend(bytes); } }
                else
                {
                    LogDebug << "Send error: " << ec.message();
                    if(onError) { onError(ec); }
                }
        });
    }
};
} // namespace detail

auto AsyncUdpSocket::__create(const communication::Connection& conn,
                              size_t datagramMaxSize /*= 65507*/) noexcept -> std::shared_ptr<AsyncUdpSocket>
{
    return std::make_shared<detail::AsyncUdpSocketImpl>(conn, datagramMaxSize);
}
} // namespace common::asio