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

#include "common/CommonHeader.hpp"
#include "common/Singleton.hpp"
#include "common/Logger.hpp"
#include "common/threading/TaskExecutor.hpp"

#include <asio/io_context.hpp>
#include <asio/executor_work_guard.hpp>
#include <asio/post.hpp>
#include <asio/dispatch.hpp>

namespace common::asio
{
/**
 * @class IOContext
 * @brief Singleton wrapper around asio::io_context that manages the asynchronous event loop
 *
 * Owns a thread pool (TaskExecutor) that drives io_context::run() across multiple threads.
 * All asio-based components (AsyncTcpSocket, AsyncTcpListener, etc.) share this single instance.
 *
 * Typical lifecycle:
 * @code
 *   IOContext::get_instance()->run();   // start at application startup
 *   // ... use AsyncTcpSocket / AsyncTcpListener ...
 *   IOContext::get_instance()->stop();  // stop at application shutdown
 * @endcode
 *
 * @note run() must be called before any asynchronous operation is initiated.
 * @note This class is not copyable or movable (LazySingleton + SINGLE_INSTANCE_ONLY).
 */
class IOContext : public LazySingleton<IOContext>
{
    SINGLE_INSTANCE_ONLY(IOContext)
    
private :
    ::asio::io_context _context;
    ::asio::executor_work_guard<::asio::io_context::executor_type> _workGuard;

    std::shared_ptr<threading::TaskExecutor> _executor;
    static constexpr size_t _executorCount = 4;
    std::atomic<bool> _running{false};

public :
    IOContext() noexcept
        : _workGuard(::asio::make_work_guard(_context))
        , _executor(threading::TaskExecutor::create(_executorCount)) {}

public :
    /**
     * @brief Returns the underlying asio::io_context.
     *
     * Intended for internal use by asio socket/acceptor constructors.
     * Prefer using post() or dispatch() for submitting work externally.
     */
    inline auto get_context() noexcept -> ::asio::io_context& { return _context; }

    /**
     * @brief Starts the event loop by running io_context on the internal thread pool.
     *
     * Spawns _executorCount threads, each calling io_context::run().
     *
     * @note Calling run() while already running logs an error and returns immediately.
     *       If STRICT_MODE_ENABLED, it aborts instead.
     */
    auto run() -> void
    {
        if(_running.load())
        {
            LogError << "IOContext already running";
            if constexpr (STRICT_MODE_ENABLED) { std::abort(); }
            return;
        }

        _running.store(true);
        for(size_t i = 0; i < _executorCount; ++i)
        {
            _executor->load<void>([this]() {
                _context.run();
            });
        }
    }

    /**
     * @brief Stops the event loop and joins all worker threads.
     *
     * Resets the work guard to allow io_context to drain, then calls
     * io_context::stop() and waits for the thread pool to finish.
     *
     * @note All pending asynchronous operations will be cancelled.
     *       Calling stop() while not running has no effect.
     */
    auto stop() -> void
    {
        if(!_running.load()) { return; }

        _workGuard.reset();
        _context.stop();
        _executor->stop();
        _running.store(false);
    }

    /**
     * @brief Posts a handler to be executed on the io_context thread pool.
     * @param handler Callable to execute. Execution is deferred and non-blocking.
     *
     * Guarantees the handler runs on one of the io_context threads,
     * even if called from a thread that is already inside the event loop.
     */
    template<typename Handler>
    auto post(Handler&& handler) -> void
    {
        ::asio::post(_context, std::forward<Handler>(handler));
    }

    /**
     * @brief Dispatches a handler to the io_context thread pool.
     * @param handler Callable to execute.
     *
     * If called from within the io_context thread, the handler is executed immediately.
     * Otherwise, behaves the same as post().
     */
    template<typename Handler>
    auto dispatch(Handler&& handler) -> void
    {
        ::asio::dispatch(_context, std::forward<Handler>(handler));
    }
};
} // namespace common::asio