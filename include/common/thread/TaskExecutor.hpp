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
#include "common/Factory.hpp"
#include "common/thread/Thread.hpp"
#include "common/misc/Utils.hpp"
#include "common/container/WorkQueue.hpp"

#include <chrono>
#include <thread>

#include <string>
#include <iostream>

namespace common
{
class TaskExecutor final : public NonCopyable,
                           public Factory<TaskExecutor>
{
    friend class Factory<TaskExecutor>;

private :
    std::atomic<bool> _running{false};
    std::atomic<uint32_t> _index{0};
    
    std::vector<std::future<void>> _futures;
    std::vector<std::unique_ptr<WorkQueue>> _queues;

private :
    static auto __create(uint32_t threadCount) noexcept -> std::shared_ptr<TaskExecutor>
    {
        return std::shared_ptr<TaskExecutor>(new TaskExecutor(threadCount));
    }

public :
    explicit TaskExecutor(uint32_t threadCount) noexcept
    {
        const uint32_t adjustedThreadCount = utils::next_pwr_of_2(threadCount);
        _running.store(true);
        _futures.reserve(adjustedThreadCount);
        for(uint32_t i = 0; i < adjustedThreadCount; ++i)
        {
            auto queue = std::make_unique<WorkQueue>();
            _queues.push_back(std::move(queue));            auto worker = Thread::create();
            auto future = worker->start([index = i, this]() mutable {
                while(_running.load())
                {
                    auto task = _queues[index]->pop(_running);
                    if (task) { task(); }

                    if (!_running.load() || _queues[index]->empty()) { break; }
                    
                    for (size_t i = 1; i < _queues.size(); ++i) 
                    {
                        size_t targetIndex = (index + i) % _queues.size();
                        task = _queues[targetIndex]->try_steal();
                        if (task) 
                        {
                            task();
                            break;
                        }
                    }
                }
            });
            worker->detach();
            _futures.push_back(std::move(future));
        }
    }

public :
    auto stop() noexcept -> void
    {
        _running.store(false);
        for(auto& queue : _queues) {
            queue->finalize();
        }
        for(auto& future : _futures) {
            future.wait();
        }
    }

    template <typename ReturnType>
    auto load(std::function<ReturnType()> task) noexcept -> std::future<ReturnType>
    {
        const uint32_t queueIndex = _index.fetch_add(1) & (_queues.size() - 1);
        return _queues[queueIndex]->push(std::forward<std::function<ReturnType()>>(task));
    }
};
} // namespace common