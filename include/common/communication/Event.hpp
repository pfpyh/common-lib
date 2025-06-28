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
#include "common/thread/TaskExecutor.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <stdint.h>

namespace common
{
struct Event
{
    std::string _topic;
    std::vector<uint8_t> _payload;
};

class COMMON_LIB_API EventBus final : public NonCopyable
{
    using TOPIC = std::string;
    using PAYLOAD = std::vector<uint8_t>;
    using HANDLER = std::function<void(const PAYLOAD&)>;

private :
    std::shared_ptr<common::TaskExecutor> _executor;
    std::unordered_map<std::string, std::vector<HANDLER>> _handlers;
    std::mutex _lock;

public :
    explicit EventBus(uint32_t threadCount = EVENT_THREADS);
    ~EventBus();

public :
    auto finalize() -> void;

    auto subscribe(const std::string& topic, HANDLER handler) -> void;

    auto publish(const std::string& topic, const PAYLOAD& payload) -> void;
};
} // namespace common
