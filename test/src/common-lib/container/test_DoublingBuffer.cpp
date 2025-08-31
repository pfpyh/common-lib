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

#include "common/container/DoublingBuffer.hpp"
#include "common/thread/Thread.hpp"

namespace common::test
{
TEST(test_DoublingBuffer, pushAndErase)
{
    // given
    DoublingBuffer<int32_t> dBuffer;

    // when
    dBuffer.push(10);

    // then
    ASSERT_FALSE(dBuffer.erase(1));
    ASSERT_TRUE(dBuffer.erase(0));

    auto list = dBuffer.get_buffer();
    ASSERT_TRUE(list->empty());
}

TEST(test_DoublingBuffer, multiReader_50threads)
{
    // given
    DoublingBuffer<int32_t> dBuffer;
    dBuffer.push(0); // buffer will not going to empty

    // when
    std::atomic<bool> running{true};
    auto writer = Thread::create();
    auto writerFuture = writer->start([&dBuffer, &running](){
        bool positive = true;
        uint8_t count = 0;
        while(running.load())
        {
            if(positive) 
            { 
                dBuffer.push(++count);
            }
            else 
            {
                 dBuffer.erase(count--);
            }
            if(count == 255 || count == 0) { positive = !positive; }
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::vector<std::future<void>> futures;
    for(uint8_t threadCnt = 0; threadCnt < 50; ++threadCnt)
    {
        auto reader = Thread::create();
        futures.push_back(std::move(reader->start([&dBuffer](){
            for(uint8_t itor = 0; itor < 100; ++itor)
            {
                auto list = dBuffer.get_buffer();
                for(uint8_t i = 0; i < list->size(); ++i)
                {
                    if(i + 1 == list->size()) { break; }
                    ASSERT_TRUE(list->at(i) < list->at(i + 1));
                }
            }
        })));
    }

    for(auto& future : futures) { future.wait(); }
    running.store(false);
    writerFuture.wait();
}

TEST(test_DoublingBuffer, multiReader_255threads)
{
    // given
    DoublingBuffer<int32_t, 4> dBuffer;
    dBuffer.push(0); // buffer will not going to empty

    // when
    std::atomic<bool> running{true};
    auto writer = Thread::create();
    auto writerFuture = writer->start([&dBuffer, &running](){
        bool positive = true;
        uint8_t count = 0;
        while(running.load())
        {
            if(positive) 
            { 
                dBuffer.push(++count);
            }
            else 
            {
                 dBuffer.erase(count--);
            }
            if(count == 255 || count == 0) { positive = !positive; }
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::vector<std::future<void>> futures;
    for(uint8_t threadCnt = 0; threadCnt < 50; ++threadCnt)
    {
        auto reader = Thread::create();
        futures.push_back(std::move(reader->start([&dBuffer](){
            for(uint8_t itor = 0; itor < 100; ++itor)
            {
                auto list = dBuffer.get_buffer();
                for(uint8_t i = 0; i < list->size(); ++i)
                {
                    if(i + 1 == list->size()) { break; }
                    ASSERT_TRUE(list->at(i) < list->at(i + 1));
                }
            }
        })));
    }

    for(auto& future : futures) { future.wait(); }
    running.store(false);
    writerFuture.wait();
}
} // namespace common::test