/**********************************************************************
MIT License

Copyright (c) 2026 Park Younghwan

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

#include "common/lifecycle/Resource.hpp"
#include "common/Exception.hpp"
#include "common/threading/TaskExecutor.hpp"

namespace common::lifecycle::test
{
TEST(test_Resource, recommended_control)
{
    // given
    class Object : public std::enable_shared_from_this<Object>
    {
    private :
        Resource<Object> _rsc;

    public :
        Object() noexcept = default;
        ~Object() noexcept { _rsc.release(); /* (optional) */ }

        auto something_do() -> void { _rsc.track(shared_from_this()); }
    };

    // when & then
    auto obj = std::make_shared<Object>();
    ASSERT_NO_THROW(ResourceManager::get_instance()->audit());
    
    obj->something_do();
    ASSERT_THROW(ResourceManager::get_instance()->audit(), 
                 BadHandlingException);
    
    obj.reset();
    ASSERT_NO_THROW(ResourceManager::get_instance()->audit());
}

TEST(test_Resource, manually_control)
{
    // given
    class Object
    {
    public :
        Object() noexcept = default;
        ~Object() noexcept = default;
    };

    // when & then
    auto obj = std::make_shared<Object>();
    ASSERT_NO_THROW(ResourceManager::get_instance()->audit());

    Resource<Object> rsc;
    rsc.track(obj);
    ASSERT_THROW(ResourceManager::get_instance()->audit(), 
                 BadHandlingException);

    rsc.release();
    ASSERT_NO_THROW(ResourceManager::get_instance()->audit());
}

TEST(test_Resource, thread_safety)
{
    // given
    class Object : public std::enable_shared_from_this<Object>
    {
    private :
        Resource<Object> _rsc;

    public :
        Object() noexcept = default;
        ~Object() noexcept { _rsc.release(); /* (optional) */ }

        auto something_do() -> void 
        {
             _rsc.track(shared_from_this()); 
             std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    };

    // when
    std::atomic<bool> running{true};
    std::vector<std::future<void>> futures;

    auto executor = threading::TaskExecutor::create(4);
    for(uint8_t i = 0; i < 4; ++i)
    {
        futures.push_back(std::move(executor->load<void>([&running]() {
            while(running.load())
            {
                auto obj = std::make_shared<Object>();
                obj->something_do();
                obj.reset();
            }
        })));

        // wait to thread starting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // then
    ASSERT_THROW(ResourceManager::get_instance()->audit(), 
                 BadHandlingException);

    running.store(false);
    for(auto& future : futures) { future.wait(); }
    executor->stop();

    // thread will release resource when loop is done.
    ASSERT_NO_THROW(ResourceManager::get_instance()->audit());
}
} // namespace common::lifecycle::test