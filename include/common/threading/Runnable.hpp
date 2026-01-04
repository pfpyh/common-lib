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
#include "common/threading/Thread.hpp"
#include "common/Exception.hpp"
#include "common/NonCopyable.hpp"

#include <vector>
#include <atomic>

namespace common::threading
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
    std::shared_ptr<Thread> _t;
    std::atomic<bool> _running{false};

#if defined(WIN32)
    Thread::Priority _priority = Thread::Policies::DEFAULT;
#elif defined(LINUX)
    Thread::Priority _priority = {Thread::Policies::DEFAULT, Thread::Level::DEFAULT};
#endif
    std::string _name;

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
        if(_running.load()) throw AlreadyRunningException(); 
        _running.store(true);

        _t = Thread::create();
        _t->set_priority(_priority);
        _t->set_name(_name);
        return _t->start([this](){            
            while(_running.load()) { __work(); }
        });
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
    inline auto stop() noexcept -> void { _running.store(false); }

    /**
     * @brief Check if run() is called and the thread is running.
     * 
     * @return true if run() is called and the thread is running, false otherwise.
     */
    inline auto status() const noexcept -> bool { return _running.load(); }

    /**
     * @brief Sets the priority of the thread.
     * 
     * @param priority The new priority of the thread.
     * @return True if thread is running and priority was set, false otherwise.
     */
    inline auto set_priority(const Thread::Priority& priority) noexcept -> bool
    {
        _priority = priority;
        if(_running.load()) return _t->set_priority(priority);
        else return false;
    }

    /**
     * @brief Gets the current priority of the thread.
     * 
     * @return The current priority.
     */
    inline auto get_priority() const noexcept -> Thread::Priority
    {
        return _priority;
    }

    /**
     * @brief Sets the name of the thread.
     * 
     * If the thread is running, the name is applied immediately.
     * 
     * @param name The new name of the thread.
     */
    inline auto set_name(const std::string& name) noexcept -> void
    {
        _name = name;
        if(_running.load()) _t->set_name(name);
    }

    /**
     * @brief Gets the current name of the thread.
     * 
     * @return The current name.
     */
    inline auto get_name() const noexcept -> const std::string&
    {
        return _name;
    }

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

namespace base
{
template <typename DataType, typename ReturnType>
class WorkInterface
{
public :
    virtual ~WorkInterface() = default;
    virtual auto __work(DataType&& data) -> ReturnType = 0;
};

template <typename DataType>
class WorkInterface<DataType, void>
{
public :
    virtual ~WorkInterface() = default;
    virtual auto __work(DataType&& data) -> void = 0;
};

template <typename ReturnType>
class WorkInterface<void, ReturnType>
{
public :
    virtual ~WorkInterface() = default;
    virtual auto __work() -> ReturnType = 0;
};

template <>
class WorkInterface<void, void>
{
public :
    virtual ~WorkInterface() = default;
    virtual auto __work() -> void = 0;
};
} // namespace base

/**
 * @brief Represents a task that can be executed in a separate thread with data notification support.
 *
 * This class represents a task that can be executed in a separate thread and is triggered by notifications.
 * It provides a way to create and manage threads, allowing for concurrent execution of tasks with data passing.
 * The thread waits for notify() calls and executes __work() with the provided data.
 * The task can be stopped by calling stop(), and the thread started by run() will be stopped.
 * If the task is stopped, status() will return false.
 *
 * @tparam DataType The type of data to be passed to __work(). Use void for no input data.
 * @tparam ReturnType The return type of __work(). Use void for no return value.
 *
 * @note A derived class must implement the pure virtual function __work() from base::WorkInterface to execute a task.
 */
