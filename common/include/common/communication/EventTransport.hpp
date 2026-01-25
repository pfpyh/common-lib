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
#include "common/communication/Socket.hpp"
#include "common/Logger.hpp"

#include <vector>
#include <functional>

namespace common::communication
{
using Bytes = std::vector<uint8_t>;
using onMessage = std::function<void(Bytes)>;
using onSubscribed = std::function<void()>;

class EventTransport
{
public :
    virtual ~EventTransport() = default;
    
    virtual auto connect(const Connection& conn) -> void = 0;
    virtual auto connected() -> bool = 0;
    virtual auto disconnect() -> void = 0;
    virtual auto publish([[maybe_unused]] const std::vector<uint8_t> bytes) -> void 
    {
        LogError << "Default operation: publish";
        if constexpr (STRICT_MODE_ENABLED) { std::abort(); }
    }
    virtual auto subscribe([[maybe_unused]] onMessage onMessageHandler,
                           [[maybe_unused]] onSubscribed onSubscribedHandler = nullptr) -> void
    {
        LogError << "Default operation: subscribe";
        if constexpr (STRICT_MODE_ENABLED) { std::abort(); }
    }
};
} // namespace common::communication