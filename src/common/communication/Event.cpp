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

#include "common/communication/Event.hpp"

#include <algorithm>

namespace common::communication
{
auto generate_subId() -> uint32_t
{
    static std::atomic<uint32_t> nextSubId{0};
    return nextSubId.fetch_add(1);
}

EventBus::EventBus(uint32_t threadCount /* = EVENT_THREADS */)
    : _executor(TaskExecutor::create(threadCount)) {}

EventBus::~EventBus() { finalize(); }

auto EventBus::finalize() -> void { _executor->stop(); }

auto EventBus::subscribe(const std::string& topic, Handler handler) -> SubID
{
    const SubID subId = generate_subId();
    auto subscriber = std::make_shared<HandlerInfo>(subId, handler);

    {
        std::lock_guard<std::mutex> lock(_subscriptionLock);
        _subscriptions[subId] = subscriber;
    }

    // Copy-on-Write pattern
    {
        std::unique_lock<std::shared_mutex> lock(_topicLock);
        
        auto& topicData = _topics[topic];
        if(!topicData)
        {
            topicData = std::make_shared<TopicData>();
        }

        auto newData = std::make_shared<TopicData>(*topicData);
        newData->push_back(subscriber);

        _topics[topic] = newData; // atomical swap
    }

    return subId;
}

auto EventBus::unsubscribe(SubID subId) -> void
{
    std::shared_ptr<HandlerInfo> handlerInfo;

    {
        std::lock_guard<std::mutex> lock(_subscriptionLock);
        auto itor = _subscriptions.find(subId);
        if(itor != _subscriptions.end())
        {
            handlerInfo = itor->second.lock();
            _subscriptions.erase(itor);
        }
    }

    if(!handlerInfo) { return; }

    handlerInfo->_active.store(false);
    if(_cleanupCount.fetch_add(1) % 10 == 0) 
    {
        cleanup_unsubscribers();
        _cleanupCount.store(0);
    }
}

auto EventBus::cleanup_unsubscribers() -> void
{
    _executor->load<void>([this]() {
        std::unique_lock<std::shared_mutex> lock(_topicLock);

        for(auto & [topic, topicData] : _topics) 
        {
            auto newTopicData = std::make_shared<TopicData>();
            std::copy_if(topicData->begin(), topicData->end(), 
                         std::back_inserter(*newTopicData),
                         [](const auto& handlerInfo) {
                            return handlerInfo->_active.load();
            });
            _topics[topic] = newTopicData;
        }
    });
}

auto EventBus::publish(const std::string& topic, const Payload& payload) -> void
{
    std::shared_ptr<TopicData> topicDataSnapshot;
    {
        std::shared_lock<std::shared_mutex> lock(_topicLock);
        auto itor = _topics.find(topic);
        if(itor != _topics.end())
        {
            topicDataSnapshot = itor->second;
        }
    }

    if(!topicDataSnapshot) { return; }

    for(const auto& handler : (*topicDataSnapshot))
    {
        if(!handler->_active.load()) { continue; }
        _executor->load<void>([payload, handler](){
            if(!handler->_active.load()) { return; }
            handler->_handler(payload);
        });
    }
}
} // namespace common::communication