template <typename DataType, typename ReturnType>
class ActiveRunnable : public base::WorkInterface<DataType, ReturnType>,
                       public NonCopyable
{
private :
    std::shared_ptr<Thread> _t;
    std::atomic<bool> _running{false};

    std::mutex _notifyLock;
    std::condition_variable _cv;

    using PromiseType = std::shared_ptr<std::promise<ReturnType>>;
    using TaskType = std::conditional_t<std::is_void_v<DataType>,
                                        PromiseType,
                                        std::pair<DataType, PromiseType>>;

    std::vector<TaskType> _tasks;

#if defined(WIN32)
    Thread::Priority _priority = Thread::Policies::DEFAULT;
#elif defined(LINUX)
    Thread::Priority _priority = {Thread::Policies::DEFAULT, Thread::Level::DEFAULT};
#endif
    std::string _name;

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
        if(_running.load()) throw AlreadyRunningException();
        _running.store(true);

        _t = Thread::create();
        return _t->start([this](){
            _t->set_priority(_priority);
            _t->set_name(_name);

            while(_running.load())
            {
                std::unique_lock<std::mutex> lock(_notifyLock);
                if(_tasks.empty()) _cv.wait(lock);
                if(!_running.load()) break;

                auto task = _tasks[0];
                _tasks.erase(_tasks.begin());
                lock.unlock();

                if constexpr (std::is_void_v<DataType> && std::is_void_v<ReturnType>)
                {
                    this->__work();
                    task->set_value();
                }
                else if constexpr (std::is_void_v<DataType>)
                {
                    auto promise = task;
                    promise->set_value(this->__work());
                }
                else if constexpr (std::is_void_v<ReturnType>)
                {
                    auto data = std::get<0>(task);
                    auto promise = std::get<1>(task);
                    this->__work(std::move(data));
                    promise->set_value();
                }
                else
                {
                    auto data = std::get<0>(task);
                    auto promise = std::get<1>(task);
                    promise->set_value(this->__work(std::move(data)));
                }
            }
        });
    }

    /**
     * @brief Notify the thread to execute work() with the data.
     *
     * @param data Data to be passed to work()
     */
    template<typename T = DataType>
    auto notify(const T& data) noexcept -> std::enable_if_t<!std::is_void_v<T>, std::future<ReturnType>>
    {
        PromiseType promise = std::make_shared<std::promise<ReturnType>>();
        auto future = promise->get_future();

        std::unique_lock<std::mutex> lock(_notifyLock);
        _tasks.push_back({data, std::move(promise)});
        _cv.notify_one();

        return future;
    }

    /**
     * @brief Notify the thread to execute work() with the data.
     *
     * @param data Data to be passed to work()
     */
    template<typename T = DataType>
    auto notify(const T&& data) noexcept -> std::enable_if_t<!std::is_void_v<T>, std::future<ReturnType>>
    {
        PromiseType promise = std::make_shared<std::promise<ReturnType>>();
        auto future = promise->get_future();

        std::unique_lock<std::mutex> lock(_notifyLock);
        _tasks.push_back({std::move(data), std::move(promise)});
        _cv.notify_one();

        return future;
    }

    /**
     * @brief Notify the thread to execute work() with the data.
     *
     * @param data Data to be passed to work()
     */
    template<typename T = DataType>
    auto notify() noexcept -> std::enable_if_t<std::is_void_v<T>, std::future<ReturnType>>
    {
        PromiseType promise = std::make_shared<std::promise<ReturnType>>();
        auto future = promise->get_future();

        std::unique_lock<std::mutex> lock(_notifyLock);
        _tasks.push_back(std::move(promise));
        _cv.notify_one();

        return future;
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
        _running.store(false);
        _cv.notify_one();
    }

    /**
     * @brief Check if run() is called and the thread is running.
     * 
     * @return true if run() is called and the thread is running, false otherwise.
     */
    inline auto status() const noexcept -> bool { return _running.load(); }

    /**
     * @brief Sets the priority of the thread.
     * 
     * @param priority The new priority of the thread.
     * @return True if thread is running and priority was set, false otherwise.
     */
    inline auto set_priority(const Thread::Priority& priority) noexcept -> bool
    {
        _priority = priority;
        if(_running.load()) return _t->set_priority(priority);
        else return false;
    }

    /**
     * @brief Gets the current priority of the thread.
     * 
     * @return The current priority.
     */
    inline auto get_priority() const noexcept -> Thread::Priority
    {
        return _priority;
    }

    /**
     * @brief Sets the name of the thread.
     * 
     * If the thread is running, the name is applied immediately.
     * 
     * @param name The new name of the thread.
     */
    inline auto set_name(const std::string& name) noexcept -> void
    {
        _name = name;
        if(_running.load()) _t->set_name(name);
    }

    /**
     * @brief Gets the current name of the thread.
     * 
     * @return The current name.
     */
    inline auto get_name() const noexcept -> const std::string&
    {
        return _name;
    }
};
} // namespace common::threading
