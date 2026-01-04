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
#include "common/utils/Misc.hpp"
#include "common/threading/Thread.hpp"
#include "common/container/WorkQueue.hpp"

#include <iostream>
#include <vector>

namespace common::threading
{
/**
 * @class TaskExecutor
 * @brief A thread pool executor that manages multiple worker threads for task execution
 * 
 * TaskExecutor implements a work-stealing thread pool that distributes tasks across
 * multiple worker threads. It uses a round-robin approach for task distribution and
 * employs work-stealing to balance load across threads.
 */
class TaskExecutor final : public NonCopyable,
                           public Factory<TaskExecutor>
{
    friend class Factory<TaskExecutor>;

private :
    std::atomic<bool> _running{false};
    std::atomic<uint32_t> _index{0};
    
    std::vector<std::tuple<std::future<void>, std::shared_ptr<Thread>>> _workers;
    std::vector<std::shared_ptr<WorkQueue>> _queues;

private :
    /**
     * @brief Factory method to create a TaskExecutor instance
     * @param threadCount Number of worker threads to create
     * @return Shared pointer to the created TaskExecutor instance
     */
    static auto __create(uint32_t threadCount) noexcept -> std::shared_ptr<TaskExecutor>
    {
        return std::shared_ptr<TaskExecutor>(new TaskExecutor(threadCount));
    }

public :
    /**
     * @brief Constructor that initializes the thread pool with specified number of threads
     * @param threadCount Number of worker threads to create (will be adjusted to next power of 2)
     * 
     * Creates a thread pool with the specified number of threads. The actual thread count
     * is adjusted to the next power of 2 for efficient bit masking operations.
     * Each thread runs a work-stealing loop that processes tasks from its own queue
     * and steals work from other queues when idle.
     */
    explicit TaskExecutor(uint32_t threadCount) noexcept
    {
        const uint32_t adjustedThreadCount = utils::next_pwr_of_2(threadCount);
        _running.store(true);
        _workers.reserve(adjustedThreadCount);
        _queues.reserve(adjustedThreadCount);

        for(uint32_t i = 0; i < adjustedThreadCount; ++i)
        {
            auto queue = std::make_shared<WorkQueue>();
            _queues.push_back(std::move(queue));
        }

        for(uint32_t i = 0; i < adjustedThreadCount; ++i)
        {
            auto worker = Thread::create();
            auto future = worker->start([index = i, this]() mutable {
                while(_running.load())
                {
                    auto task = _queues[index]->pop(_running);
                    if (task) { task(); }
                    
                    if (_queues[index]->empty())
                    {
                        for (size_t i = 1; i < _queues.size(); ++i) 
                        {
                            if (!_running.load()) { break; }
                            size_t targetIndex = (index + i) % _queues.size();
                            task = _queues[targetIndex]->try_steal();
                            if (task) 
                            {
                                task();
                                break;
                            }
                        }
                    }
                }
            });
            _workers.push_back({std::move(future), std::move(worker)});
        }
    }

    /**
     * @brief Destructor that stops all worker threads
     * 
     * Ensures all worker threads are properly stopped and joined before destruction.
     */
    ~TaskExecutor() noexcept { stop(); }

public :    
    /**
     * @brief Submits a task for execution by the thread pool
     * @tparam ReturnType The return type of the task function
     * @param task The task function to execute
     * @return A future object that can be used to retrieve the task result
     * 
     * Distributes the task to one of the worker threads using round-robin scheduling.
     * The task is added to a work queue and will be executed by an available worker thread.
     */
    template <typename ReturnType>
    auto load(std::function<ReturnType()>&& task) noexcept -> std::future<ReturnType>
    {
        const uint32_t queueIndex = _index.fetch_add(1) & (_queues.size() - 1);
        return _queues[queueIndex]->push(std::move(task));
    }

    /**
     * @brief Stops all worker threads and waits for them to complete
     * 
     * Sets the running flag to false, finalizes all work queues to wake up
     * waiting threads, and waits for all worker threads to complete their
     * current tasks and terminate.
     */
    auto stop() noexcept -> void
    {
        if(!_running.load()) { return; }

        _running.store(false);
        const auto threadCount = _workers.size();
        for(size_t i = 0; i < threadCount; ++i)
        {
            _queues[i]->finalize();
            std::get<0>(_workers[i]).wait();
        }
        _workers.clear();
    }
};
} // namespace common::threading