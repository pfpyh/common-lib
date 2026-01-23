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

#include "common/threading/Timer.hpp"
#include "common/threading/Thread.hpp"
#include "common/Exception.hpp"
#include "common/Singleton.hpp"

#include "common/Logger.hpp"

#include <thread>
#include <exception>
#include <memory>
#include <shared_mutex>

namespace common::threading
{
namespace detail
{
class TimerDetail final : public Timer
                        , public std::enable_shared_from_this<TimerDetail>
{
private :
    Function _func;
    const Interval _interval;
    const bool _async;
    std::atomic<bool> _running{false};
    std::shared_ptr<Thread> _thread;
    std::future<void> _future;
    std::condition_variable _cv;
    std::mutex _lock;

public :
    TimerDetail(Function&& func,
                Interval interval,
                bool async = false) noexcept;
    ~TimerDetail() noexcept;

public :
    auto start() -> std::future<void> override;
    auto stop() noexcept -> void override;
    auto status() noexcept -> bool override;
    auto wait_until_stop() -> void;
};

class TimerManager : public Singleton<TimerManager>
{
private :
    std::shared_mutex _lock;
    std::vector<std::shared_ptr<TimerDetail>> _timers;
    std::atomic<bool> _shutingdown{false};

public :
    auto regist(std::shared_ptr<TimerDetail> timer) -> void
    {
        if(_shutingdown.load()) { return; }

        std::lock_guard<std::shared_mutex> scopedLock(_lock);
        _timers.push_back(timer);
    }

    auto unregist(std::shared_ptr<TimerDetail> timer) -> void
    {
        if (!_lock.try_lock()) { return; }
        std::lock_guard<std::shared_mutex> scopedLock(_lock, std::adopt_lock);
        _timers.erase(std::remove_if(_timers.begin(), _timers.end(),
                                     [&](const std::shared_ptr<TimerDetail>& t) {
                                        return t == timer || !t->status();
        }), _timers.end());
    }

    auto force_stop_all() -> void
    {
        _shutingdown.store(true);

        std::lock_guard<std::shared_mutex> scopedLock(_lock);
        for(auto timer : _timers)
        {
            timer->stop();
            timer->wait_until_stop();
        }

        _timers.clear();
    }
};

TimerDetail::TimerDetail(Function&& func,
                         Interval interval,
                         bool async /* = false */) noexcept
                         : _func(func), _interval(interval), _async(async) {}

TimerDetail::~TimerDetail() noexcept
{
    stop();
    wait_until_stop();
    if(_thread && !_async) { _thread->join(); };
}

auto TimerDetail::start() -> std::future<void>
{
    if(_running.load()) throw AlreadyRunningException(); 
    _running.store(true);

    std::shared_ptr<std::promise<void>> timerPromise = std::make_shared<std::promise<void>>();
    std::future<void> timerFuture = timerPromise->get_future();

    _thread = Thread::create();
    _future = _thread->start([this, timerPromise](){
        {
            std::unique_lock<std::mutex> lock(_lock);
            _cv.wait_for(lock, _interval, [this]() {
                return !_running.load();
            });
        }

        while(_running.load())
        {
            if(false == _func()) { break; }

            std::unique_lock<std::mutex> lock(_lock);
            _cv.wait_for(lock, _interval, [this]() {
                return !_running.load();
            });
        }
        timerPromise->set_value();
        detail::TimerManager::get_instance()->unregist(shared_from_this());
    });
    if(_async) { _thread->detach(); }
    return timerFuture;
}

auto TimerDetail::stop() noexcept -> void
{ 
    _running.store(false);
    _cv.notify_all();
}

auto TimerDetail::status() noexcept -> bool
{ 
    return _running.load(); 
}

auto TimerDetail::wait_until_stop() -> void
{ 
    if(_future.valid()) { _future.wait(); }
}
} // namespace detail

auto Timer::__create(Function&& func, Interval interval) noexcept -> std::unique_ptr<Timer, std::function<void(Timer*)>>
{
    auto sharedTimer = std::make_shared<detail::TimerDetail>(std::forward<Function>(func), 
                                                             interval);
    detail::TimerManager::get_instance()->regist(sharedTimer);
    return std::unique_ptr<Timer, std::function<void(Timer*)>>(sharedTimer.get(), [sharedTimer](Timer*) mutable {
        sharedTimer.reset();
    });
}

auto Timer::async(Function&& func, Interval interval) noexcept -> std::future<void>
{
    auto sharedTimer = std::shared_ptr<detail::TimerDetail>(new detail::TimerDetail(std::forward<Function>(func), 
                                                                                    interval,
                                                                                    true));
    detail::TimerManager::get_instance()->regist(sharedTimer);
    return sharedTimer->start();
}
} // namespace common::threading