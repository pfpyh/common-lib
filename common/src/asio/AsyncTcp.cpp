#include "common/asio/AsyncTcp.hpp"
#include "common/asio/IOContext.hpp"

#include <asio/ip/tcp.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>
#include <asio/connect.hpp>

#include <atomic>
#include <cstring>

namespace common::asio
{
namespace detail
{
class AsyncSocketImpl final : public AsyncTcpSocket
{
private :
    communication::Connection _conn;
    std::atomic<bool> _connected{false};
    ::asio::ip::tcp::socket _socket;

public :
    AsyncSocketImpl(const communication::Connection& conn) noexcept
        : _conn(conn), _socket(IOContext::get_instance()->get_context()) {}

    explicit AsyncSocketImpl(::asio::ip::tcp::socket socket) noexcept 
        : _socket(std::move(socket)) { _connected.store(true); }

    ~AsyncSocketImpl() noexcept
    {
        disconnect();
    }

public :
    auto connect(onConnect onConnectHandler, onError onErrorHandler /*= nullptr*/) -> void override
    {
        if(_connected.load()) 
        { 
            LogDebug << "AsyncSocket already connected";
            return; 
        }

        ::asio::ip::tcp::resolver resolver(IOContext::get_instance()->get_context());
        auto endpoint = resolver.resolve(_conn._address, std::to_string(_conn._port));

        ::asio::async_connect(_socket, endpoint,
                              [this, onConnect = std::move(onConnectHandler), onError = std::move(onErrorHandler)]
                              (const auto& ec, [[maybe_unused]] const auto& endpoint) {
            if(!ec) 
            { 
                _connected.store(true);
                onConnect(); 
            }
            else 
            { 
                LogDebug << "Connect error: " << ec.message(); 
                if(onError) { onError(ec); }
            }
        });
    }

    auto disconnect() -> void override
    {
        if(_socket.is_open()) { _socket.close(); }
        _connected.store(false);
    }

    auto receive(onReceive onReceiveHandler, onError onErrorHandler /*= nullptr*/) -> void override
    {
        auto header = std::make_shared<std::array<uint8_t, 4>>();
        ::asio::async_read(_socket, ::asio::buffer(*header),
                           [this, header, onReceive = std::move(onReceiveHandler), onError = std::move(onErrorHandler)]
                           (const auto& ec, std::size_t) mutable 
            {
                if(ec)
                {
                    if(ec == ::asio::error::eof)
                    {
                        LogDebug << "Connection closed by peer";
                        disconnect();
                    }
                    else { LogDebug << "Receive error: " << ec.message(); }
                    if(onError) { onError(ec); }
                    return;
                }

                uint32_t payloadSize = 0;
                std::memcpy(&payloadSize, header->data(), sizeof(payloadSize));

                auto payload = std::make_shared<std::vector<uint8_t>>(payloadSize);
                ::asio::async_read(_socket, ::asio::buffer(*payload),
                                   [this, payload, onReceive, onError]
                                   (const auto& ec, std::size_t) mutable {
                    if(!ec)
                    {
                        onReceive(*payload);
                        receive(std::move(onReceive), std::move(onError));
                    }
                    else
                    {
                        LogDebug << "Receive payload error: " << ec.message();
                        if(onError) { onError(ec); }
                    }
                });
            }
        );
    }

    auto send(const std::vector<uint8_t>& data, onSend onSendHandler /*= nullptr*/, onError onErrorHandler /*= nullptr*/) -> void override
    {
        auto buffer = std::make_shared<std::vector<uint8_t>>(4 + data.size());
        const uint32_t payloadSize = static_cast<uint32_t>(data.size());
        std::memcpy(buffer->data(), &payloadSize, sizeof(payloadSize));
        std::memcpy(buffer->data() + 4, data.data(), data.size());

        ::asio::async_write(_socket, ::asio::buffer(*buffer),
                            [buffer, onSend = std::move(onSendHandler), onError = std::move(onErrorHandler)]
                            (const auto& ec, std::size_t bytes) {
            if(!ec)
            {
                if(onSend) { onSend(bytes); }
            }
            else
            {
                LogDebug << "Send error: " << ec.message();
                if(onError) { onError(ec); }
            }
        });
    }
};

class AsyncListenerImpl final : public AsyncTcpListener
{
private :
    const communication::Connection _conn;
    std::unique_ptr<::asio::ip::tcp::acceptor> _acceptor;

public :
    explicit AsyncListenerImpl(const communication::Connection& conn) noexcept : _conn(conn) {}
    ~AsyncListenerImpl() noexcept { stop(); }

public :
    auto listen(onAccept onAcceptHandler) noexcept -> void override
    {
        auto address = ::asio::ip::make_address(_conn._address);
        auto endpoint = ::asio::ip::tcp::endpoint(address, _conn._port);

        _acceptor = std::make_unique<::asio::ip::tcp::acceptor>(
            IOContext::get_instance()->get_context(), 
            endpoint);
        _acceptor->set_option(::asio::ip::tcp::acceptor::reuse_address(true));
        listening(std::move(onAcceptHandler));
    }

    auto stop() noexcept -> void override
    {
        if(_acceptor && _acceptor->is_open()) { _acceptor->close(); }
    }

private :
    auto listening(onAccept&& onAcceptHandler) noexcept -> void
    {
        _acceptor->async_accept([this, handler = std::move(onAcceptHandler)](const auto& ec, auto endPoint) mutable {
            if(!ec)
            {
                auto client = std::make_shared<AsyncSocketImpl>(std::move(endPoint));
                handler(client);
                listening(std::move(handler));
            }
            else if(ec == ::asio::error::operation_aborted)
            {
                if(_acceptor && _acceptor->is_open())
                    LogDebug << "Accept loop stopped: io_context stopped";
                else
                    LogDebug << "Accept loop stopped: acceptor closed";
            }
            else
            {
                LogDebug << "Accept error:" << ec.message();
                if(_acceptor && _acceptor->is_open())
                    listening(std::move(handler));
            }
        });
    }
};
} // namespace detail

auto AsyncTcpSocket::__create(const communication::Connection& conn) noexcept -> std::shared_ptr<AsyncTcpSocket>
{
    return std::make_shared<detail::AsyncSocketImpl>(conn);
}

auto AsyncTcpListener::__create(const communication::Connection& conn) noexcept -> std::shared_ptr<AsyncTcpListener>
{
    return std::make_shared<detail::AsyncListenerImpl>(conn);
}
} // namespace common::asio