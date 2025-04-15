#include "common/thread/TaskExecutor.hpp"

namespace common
{
TaskExecutor::TaskExecutor(const uint32_t threadCount) noexcept
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

TaskExecutor::~TaskExecutor()
{
    if (_running) { stop(false); }
}

auto TaskExecutor::stop(const bool waitUntilDone /*= true*/) noexcept -> void
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
} // namespace common