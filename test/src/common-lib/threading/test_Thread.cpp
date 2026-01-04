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

#include <gtest/gtest.h>

#include "common/threading/Thread.hpp"
#include "common/Exception.hpp"

namespace common::threading::test
{
TEST(test_Thread, create)
{
    // given
    bool value = false;
    auto t = Thread::create();

    // when
    auto future = t->start([&value](){
        value = true;
    });
    t->detach();
    future.wait();

    // then
    ASSERT_TRUE(value);
}

TEST(test_Thread, start_already_running)
{
    // given
    std::atomic<bool> running{true};
    auto t = Thread::create();
    auto future = t->start([&running](){
        while(running.load()) 
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });

    // when & then
    bool catched = false;
    try { t->start([&running](){}); }
    catch(AlreadyRunningException) { catched = true; }
    ASSERT_TRUE(catched);

    running.store(false);
    future.wait();

    catched = false;
    running.store(true);
    try
    {
        auto future_ = t->start([&running](){
        while(running.load()) 
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });

        running.store(false);
        future_.wait();
    }
    catch(AlreadyRunningException) { catched = true; }
    ASSERT_FALSE(catched);
}

TEST(test_Thread, start_already_running_detached)
{
    // given
    std::atomic<bool> running{true};
    auto t = Thread::create();
    auto future = t->start([&running](){
        while(running.load()) 
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });
    t->detach();

    // when & then
    bool catched = false;
    try { t->start([&running](){}); }
    catch(AlreadyRunningException) { catched = true; }
    ASSERT_TRUE(catched);

    running.store(false);
    future.wait();

    catched = false;
    running.store(true);
    try
    {
        auto future_ = t->start([&running](){
        while(running.load()) 
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });
        t->detach();

        running.store(false);
        future_.wait();
    }
    catch(AlreadyRunningException) { catched = true; }
    ASSERT_FALSE(catched);
}

TEST(test_Thread, set_name_priority_before_start)
{
    // given
    std::atomic<bool> running{true};
    auto t = Thread::create();

    // when
    const std::string testThreadName{"unit_test_t"};
    t->set_name(testThreadName);

#if defined(WIN32)
    const Thread::Priority testThreadPrio = Thread::Policies::TIME_CRITICAL;
#elif defined(LINUX)
    const Thread::Priority testThreadPrio = {Thread::Policies::RR, 15};
#endif
    ASSERT_TRUE(t->set_priority(testThreadPrio));

    auto future = t->start([&running](){
        while(running.load()) 
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });

    const auto threadName = t->get_name();
    const auto threadPrio = t->get_priority();

    running.store(false);
    future.wait();

    // then
    ASSERT_TRUE(threadName == testThreadName);
    ASSERT_TRUE(threadPrio == testThreadPrio);
}

TEST(test_Thread, set_name_priority_after_start)
{
    // given
    std::atomic<bool> running{true};
    auto t = Thread::create();

    // when
    const std::string testThreadName{"unit_test_t"};

#if defined(WIN32)
    const Thread::Priority testThreadPrio = Thread::Policies::TIME_CRITICAL;
#elif defined(LINUX)
    const Thread::Priority testThreadPrio = {Thread::Policies::RR, 15};
#endif
    ASSERT_TRUE(t->set_priority(testThreadPrio));

    auto future = t->start([&running](){
        while(running.load()) 
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });

    t->set_name(testThreadName);
    ASSERT_TRUE(t->set_priority(testThreadPrio));

    const auto threadName = t->get_name();
    const auto threadPrio = t->get_priority();

    running.store(false);
    future.wait();

    // then
    ASSERT_TRUE(threadName == testThreadName);
    ASSERT_TRUE(threadPrio == testThreadPrio);
}

TEST(test_Thread, async)
{
    // given
    std::atomic<bool> value{false};

    // when
    auto future = Thread::async([&value](){
        value.store(true); 
    });
    future.wait();

    // then
    ASSERT_TRUE(value.load());
}
} // namespace common::threading::test