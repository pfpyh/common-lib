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

#include "common/threading/TaskExecutor.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <climits>
#include <thread>

namespace common::threading::test
{
TEST(test_TaskExecutor, LoadSimpleFunction)
{
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

TEST(test_TaskExecutor, LoadWithMultiTypesReturn)
{
    // given
    auto executor = TaskExecutor::create(4);

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

    executor->stop();
}

TEST(test_TaskExecutor, LoadByMultpleThread)
{
    // given
    auto executor = TaskExecutor::create(4);
    const int32_t tasksPerThread = 50;
    const int32_t numThreads = 4;
    std::vector<std::vector<std::future<int32_t>>> futures(numThreads);
    std::vector<std::shared_ptr<Thread>> threads;
    std::vector<std::future<void>> threadFutures;
    
    // when
    for(int32_t threadId = 0; threadId < numThreads; ++threadId) {
        auto thread = Thread::create();
        auto future = thread->start([threadId, tasksPerThread, &executor, &futures]() {
            for(int32_t taskId = 0; taskId < tasksPerThread; ++taskId) {
                auto taskFuture = executor->load<int32_t>([threadId, taskId]() {
                    auto start = std::chrono::high_resolution_clock::now();
                    while(true) {
                        auto now = std::chrono::high_resolution_clock::now();
                        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - start);
                        if(elapsed.count() >= 1000) break; // 1ms = 1000 microseconds
                    }
                    return threadId * 1000 + taskId;
                });
                futures[threadId].push_back(std::move(taskFuture));
            }
        });
        threads.push_back(thread);
        threadFutures.push_back(std::move(future));
    }
    
    for(auto& future : threadFutures) {
        future.wait();
    }
    
    // then
    auto startTime = std::chrono::high_resolution_clock::now();
    for(int32_t threadId = 0; threadId < numThreads; ++threadId) {
        for(int32_t taskId = 0; taskId < tasksPerThread; ++taskId) {
            auto status = futures[threadId][taskId].wait_for(std::chrono::seconds(5));
            ASSERT_EQ(status, std::future_status::ready);
            
            int32_t result = futures[threadId][taskId].get();
            int32_t expected = threadId * 1000 + taskId;
            ASSERT_EQ(result, expected);
        }
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Total execution time: " << totalTime.count() << "ms for " 
                << (numThreads * tasksPerThread) << " tasks" << std::endl;
    
    ASSERT_LT(totalTime.count(), 150);

    executor->stop();
}

TEST(test_TaskExecutor, WorkStealing)
{
    // given
    auto executor = TaskExecutor::create(4);
    const int32_t totalTasks = 200;
    std::vector<std::future<int32_t>> futures;
    std::atomic<int32_t> taskCounter{0};
    std::array<std::atomic<int32_t>, 4> threadWorkCount{};
    
    // when
    for(int32_t i = 0; i < totalTasks; ++i) {
        auto future = executor->load<int32_t>([i, &taskCounter, &threadWorkCount]() {
            thread_local static int32_t threadId = -1;
            if (threadId == -1) {
                static std::atomic<int32_t> nextId{0};
                threadId = nextId.fetch_add(1);
            }
                if (threadId < 4) {
                threadWorkCount[threadId].fetch_add(1);
            }

            // Wait only 4th thread 100ms to check work-stealing
            if (threadId == 3) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            } else {
                int randomMs = 1 + (std::rand() % 50);
                std::this_thread::sleep_for(std::chrono::milliseconds(randomMs));
            }
            
            return taskCounter.fetch_add(1);
        });
        futures.push_back(std::move(future));
    }
    
    // then
    std::set<int32_t> results;
    for(auto& future : futures) {
        auto status = future.wait_for(std::chrono::seconds(10));
        ASSERT_EQ(status, std::future_status::ready);
        results.insert(future.get());
    }
    
    ASSERT_EQ(results.size(), totalTasks);
    ASSERT_EQ(*results.begin(), 0);
    ASSERT_EQ(*results.rbegin(), totalTasks - 1);
    
    int32_t activeThreads = 0;
    for(int32_t i = 0; i < 4; ++i) {
        int32_t workCount = threadWorkCount[i].load();
        std::cout << "Thread " << i << " processed " << workCount << " tasks" << std::endl;
        if (workCount > 0) {
            activeThreads++;
        }
    }
    
    ASSERT_GE(activeThreads, 3);
    
    executor->stop();
}
} // namespace common::threading::test