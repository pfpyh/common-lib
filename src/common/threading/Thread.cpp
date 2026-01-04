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

#include "common/threading/Thread.hpp"
#include "common/lifecycle/Application.h"
#include "common/lifecycle/Resource.hpp"
#include "common/Logger.hpp"
#include "common/Exception.hpp"

#include <thread>
#include <functional>
#include <memory>

namespace common::threading
{
namespace detail
{
class ThreadDetail final : public Thread
                         , public std::enable_shared_from_this<ThreadDetail>
{
private :
    std::shared_ptr<std::thread> _thread;
    std::shared_ptr<std::promise<void>> _promise;

#if defined(WIN32)
    Priority _priority = Policies::DEFAULT;
#elif defined(LINUX)
    Priority _priority = {Policies::DEFAULT, Level::DEFAULT};
#endif
    std::string _name{lifecycle::get_app_name()};
    uint64_t _tid = 0;
    std::atomic<bool> _started{false};

public :
    ~ThreadDetail() noexcept final = default;

public :
    auto start(std::function<void()>&& func) -> std::future<void> override
    {
        if(_started.load()) 
        {
            LogError << "[" << std::hex << _tid << "][" << _name << "] thread already running";
            throw AlreadyRunningException();
        }

        join();

        _promise = std::make_shared<std::promise<void>>();
        auto future = _promise->get_future();
        auto initPromise = std::make_shared<std::promise<void>>();
        auto initFuture = initPromise->get_future();
        _thread = std::make_shared<std::thread>([this, 
                                                 work = std::move(func), 
                                                 ip = std::move(initPromise)]() {
#if (STRICT_MODE_ENABLED)
            lifecycle::Resource<Thread> resource;
            resource.track(shared_from_this());
#endif
            _started.store(true);
#if defined(WIN32)
            _tid = static_cast<uint64_t>(GetCurrentThreadId());
#elif defined(LINUX)
            _tid = static_cast<uint64_t>(syscall(SYS_gettid));
#endif
            set_priority(_priority);
            set_name(_name);
            ip->set_value();
            work();
            _started.store(false);
#if (STRICT_MODE_ENABLED)
            resource.release();
#endif
            _promise->set_value();
        });
        initFuture.wait();
        return future;
    } 

    auto status() noexcept -> bool override
    {
        return _started.load();
    }

    auto detach() -> void override
    {
        if(!_thread || !_thread->joinable())
        {
            LogError << "[" << std::hex << _tid << "][" << _name << "] can't detach";
            throw BadHandlingException("invalid thread");
        }
        _thread->detach();
    }

    auto join() noexcept -> void override
    {
        if(_thread && _thread->joinable())
            _thread->join();
    }

    auto set_priority(const Priority& priority) -> bool override
    {
        if(_started.load() && !_thread->joinable())
        {
            LogError << "[" << std::hex << _tid << "][" << _name << "] can't set thread priority";
            throw BadHandlingException("invalid thread");
        }
        _priority = priority;
        if(!_started.load()) return true;

#if defined(WIN32)
        if(priority != Policies::DEFAULT)
        {
            HANDLE handle = _thread->native_handle();
            if(false == SetThreadPriority(handle, priority))
            {
                LogWarn << "[" << std::hex << _tid << std::dec << "][" << _name << "] failed to set thread priority (" << GetLastError() << ")";
                return false;
            }
        }
#elif defined(LINUX)
        if(std::get<0>(priority) != Policies::DEFAULT || std::get<1>(priority) != Level::DEFAULT)
        {
            pthread_t handle = _thread->native_handle();
            struct sched_param param;
            param.sched_priority = std::get<1>(priority);
            if(0 != sched_setscheduler(handle, std::get<0>(priority), &param))
            {
                LogWarn << "[" << std::hex << _tid << std::dec << "][" << _name << "] failed to set thread priority (" << errno << ")";
                return false;
            }
        }
#endif
        return true;
    }

    auto get_priority() const noexcept -> Priority override
    {
        return _priority;
    }

    auto set_name(const std::string& name) -> void
    {
        if(_started.load() && !_thread->joinable())
        {
            LogError << "[" << std::hex << _tid << "][" << _name << "] can't set thread name (" << name << ")";
            throw BadHandlingException("invalid thread");
        }
        _name = name;
        if(!_started.load()) return;

#if defined(WIN32)
        HANDLE handle = _thread->native_handle();
        std::wstring wideName(name.begin(), name.end());
        SetThreadDescription(handle, wideName.c_str());
#elif defined(LINUX)
        pthread_t handle = _thread->native_handle();
        pthread_setname_np(handle, name.c_str());
#endif
    }

    auto get_name() const noexcept -> const std::string&
    {
        return _name;
    }

    auto get_tid() const noexcept -> uint64_t override
    {
        return _tid;
    }
};
} // namespace detail

auto Thread::__create() noexcept -> std::shared_ptr<Thread>
{
    auto t = std::shared_ptr<Thread>(new detail::ThreadDetail, [](detail::ThreadDetail* obj){
        obj->join();
        delete obj;
    });

    // lifecycle::ResourceManager::get_instance()->track(std::weak_ptr<Thread>(t));
    return t;
}

auto Thread::async(std::function<void()>&& func) noexcept -> std::future<void>
{
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();
    
    std::thread([promise, work = std::move(func)]() {
        work();
        promise->set_value();
    }).detach();
    
    return future;
}
} // namespace common::threading