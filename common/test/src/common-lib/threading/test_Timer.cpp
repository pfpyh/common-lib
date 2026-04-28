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

#include "common/threading/Timer.hpp"
#include "common/Logger.hpp"

#include <thread>

namespace common::threading::test
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
    isRunning.store(false);
    future->wait();

    // then
    ASSERT_TRUE(result);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
} // namespace common::threading::test