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

#include "common/communication/Event.hpp"
#include "common/Logger.hpp"
#include "common/asio/AsyncEventTransport.hpp"

namespace common::communication
{
template <typename DataType>
EventPublisher<DataType>::EventPublisher(std::shared_ptr<EventTransport>&& transport,
                                         const Connection& conn)
    : _transport(std::move(transport))
    , _conn(conn)
{

}

template <typename DataType>
EventPublisher<DataType>::~EventPublisher()
{
    _transport->disconnect();
}

template <typename DataType>
auto EventPublisher<DataType>::regist() -> void
{
    if(_transport->connected()) 
    { 
        LogDebug << "already registed";
        if constexpr (STRICT_MODE_ENABLED) { std::abort(); }
        return; 
    }
    _transport->connect(_conn);
}

template <typename DataType>
auto EventPublisher<DataType>::unregist() -> void
{
    _transport->disconnect();
}

template <typename DataType>
auto EventPublisher<DataType>::publish(const DataType& data) -> void
{
    Bytes bytes;
    bytes << data;
    _transport->publish(bytes);
}

template <typename DataType>
auto EventPublisher<DataType>::__create(const Connection& conn) noexcept -> std::unique_ptr<EventPublisher>
{
    if(conn._protocol == Protocol::TCP)
    {
        auto transport = std::make_shared<asio::TcpTransport>();
        return std::unique_ptr<EventPublisher>(new EventPublisher(std::move(transport), conn));
    }
    if(conn._protocol == Protocol::UDP)
    {
        LogError << "not implemented for UDP";
        std::abort();
    }
    return nullptr;
}
} // namespace common::communication