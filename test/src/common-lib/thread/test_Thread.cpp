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

#include "common/thread/Thread.hpp"

namespace common::test
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

TEST(test_Thread, priority)
{
    // given
    std::promise<void> promise;
    auto future = promise.get_future();
    auto t = Thread::create();

#if defined(WIN32)
    const Thread::Priority origin = Thread::Policies::DEFAULT;
    const Thread::Priority setting = Thread::Policies::TIME_CRITICAL;
#elif defined(LINUX)
    const Thread::Priority origin = {Thread::Policies::DEFAULT, Thread::Level::DEFAULT};
    const Thread::Priority setting = {Thread::Policies::RR, 15};
#endif

    // when
    auto threadFuture = t->start([&future](){
        future.wait();
    });

    auto before = t->get_priority();
    const bool rtn = t->set_priority(setting);
    auto after = t->get_priority();

    // then
    ASSERT_TRUE(rtn);
    EXPECT_EQ(before, origin);
    EXPECT_EQ(after, setting);

    promise.set_value();   
    threadFuture.wait();
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
} // namespace common::test