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
#include "common/NonCopyable.hpp"
#include "common/Factory.hpp"
#include "common/communication/Socket.hpp"

#include <memory>
#include <shared_mutex>
#include <vector>

namespace common::communication
{
class EventTransport;

template <typename DataType>
class EventPublisher : public NonCopyable
                     , public UniqueFactory<EventPublisher<DataType>>
{
    friend class UniqueFactory<EventPublisher<DataType>>;

private :
    std::shared_ptr<EventTransport> _transport;
    Connection _conn;

private :
    explicit EventPublisher(std::shared_ptr<EventTransport>&& transport,
                            const Connection& conn);

public :
    ~EventPublisher();

public :
    auto regist() -> void;
    auto unregist() -> void;

    auto publish(const DataType& data) -> void;

private :
    static auto __create(const Connection& conn) noexcept -> std::unique_ptr<EventPublisher>;
};

template <typename DataType>
class EventSubscriber : public NonCopyable
                      , public UniqueFactory<EventSubscriber<DataType>>
{
    friend class UniqueFactory<EventSubscriber<DataType>>;

private :
    std::function<void(const DataType&)> _handler;
    std::shared_ptr<EventTransport> _transport;
    Connection _conn;

public :
    explicit EventSubscriber(std::shared_ptr<EventTransport>&& transport,
                             const Connection& conn);
    virtual ~EventSubscriber();

public :
    auto subscribe(std::function<void(const DataType& data)> onEventHandler,
                   std::function<void()> onSubscribedHandler = nullptr) -> void;
    auto unsubscribe() -> void;

private :
    static auto __create(const Connection& conn) noexcept -> std::unique_ptr<EventSubscriber>;
};
} // namespace common::communication

#include "impl/EventPublisher.ipp"
#include "impl/EventSubscriber.ipp"
