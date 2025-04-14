#include "common/thread/Thread.hpp"
#include "common/logging/Logger.hpp"

#include <thread>
#include <functional>

namespace common
{
namespace detail
{
class ThreadDetail final : public Thread
{
private :
    std::thread _thread;
    std::promise<void> _promise;
#if defined(WIN32)
    Priority _priority = Policies::DEFAULT;
#elif defined(LINUX)
    Priority _priority = {Policies::DEFAULT, Level::DEFAULT};
#endif

public :
    ~ThreadDetail() noexcept final = default;

public :
    auto start(std::function<void()>&& func) noexcept -> std::future<void> override
    {
        auto future = _promise.get_future();
        _thread = std::thread([this, work = std::move(func)]() mutable 
        {
            set_priority(_priority);
            work();
            _promise.set_value();
        });
        return future;
    } 

    auto detach() noexcept -> void override
    {
        _thread.detach();
    }

    auto join() noexcept -> void override
    {
        if(_thread.joinable())
        {
            _thread.join();
        }
    }

    auto set_priority(const Priority& priority) noexcept -> bool override
    {
        if(_thread.joinable() == false) // Thread is not started
        {
            _priority = priority;
            return true;
        }
#if defined(WIN32)
        else if(priority != Policies::DEFAULT)
        {
            HANDLE handle = _thread.native_handle();
            if(false == SetThreadPriority(handle, priority))
            {
                _ERROR_("set_priority = (%d)", GetLastError());
                return false;
            }
            _priority = priority;
        }
#elif defined(LINUX)
        else if(std::get<0>(priority) != Policies::DEFAULT ||
                std::get<1>(priority) != Level::DEFAULT)
        {
            pthread_t handle = _thread.native_handle();
            struct sched_param param;
            param.sched_priority = std::get<1>(priority);
            if(false == sched_setscheduler(handle, std::get<0>(priority), &param))
            {
                _ERROR_("set_priority = (%d)", errno);
                return false;
            }
            _priority = priority;            
        }
#endif
        return true;
    }

    auto get_priority() noexcept -> Priority override
    {
        return _priority;
    }
};
} // namespace detail

auto Thread::__create() noexcept -> std::shared_ptr<Thread>
{
    return std::shared_ptr<Thread>(new detail::ThreadDetail, [](detail::ThreadDetail* obj){
        obj->join();
    });
}

auto Thread::async(std::function<void()>&& func) noexcept -> std::future<void>
{
    auto t = __create();
    auto future = t->start(std::move(func));
    t->detach();
    return future;
}
} // namespace common