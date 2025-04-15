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

#include "common/thread/Timer.hpp"
#include "common/thread/Thread.hpp"
#include "common/Exception.hpp"
#include "common/Singleton.hpp"

#include "common/logging/Logger.hpp"

#include <thread>
#include <exception>
#include <memory>

namespace common
{
namespace detail
{
class TimerDetail final : public Timer
                        , public std::enable_shared_from_this<TimerDetail>{
private :
    Function _func;
    Interval _interval;
    bool _running = false;
    std::shared_ptr<Thread> _thread;

public :
    TimerDetail(Function&& func,
                Interval interval) noexcept
        : _func(func)
        , _interval(interval) {}

    ~TimerDetail() noexcept
    {
        stop();
        if(_thread) { _thread->join(); };
    };

public :
    auto start() -> std::future<void> override
    {
        if(_running) throw AlreadyRunningException(); 
        _running = true;

        _thread = Thread::create();
        auto future = _thread->start([this](){
            while(_running)
            {
                if(false == _func()) { break; }
                std::this_thread::sleep_for(_interval);
            }
        });
        return future;
    }

    auto stop() noexcept -> void override
    {
        _running = false;
    }

    auto running() noexcept -> bool override
    {
        return _running;
    }
};

class TimerHandler : public Singleton<TimerHandler>
{
private :
    std::mutex _lock;
    std::unordered_map<std::shared_ptr<Thread>, std::future<void>> _map;

public :
    auto async(TimerDetail::Function&& func, 
               TimerDetail::Interval interval) noexcept -> std::future<void>
    {
        std::shared_ptr<std::promise<void>> promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();

        auto thread = Thread::create();
        _map.emplace(thread, thread->start([func = std::move(func), 
                                            interval = std::move(interval), 
                                            promise = std::move(promise),
                                            thread,
                                            this](){
            std::this_thread::sleep_for(interval);
            while(true == func()) { std::this_thread::sleep_for(interval); }

            std::lock_guard<std::mutex> lock(_lock);
            if( this->_map.find(thread) != this->_map.end())
            {
                this->_map.erase(thread);
            }
            promise->set_value();
        }));
        thread->detach();
        return future;
    }
};
} // namespace detail


auto Timer::__create(Function&& func, Interval interval) noexcept -> std::unique_ptr<Timer>
{
    return std::make_unique<detail::TimerDetail>(std::forward<Function>(func), 
                                                 interval);
}

auto Timer::async(Function&& func, Interval interval) noexcept -> std::future<void>
{
    return detail::TimerHandler::get_instance()->async(std::move(func), interval);
}
} // namespace common