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
#include "common/thread/Thread.hpp"
#include "common/Exception.hpp"
#include "common/NonCopyable.hpp"

#include <vector>

namespace common
{
/**
 * @brief Represents a task that can be executed in a separate thread.
 *
 * This class represents a task that can be executed in a separate thread.
 * It provides a way to create and manage threads, allowing for concurrent execution of tasks.
 *
 * @note A derived class must implement the pure virtual function __work() to execute a task.
 */
class Runnable : public NonCopyable
{
private :
    std::promise<void> _promise;
    bool _running = false;

public :
    /**
     * @brief Start a new thread and call __work() in the thread continuously.
     * 
     * This function can be called only once.
     *
     * @return std::future which is set when thread is finished.
     * @throw common::exception::AlreadyRunningException if run() is called multiple times.
     */
    auto run() -> std::future<void>
    {
        if(_running) throw AlreadyRunningException(); 
        _running = true;

        auto t = Thread::create();
        t->start([this](){            
            while(_running) { __work(); }
            _promise.set_value();
        });
        t->detach();

        return _promise.get_future();
    }

    /**
     * @brief Request to stop the thread started by run().
     * 
     * This function sets a flag to request the running thread to stop. 
     * The thread will not stop immediately but will stop after the current 
     * iteration of work() completes or when it is safe to stop. 
     * 
     * Once stop() is called, status() will return false.
     * If the thread has already been stopped or was never started, this function does nothing.
     */
    inline auto stop() noexcept -> void { _running = false; }

    /**
     * @brief Check if run() is called and the thread is running.
     * 
     * @return true if run() is called and the thread is running, false otherwise.
     */
    inline auto status() const noexcept -> bool { return _running; }

protected :
    /**
     * @brief This function is called by run() in the thread created by run().
     * 
     * This function should be implemented by the derived classes.
     * This function is called continuously until stop() is called or an exception is thrown.
     * If an exception is thrown, run() will catch it and set the future returned by run() to the exception.
     * If stop() is called, this function will return immediately.
     */
    virtual auto __work() -> void = 0;
};

/**
 * @brief Represents a task that can be executed in a separate thread and is cancellable.
 *
 * This class represents a task that can be executed in a separate thread and is cancellable.
 * It provides a way to create and manage threads, allowing for concurrent execution of tasks.
 * The task can be stopped by calling stop(), and the thread started by run() will be stopped.
 * If the task is stopped, status() will return false.
 *
 * @note A derived class must implement the pure virtual function __work() to execute a task.
 */
template <typename DataType>
class ActiveRunnable
{
private :
    std::promise<void> _promise;
    bool _running = false;

    std::mutex _notifyLock;
    std::condition_variable _cv;
    std::vector<DataType> _datas;

public :
    /**
     * @brief Start a new thread and call __work() in the thread continuously with the data that is passed by notify() function.
     * 
     * This function can be called only once.
     *
     * @return std::future which is set when thread is finished.
     * @throw common::exception::AlreadyRunningException if run() is called multiple times.
     */
    auto run() -> std::future<void>
    {
        if(_running) throw AlreadyRunningException();
        _running = true;

        auto t = Thread::create();
        t->start([this](){            
            while(_running)
            {
                std::unique_lock<std::mutex> lock(_notifyLock);
                if(_datas.empty()) _cv.wait(lock);
                if(!_running) break;

                DataType data = _datas[0];
                _datas.erase(_datas.begin());
                lock.unlock();

                __work(std::move(data));
            }
            _promise.set_value();
        });
        t->detach();

        return _promise.get_future();
    }

    /**
     * @brief Notify the thread to execute __work() with the data.
     *
     * @param data Data to be passed to __work()
     */
    auto notify(const DataType& data) noexcept -> void
    {
        std::unique_lock<std::mutex> lock(_notifyLock);
        _datas.push_back(data);
        _cv.notify_one();
    }

    /**
     * @brief Notify the thread to execute __work() with the data.
     *
     * @param data Data to be passed to __work()
     */
    auto notify(DataType&& data) noexcept -> void
    {
        std::unique_lock<std::mutex> lock(_notifyLock);
        _datas.push_back(std::move(data));
        _cv.notify_one();
    }

    /**
     * @brief Request to stop the thread started by run().
     * 
     * This function sets a flag to request the running thread to stop. 
     * The thread will not stop immediately but will stop after the current 
     * iteration of __work() completes or when it is safe to stop. 
     * 
     * Once stop() is called, status() will return false.
     * If the thread has already been stopped or was never started, this function does nothing.
     */
    auto stop() noexcept -> void
    {
        std::unique_lock<std::mutex> lock(_notifyLock);
        _running = false;
        _cv.notify_one();
    }

    /**
     * @brief Check if run() is called and the thread is running.
     * 
     * @return true if run() is called and the thread is running, false otherwise.
     */
    inline auto status() const noexcept -> bool { return _running; }

protected :
    /**
     * @brief This function is called by run() in the thread created by run() with the data that is passed by notify() function.
     * 
     * This function should be implemented by the derived classes.
     * This function is called continuously until stop() is called or an exception is thrown.
     * If an exception is thrown, run() will catch it and set the future returned by run() to the exception.
     * If stop() is called, this function will return immediately.
     *
     * @param data Data to be passed to __work()
     */
    virtual auto __work(DataType&& data) -> void = 0;
};
} // namespace common
