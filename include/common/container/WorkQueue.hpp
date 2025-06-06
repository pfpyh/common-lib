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

#include <deque>
#include <memory>
#include <thread>
#include <future>

#include <iostream>

namespace common
{
/**
 * @class WorkQueue
 * @brief A thread-safe work queue for task management in thread pools
 * 
 * WorkQueue provides a thread-safe container for storing and managing tasks
 * that can be executed by worker threads. It supports both FIFO task retrieval
 * and work-stealing from the back of the queue for load balancing.
 */
class WorkQueue
{
private :
    std::deque<std::function<void()>> _tasks;
    mutable std::mutex _lock;
    std::condition_variable _cv;
    
public :          
    /**
     * @brief Adds a task to the work queue
     * @tparam ReturnType The return type of the task function
     * @param task The task function to be executed
     * @return A future object that can be used to retrieve the task result
     * 
     * Wraps the task in a packaged_task and adds it to the queue. The task
     * will be executed asynchronously by a worker thread. Returns a future
     * that can be used to wait for completion and retrieve the result.
     */
    template <typename ReturnType>
    auto push(std::function<ReturnType()>&& task) noexcept -> std::future<ReturnType>
    {
        using TaskType = std::packaged_task<ReturnType()>;
        
        auto packagedTask = std::make_shared<TaskType>(std::move(task));
        std::future<ReturnType> future = packagedTask->get_future();
        {
            std::lock_guard<std::mutex> lock(_lock);
            _tasks.emplace_back([task = std::move(packagedTask)]() { 
                (*task)(); 
            });
        }
        
        _cv.notify_one();
        return future;
    }

    /**
     * @brief Retrieves and removes a task from the front of the queue
     * @param running Reference to atomic boolean indicating if the system is running
     * @return A task function to execute, or nullptr if the queue is empty and system is stopping
     * 
     * Blocks until a task is available or the system stops running. Uses condition
     * variable to efficiently wait for new tasks. Returns the first task in FIFO order.
     */
    auto pop(const std::atomic<bool>& running) -> std::function<void()>
    {
        std::unique_lock<std::mutex> lock(_lock);
        _cv.wait(lock, [this, &running]() {
            return !_tasks.empty() || !running.load();
        });
        
        if(_tasks.empty()) { return nullptr; }
        
        auto task = std::move(_tasks.front());
        _tasks.pop_front();
        return task;
    }

    /**
     * @brief Checks if the work queue is empty
     * @return True if the queue contains no tasks, false otherwise
     * 
     * Thread-safe method to check if the queue is empty. Uses a lock guard
     * to ensure atomic read of the queue state.
     */
    auto empty() const -> bool
    {
        std::lock_guard<std::mutex> lock(_lock);
        return _tasks.empty();
    }

    /**
     * @brief Attempts to steal a task from the back of the queue
     * @return A task function to execute, or nullptr if unsuccessful
     * 
     * Non-blocking operation that attempts to acquire the lock and steal
     * a task from the back of the queue. Used for work-stealing load balancing.
     * Returns nullptr if lock cannot be acquired or queue is empty.
     */
    auto try_steal() -> std::function<void()>
    {
        std::unique_lock<std::mutex> lock(_lock, std::try_to_lock);
        if (!lock.owns_lock() || _tasks.empty()) {
            return nullptr;
        }
        
        auto task = std::move(_tasks.back());
        _tasks.pop_back();
        return task;
    }

    /**
     * @brief Returns the number of tasks currently in the queue
     * @return The size of the task queue
     * 
     * Thread-safe method to get the current number of pending tasks.
     * Uses a lock guard to ensure atomic read of the queue size.
     */
    auto size() const -> size_t
    {
        std::lock_guard<std::mutex> lock(_lock);
        return _tasks.size();
    }
    
    /**
     * @brief Notifies all waiting threads to wake up
     * 
     * Used during shutdown to wake up all threads that are waiting
     * on the condition variable. This allows threads to check the
     * running state and exit gracefully.
     */
    auto finalize() -> void
    {
        _cv.notify_all();
    }
};
} // namespace common