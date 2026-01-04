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

#include "common/NonCopyable.hpp"
#include "common/Factory.hpp"

#include <future>
#if defined(WIN32)
#include <windows.h>
#elif defined(LINUX)

#endif

namespace common::threading
{
/**
 * @brief Represents a thread that can be executed independently.
 * 
 * This class provides a way to create and manage threads, allowing for concurrent execution of tasks.
 * It offers methods for starting, detaching, joining, and setting the priority of the thread.
 * 
 * @note Detailed implementation can be different based on sysytem.
 */
class COMMON_LIB_API Thread : public NonCopyable
                            , public Factory<Thread>
{
    friend class Factory<Thread>;

public :
#if defined(WIN32)
    class Policies
    {
    public :
        enum type : int32_t
        { 
            DEFAULT = THREAD_PRIORITY_NORMAL,
            ABOVE_NORMAL = THREAD_PRIORITY_ABOVE_NORMAL,
            BELOW_NORMAL = THREAD_PRIORITY_BELOW_NORMAL,
            HIGHEST = THREAD_PRIORITY_HIGHEST,
            IDLE = THREAD_PRIORITY_IDLE,
            LOWEST = THREAD_PRIORITY_LOWEST,
            NORMAL = THREAD_PRIORITY_NORMAL,
            TIME_CRITICAL = THREAD_PRIORITY_TIME_CRITICAL,
        };
    };

    using Priority = Policies::type;
#elif defined(LINUX)    
    class Policies
    {
    public :
        enum type : uint32_t
        {
            DEFAULT = SCHED_OTHER,
            OTHER = SCHED_OTHER,
            FIFO = SCHED_FIFO,
            RR = SCHED_RR,
            BATCH = SCHED_BATCH,
            IDLE = SCHED_IDLE,            
        };
    };

    class Level
    {
    public :
        using type = uint8_t;
        static constexpr type DEFAULT = 0;
    };

    using Priority = std::tuple<Policies::type, Level::type>;
#endif

private :
    /**
     * @brief Creates a new thread object.
     * 
     * @return A shared pointer to the newly created thread object.
     */
    static auto __create() noexcept -> std::shared_ptr<Thread>;

public :
    /**
     * @brief Destructor for the Thread class.
     * 
     * Releases any resources held by the thread object.
     */
    virtual ~Thread() noexcept = default;

public :
    /**
     * @brief Creates a detached thread and executes the given function asynchronously.
     * 
     * @param func The function to be executed in the new thread.
     * @return A future object to synchronize the execution of the function.
     */
    static auto async(std::function<void()>&& func) noexcept -> std::future<void>;

public :
    /**
     * @brief Starts the thread with the given function.
     * 
     * Executes the given function in a separate thread. The thread's priority and name
     * are applied during initialization. If the thread is already running, joins the
     * previous thread before starting a new one.
     * 
     * @param func The function to be executed in the new thread.
     * @return A future object to synchronize with thread completion.
     * 
     * @throws AlreadyRunningException If thread is already running and cannot be restarted.
     */
    virtual auto start(std::function<void()>&& func) -> std::future<void> = 0;

    /**
     * @brief Checks if the thread is currently running.
     * 
     * @return True if the thread is running, false otherwise.
     */
    virtual auto status() noexcept -> bool = 0;

    /**
     * @brief Detaches the thread from the thread object.
     * 
     * Allows the thread to run independently. Once detached, the thread cannot be joined.
     * 
     * @throws BadHandlingException If the thread is invalid or not joinable.
     */
    virtual auto detach() -> void = 0;

    /**
     * @brief Joins the thread with the current thread.
     * 
     * Blocks until the thread completes its execution.
     */
    virtual auto join() noexcept -> void = 0;

    /**
     * @brief Sets the priority of the thread.
     * 
     * Can be called before or after the thread starts. If called before start(),
     * the priority will be applied during thread initialization.
     * 
     * @param priority The new priority of the thread.
     * @return True if priority was successfully set, false if system call failed.
     *         Always returns true when called before thread starts.
     * 
     * @throws BadHandlingException If the thread is started but not joinable (invalid state).
     */
    virtual auto set_priority(const Priority& priority) -> bool = 0;

    /**
     * @brief Gets the current priority of the thread.
     * 
     * @return The current priority of the thread.
     */
    virtual auto get_priority() const noexcept -> Priority = 0;

    /**
     * @brief Sets the name of the thread.
     * 
     * Can be called before or after the thread starts. If called before start(),
     * the name will be applied during thread initialization. Useful for debugging.
     * 
     * @param name The name to assign to the thread.
     * 
     * @throws BadHandlingException If the thread is started but not joinable (invalid state).
     */
    virtual auto set_name(const std::string& name) -> void = 0;

    /**
     * @brief Gets the name of the thread.
     * 
     * @return A constant reference to the thread's name.
     */
    virtual auto get_name() const noexcept -> const std::string& = 0;

    /**
     * @brief Gets the thread ID.
     * 
     * @return The platform-specific thread ID. Returns 0 if thread hasn't started.
     */
    virtual auto get_tid() const noexcept -> uint64_t = 0;
};
} // namespace common::threading