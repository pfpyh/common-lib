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
#include "common/Logger.hpp"

#include <functional>

namespace common::communication
{
/// @brief Callback invoked when a DATA frame payload is received.
using onMessage = std::function<void(Bytes)>;

/// @brief Callback invoked once the broker acknowledges the subscription.
using onSubscribed = std::function<void()>;

/**
 * @brief Abstract transport interface for the event pub/sub system.
 */
class EventTransport
{
public :
    virtual ~EventTransport() = default;

    /// @brief Establishes a connection to the broker (publisher path).
    virtual auto connect() -> void = 0;

    /// @brief Returns whether the transport is currently connected.
    virtual auto connected() -> bool = 0;

    /// @brief Closes the connection and releases socket resources.
    virtual auto disconnect() -> void = 0;

    /**
     * @brief Sends a DATA frame to the broker.
     * @param bytes Serialized event payload.
     * @note Default implementation logs an error and aborts in STRICT_MODE.
     */
    virtual auto publish([[maybe_unused]] const Bytes& bytes) -> void 
    {
        LogError << "Default operation: publish";
        if constexpr (STRICT_MODE_ENABLED) { std::abort(); }
    }

    /**
     * @brief Connects to the broker, registers on the topic, and starts the receive loop.
     * @param onMessageHandler Called with raw payload bytes on DATA frame receipt.
     * @param onSubscribedHandler Optional. Called when the broker sends an ACK. Defaults to nullptr.
     * @note Default implementation logs an error and aborts in STRICT_MODE.
     */
    virtual auto subscribe([[maybe_unused]] onMessage onMessageHandler,
                           [[maybe_unused]] onSubscribed onSubscribedHandler = nullptr) -> void
    {
        LogError << "Default operation: subscribe";
        if constexpr (STRICT_MODE_ENABLED) { std::abort(); }
    }
};
} // namespace common::communication