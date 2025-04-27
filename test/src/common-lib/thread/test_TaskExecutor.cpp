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

#include "common/thread/TaskExecutor.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

namespace common::test
{
TEST(TaskExecutorTest, Load)
{
    { // Case 1: Load with a simple function
        // given
        auto executor = TaskExecutor::create(4);
        std::vector<std::future<void>> futures;
        
        // when
        auto thread = Thread::create();
        auto testFuture = thread->start([&executor, &futures](){
            for(uint8_t i = 0; i < 10; ++i)
            {
                auto future = executor->load<void>([]() { 
                    auto sleepDuration = std::chrono::milliseconds(100);
                    std::this_thread::sleep_for(sleepDuration);
                });
                futures.push_back(std::move(future));
            }
        });
        testFuture.wait();

        // then
        for(auto& future : futures) 
        { 
            auto state = future.wait_for(std::chrono::seconds(3));
            ASSERT_EQ(state, std::future_status::ready);
        }
        executor->stop();
    }
    { // Case 2: Load with different return types
        // given
        auto executor = TaskExecutor::create(4);
        std::vector<std::future<void>> futures;

        // when
        auto future_int32_t = executor->load<int32_t>([]() {
            auto sleepDuration = std::chrono::milliseconds(100 + (std::rand() % 101));
            std::this_thread::sleep_for(sleepDuration);
            return 42;
        });

        auto future_void = executor->load<void>([]() {
            auto sleepDuration = std::chrono::milliseconds(100 + (std::rand() % 101));
            std::this_thread::sleep_for(sleepDuration);
        });

        auto future_string = executor->load<std::string>([]() {
            auto sleepDuration = std::chrono::milliseconds(100 + (std::rand() % 101));
            std::this_thread::sleep_for(sleepDuration);
            return std::string("Hello, World!");
        });

        // then
        ASSERT_EQ(future_int32_t.wait_for(std::chrono::seconds(1)), std::future_status::ready);
        ASSERT_EQ(future_int32_t.get(), 42);
        ASSERT_EQ(future_void.wait_for(std::chrono::seconds(1)), std::future_status::ready);
        ASSERT_NO_THROW(future_void.get());
        ASSERT_EQ(future_string.wait_for(std::chrono::seconds(1)), std::future_status::ready);
        ASSERT_EQ(future_string.get(), "Hello, World!");
    }
}
} // namespace common::test