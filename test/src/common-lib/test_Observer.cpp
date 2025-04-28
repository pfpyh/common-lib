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