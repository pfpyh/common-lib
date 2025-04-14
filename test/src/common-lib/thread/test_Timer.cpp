#include <gtest/gtest.h>

#include "common/thread/Timer.hpp"
#include "common/logging/Logger.hpp"

#include <thread>

namespace common::test
{
TEST(test_Timer, start)
{
    // given
    uint8_t count = 0;
    auto timer = Timer::create([&count]() -> bool{
        ++count;
        if (count == 10) return false;
        return true;
    }, std::chrono::milliseconds(10));

    // when
    auto future = timer->start();
    future.wait();

    // then
    ASSERT_EQ(count, 10);
}

TEST(test_Timer, stop)
{
    // given
    uint8_t count = 0;
    auto timer = Timer::create([&count]() -> bool{
        ++count;
        if (count == 255) return false;
        return true;
    }, std::chrono::milliseconds(10));

    // when
    auto future = timer->start();
    timer->stop();
    future.wait();

    // then
    ASSERT_NE(count, 255);
}

TEST(test_Timer, async_start)
{

    // given
    std::atomic<bool> isRunning = true;
    bool result = false;
    std::shared_ptr<std::future<void>> future;
    {
        future = std::make_shared<std::future<void>>(Timer::async([&isRunning, &result]() -> bool{
            if(!isRunning.load()) 
            {
                result = true;
                return false;
            }
            return true;
        }, std::chrono::milliseconds(10)));
    }

    // when
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    isRunning.exchange(false);
    future->wait();

    // then
    ASSERT_TRUE(result);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
} // namespace common::test