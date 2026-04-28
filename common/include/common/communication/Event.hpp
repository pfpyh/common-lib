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

/**
 * @brief Template publisher for a single topic.
 *
 * Use the UniqueFactory interface to construct: @c EventPublisher<T>::create(conn, topic).
 * @c DataType must provide @c operator<<(Bytes&, const DataType&) for serialization.
 *
 * @tparam DataType The event data type to publish.
 */
template <typename DataType>
class EventPublisher : public NonCopyable
                     , public UniqueFactory<EventPublisher<DataType>>
{
    friend class UniqueFactory<EventPublisher<DataType>>;

private :
    std::shared_ptr<EventTransport> _transport;

private :
    explicit EventPublisher(std::shared_ptr<EventTransport>&& transport);

public :
    ~EventPublisher();

public :
    /// @brief Connects to the broker and makes the publisher ready to send.
    /// @note Aborts in STRICT_MODE if already connected.
    auto regist() -> void;

    /// @brief Disconnects from the broker.
    auto unregist() -> void;

    /**
     * @brief Serializes @p data and sends it as a DATA frame.
     * @param data Event data to publish.
     * @note Silently dropped if called before regist(). This is allowed even in STRICT_MODE.
     */
    auto publish(const DataType& data) -> void;

private :
    static auto __create(const Connection& conn, uint16_t topic) noexcept -> std::unique_ptr<EventPublisher>;
};

/**
 * @brief Template subscriber for a single topic.
 *
 * Use the UniqueFactory interface to construct: @c EventSubscriber<T>::create(conn, topic).
 * @c DataType must provide @c operator<<(DataType&, const Bytes&) for deserialization.
 *
 * @tparam DataType The event data type to receive.
 */
template <typename DataType>
class EventSubscriber : public NonCopyable
                      , public UniqueFactory<EventSubscriber<DataType>>
                      , public std::enable_shared_from_this<EventSubscriber<DataType>>
{
    friend class UniqueFactory<EventSubscriber<DataType>>;

private :
    std::function<void(const DataType&)> _handler;
    std::shared_ptr<EventTransport> _transport;

public :
    explicit EventSubscriber(std::shared_ptr<EventTransport>&& transport);
    virtual ~EventSubscriber();

public :
    /**
     * @brief Connects to the broker, registers on the topic, and installs event handlers.
     * @param onEventHandler Invoked on every DATA frame received for the subscribed topic.
     * @param onSubscribedHandler Optional. Invoked once the broker sends an ACK, ensuring
     *                            subsequent publishes will be delivered. Defaults to nullptr.
     * @note Aborts in STRICT_MODE if already connected.
     */
    auto subscribe(std::function<void(const DataType& data)> onEventHandler,
                   std::function<void()> onSubscribedHandler = nullptr) -> void;

    /// @brief Disconnects from the broker and stops receiving events.
    auto unsubscribe() -> void;

private :
    static auto __create(const Connection& conn, uint16_t topic) noexcept -> std::unique_ptr<EventSubscriber>;
};
} // namespace common::communication

#include "impl/EventPublisher.ipp"
#include "impl/EventSubscriber.ipp"
