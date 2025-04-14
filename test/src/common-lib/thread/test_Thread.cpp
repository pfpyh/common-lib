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
    bool value = false;

    // when
    auto future = Thread::async([&value](){
        value = true; 
    });
    future.wait();

    // then
    ASSERT_TRUE(value);
}
} // namespace common::test