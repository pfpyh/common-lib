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

#include "common/Observer.hpp"

namespace common::test
{
TEST(test_Observer, notify)
{
    // given
    class TestObserver : public Observer<TestObserver, int32_t>
    {
        friend class Observer<TestObserver, int32_t>;

    public :
        int32_t _last = -1;

    private :
        auto onEvent(int32_t data) -> void override { _last = data; }
    };

    Subject<int32_t> subject;
    std::vector<std::shared_ptr<TestObserver>> observers;
    const uint8_t numberOfObservers = 10;

    // when
    for(uint8_t i = 0; i < numberOfObservers; ++i)
    {
        auto observer = TestObserver::create();
        subject.regist(observer);
        observers.push_back(std::move(observer));
    }

    subject.notify(10);

    for(uint8_t i = 0; i < numberOfObservers; ++i)
    {
        subject.unregist(observers[i]);
    }

    // then
    for(uint8_t i = 0; i < numberOfObservers; ++i)
    {
        ASSERT_EQ(observers[i]->_last, 10);
    }
}
} // namespace common::test