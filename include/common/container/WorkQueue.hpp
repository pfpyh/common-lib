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

namespace common
{
class WorkQueue
{
private :
    std::deque<std::function<void()>> _tasks;
    mutable std::mutex _lock;
    std::condition_variable _cv;
    
public :        
    template <typename ReturnType>
    auto push(std::function<ReturnType()> task) noexcept -> std::future<ReturnType>
    {
        using TaskType = std::packaged_task<ReturnType()>;
        
        auto packagedTask = std::make_shared<TaskType>(std::move(task));
        std::future<ReturnType> future = packagedTask->get_future();
        {
            std::lock_guard<std::mutex> lock(_lock);
            _tasks.emplace_back([task = std::move(packagedTask)]() mutable { 
                (*task)(); 
            });
        }
        
        _cv.notify_one();
        return future;
    }

    auto pop(std::atomic<bool>& running) -> std::function<void()>
    {
        std::unique_lock<std::mutex> lock(_lock);
        _cv.wait(lock, [this, &running]() {
            return !_tasks.empty() || !running.load();
        });
        
        if (!running.load() || _tasks.empty()) { return nullptr; }
        
        auto task = std::move(_tasks.front());
        _tasks.pop_front();
        return task;
    }

    auto empty() const -> bool
    {
        std::lock_guard<std::mutex> lock(_lock);
        return _tasks.empty();
    }

    auto try_steal() -> std::function<void()>
    {
        std::unique_lock<std::mutex> lock(_lock, std::defer_lock);
        if (!lock.try_lock() || _tasks.empty()) {
            return nullptr;
        }
        
        auto task = std::move(_tasks.back());
        _tasks.pop_back();
        return task;
    }

    auto size() const -> size_t
    {
        std::lock_guard<std::mutex> lock(_lock);
        return _tasks.size();
    }
    
    void finalize()
    {
        _cv.notify_all();
    }
};
} // namespace common