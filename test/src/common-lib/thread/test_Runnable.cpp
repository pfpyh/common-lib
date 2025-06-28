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
#include <thread>
#include <iostream>

#include "common/thread/Runnable.hpp"

namespace common::test
{
TEST(test_Runnable, run)
{
    // given
    class TestRunnable : public Runnable
    {
    public :
        bool value = false;

    private :
        auto __work() -> void override
        {
            value = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    };

    auto runnable = TestRunnable();

    // when
    auto future = runnable.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    const auto running = runnable.status();
    runnable.stop();
    future.wait();
    const auto stopped = runnable.status();

    // then
    ASSERT_TRUE(runnable.value);
    ASSERT_TRUE(running);
    ASSERT_FALSE(stopped);
}

TEST(test_Runnable, already_running)
{
    // given
    class TestRunnable : public Runnable
    {
    public :
        bool value = false;

    private :
        auto __work() -> void override
        {
            value = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    };

    auto runnable = TestRunnable();
    bool exceptionRised = false;

    // when
    auto future = runnable.start();
    try 
    { 
        runnable.start(); 
    }
    catch(const AlreadyRunningException& e) 
    { 
        exceptionRised = true; 
    }
    runnable.stop();
    future.wait();

    // then
    ASSERT_TRUE(exceptionRised);
}

TEST(test_ActiveRunnable, run)
{
    // given
    class TestRunnable : public ActiveRunnable<int32_t>
    {
    public :
        int32_t _last = -1;

    private :
        auto __work(int32_t&& data) -> void override
        {
            _last = data;
        }
    };

    auto runnable = TestRunnable();
    std::vector<int32_t> dataList = {0, 1, 2, 3, 4, 5};

    // when
    auto future = runnable.start();
    for(auto i : dataList) { runnable.notify(i); }
    const auto running = runnable.status();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    runnable.stop();
    future.wait();
    const auto stopped = runnable.status();

    // then
    ASSERT_EQ(runnable._last, 5);
    ASSERT_TRUE(running);
    ASSERT_FALSE(stopped);
}

TEST(test_ActiveRunnable, run_dataType)
{
    // given
    class TestRunnable : public ActiveRunnable<std::tuple<uint8_t, int8_t>>
    {
    public :
        std::tuple<uint8_t, int8_t> _last;

    private :
        auto __work(std::tuple<uint8_t, int8_t>&& data) -> void override
        {
            _last = std::move(data);
        }
    };

    auto runnable = TestRunnable();
    std::vector<std::tuple<uint8_t, int8_t>> dataList = {
        {0, 0},
        {1, -1},
        {2, -2},
        {3, -3},
        {4, -4},
        {5, -5}
    };

    // when
    auto future = runnable.start();
    for(auto i : dataList) { runnable.notify(i); }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    runnable.stop();
    future.wait();

    // then
    ASSERT_EQ(std::get<0>(runnable._last), 5);
    ASSERT_EQ(std::get<1>(runnable._last), -5);
}

TEST(test_ActiveRunnable, already_running)
{
    // given
    class TestRunnable : public ActiveRunnable<int32_t>
    {
    public :
        int32_t _last = -1;

    private :
        auto __work(int32_t&& data) -> void override
        {
            _last = data;
        }
    };

    auto runnable = TestRunnable();
    bool exceptionRised = false;
    std::vector<int32_t> dataList = {0, 1, 2, 3, 4, 5};

    // when
    auto future = runnable.start();
    try 
    { 
        runnable.start(); 
    }
    catch(const AlreadyRunningException& e) 
    {
        exceptionRised = true;
    }
    for(auto i : dataList) { runnable.notify(i); }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    runnable.stop();
    future.wait();

    // then
    ASSERT_EQ(runnable._last, 5);
    ASSERT_TRUE(exceptionRised);
}
} // namespace common::test

