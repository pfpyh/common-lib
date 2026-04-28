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

#include "common/threading/Runnable.hpp"

namespace common::threading::test
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

    // when
    auto runnable = TestRunnable();
    auto future = runnable.run();
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

    bool exceptionRised = false;

    // when
    auto runnable = TestRunnable();
    auto future = runnable.run();
    try 
    { 
        runnable.run(); 
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

TEST(test_ActiveRunnable, run_with_data_type_and_return_type)
{
    // given
    class TestRunnable : public ActiveRunnable<int32_t, int32_t>
    {
    public :
        int32_t _last = -1;

    private :
        auto __work(int32_t&& data) -> int32_t override
        {
            _last = data;
            return _last;
        }
    };

    const std::vector<int32_t> dataList = {0, 1, 2, 3, 4, 5};

    // when
    std::vector<int32_t> recvList;
    recvList.reserve(dataList.size());

    auto runnable = TestRunnable();
    auto future = runnable.run();
    const auto running = runnable.status();
    for(auto i : dataList) 
    { 
        auto notify_future = runnable.notify(i); 
        recvList.push_back(notify_future.get());
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    runnable.stop();
    future.wait();
    const auto stopped = runnable.status();

    // then
    ASSERT_EQ(runnable._last, 5);
    ASSERT_TRUE(running);
    ASSERT_FALSE(stopped);
    for(size_t i = 0; i < dataList.size(); ++i) { ASSERT_EQ(dataList[i], recvList[i]); }
}

TEST(test_ActiveRunnable, run_with_data_type)
{
    // given
    class TestRunnable : public ActiveRunnable<int32_t, void>
    {
    public :
        uint8_t _last = 0;

    private :
        auto __work(int32_t&& data) -> void override { _last = data; }
    };

    const std::vector<int32_t> dataList = {0, 1, 2, 3, 4, 5};

    // when
    auto runnable = TestRunnable();
    auto future = runnable.run();
    const auto running = runnable.status();
    for(auto i : dataList)
    {
        auto notify_future = runnable.notify(i);
        notify_future.wait();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    runnable.stop();
    future.wait();
    const auto stopped = runnable.status();

    // then
    ASSERT_EQ(runnable._last, 5);
    ASSERT_TRUE(running);
    ASSERT_FALSE(stopped);
}

TEST(test_ActiveRunnable, run_with_return_type)
{
    // given
    class TestRunnable : public ActiveRunnable<void, int32_t>
    {
    public :
        uint8_t _count = 0;

    private :
        auto __work() -> int32_t override { return _count++; }
    };

    constexpr size_t iteration = 5;

    // when
    std::vector<int32_t> recvList;
    recvList.reserve(iteration);

    auto runnable = TestRunnable();
    auto future = runnable.run();
    const auto running = runnable.status();
    for(size_t i = 0; i < iteration; ++i)
    {
        auto notify_future = runnable.notify();
        recvList.push_back(notify_future.get());
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    runnable.stop();
    future.wait();
    const auto stopped = runnable.status();

    // then
    ASSERT_EQ(runnable._count, iteration);
    ASSERT_TRUE(running);
    ASSERT_FALSE(stopped);
    for(size_t i = 0; i < iteration; ++i) { ASSERT_EQ(i, recvList[i]); }
}

TEST(test_ActiveRunnable, run_with_void)
{
    // given
    class TestRunnable : public ActiveRunnable<void, void>
    {
    public :
        uint8_t _count = 0;

    private :
        auto __work() -> void override { ++_count; }
    };

    constexpr size_t iteration = 5;

    // when
    auto runnable = TestRunnable();
    auto future = runnable.run();
    const auto running = runnable.status();
    for(size_t i = 0; i < iteration; ++i)
    {
        auto notify_future = runnable.notify();
        notify_future.wait();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    runnable.stop();
    future.wait();
    const auto stopped = runnable.status();

    // then
    ASSERT_EQ(runnable._count, iteration);
    ASSERT_TRUE(running);
    ASSERT_FALSE(stopped);
}

TEST(test_ActiveRunnable, run_dataType)
{
    // given
    class TestRunnable : public ActiveRunnable<std::tuple<uint8_t, int8_t>, void>
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
    auto future = runnable.run();
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
    class TestRunnable : public ActiveRunnable<int32_t, void>
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
    auto future = runnable.run();
    try 
    { 
        runnable.run(); 
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
} // namespace common::threading::test

