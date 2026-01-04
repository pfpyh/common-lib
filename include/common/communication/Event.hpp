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

#include "CommonHeader.hpp"

#include "common/NonCopyable.hpp"
#include "common/threading/TaskExecutor.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <stdint.h>
#include <atomic>
#include <shared_mutex>

namespace common::communication
{
class COMMON_LIB_API EventBus final : public NonCopyable
{
public :
    using Topic = std::string;
    using Payload = std::vector<uint8_t>;
    using Handler = std::function<void(const Payload&)>;
    using SubID = uint32_t;

    struct HandlerInfo
    {
        SubID _subId;
        Handler _handler;
        std::atomic<bool> _active{true};

        HandlerInfo(SubID subId, Handler handler)
            : _subId(subId), _handler(handler) {}
    };

    using TopicData = std::vector<std::shared_ptr<HandlerInfo>>;

private :
    std::shared_ptr<threading::TaskExecutor> _executor;
    std::vector<std::shared_ptr<HandlerInfo>> _handlers;

    std::shared_mutex _topicLock;
    std::unordered_map<Topic, std::shared_ptr<TopicData>> _topics;

    std::mutex _subscriptionLock;
    std::unordered_map<SubID, std::weak_ptr<HandlerInfo>> _subscriptions;
    std::atomic<uint8_t> _cleanupCount{0};

public :
    explicit EventBus(uint32_t threadCount = EVENT_THREADS);
    ~EventBus();

public :
    auto finalize() -> void;

    auto subscribe(const std::string& topic, Handler handler) -> SubID;
    auto unsubscribe(SubID subId) -> void;

    auto publish(const std::string& topic, const Payload& payload) -> void;

private :
    auto cleanup_unsubscribers() -> void;
};
} // namespace common::communication
