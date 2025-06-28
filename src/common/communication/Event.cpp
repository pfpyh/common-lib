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

namespace common
{
EventBus::EventBus(uint32_t threadCount /* = EVENT_THREADS */)
    : _executor(TaskExecutor::create(threadCount)) {}

EventBus::~EventBus() { finalize(); }

auto EventBus::finalize() -> void { _executor->stop(); }

auto EventBus::subscribe(const std::string& topic, HANDLER handler) -> void
{
    std::lock_guard<std::mutex> scopedLock(_lock);
    _handlers[topic].push_back(std::move(handler));
}

auto EventBus::publish(const std::string& topic, const PAYLOAD& payload) -> void
{
    std::lock_guard<std::mutex> scopedLock(_lock);
    auto itor = _handlers.find(topic);
    if(_handlers.end() != itor)
    {
        for(auto& handler : itor->second)
        {
            _executor->load<void>([payload = payload, handler](){
                handler(payload);
            });
        }
    }
}
} // namespace common
