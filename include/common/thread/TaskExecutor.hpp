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

#include <stdint.h>
#include <future>
#include <functional>
#include <atomic>
#include <tuple>
#include <vector>
#include <queue>

namespace common
{
class COMMON_LIB_API TaskExecutor final : public NonCopyable, 
                                          public UniqueFactory<TaskExecutor>
{
    friend class UniqueFactory<TaskExecutor>;

private :
    std::mutex _lock;
    std::condition_variable _cv;
    std::atomic<bool> _running = true;

    std::queue<std::function<void()>> _tasks;
    std::vector<std::tuple<std::shared_ptr<Thread>, std::future<void>>> _workers;

private :
    explicit TaskExecutor(const uint32_t threadCount) noexcept
    {
        _workers.reserve(threadCount);
        for(uint32_t i = 0; i < threadCount; ++i)
        {
            auto worker = Thread::create();
            auto future = worker->start([this]() mutable
            {
                while (true)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(_lock);
                        _cv.wait(lock, [this]() { return !_running || !_tasks.empty(); });

                        if (!_running && _tasks.empty()) { return; }
                        
                        task = std::move(_tasks.front());
                        _tasks.pop();
                    }
                    task();
                }
            });
            _workers.push_back({worker, std::move(future)});
        }
    }

    static auto __create(const uint32_t threadCount) noexcept -> std::unique_ptr<TaskExecutor>
    {
        return std::unique_ptr<TaskExecutor>(new TaskExecutor(threadCount));
    }

public :
    ~TaskExecutor()
    {
        if(_running) { stop(false); }
    }

public :
    auto stop(const bool waitUntilDone = true) noexcept -> void
    {
        {
            std::lock_guard<std::mutex> lock(_lock);
            _running = false;
        }

        if (waitUntilDone)
        {
            for (;;)
            {
                std::unique_lock<std::mutex> lock(_lock);
                if (_tasks.empty()) { break; }
                _cv.wait(lock);
            }
        }

        _cv.notify_all();
        for(auto& worker : _workers)
        {
            auto& t = std::get<0>(worker);
            auto& f = std::get<1>(worker);

            t->join();
            f.wait();
        }
    }

    template <typename ReturnType>
    auto load(std::function<ReturnType()> task) noexcept -> std::future<ReturnType>
    {
        using TaskType = std::packaged_task<ReturnType()>;
        auto packagedTask = std::make_shared<TaskType>(std::move(task));

        std::future<ReturnType> future = packagedTask->get_future();
        {
            std::lock_guard<std::mutex> lock(_lock);
            _tasks.emplace([packagedTask]() mutable { (*packagedTask)(); });
        }

        _cv.notify_one();
        return future;
    }
};
} // namespace common