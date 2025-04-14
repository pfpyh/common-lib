#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "hal/imu/Imu.hpp"

#include "mock_Serial.hpp"

namespace common::hal::test
{
TEST(test_Imu, common_test)
{
    // given
    auto mockSerial = std::make_shared<mock::MockSerial>();
    EXPECT_CALL(*mockSerial, readline())
        .WillOnce(testing::Return("100 -100 100 -100 100 -100"));

    auto imu = Imu::create(mockSerial);

    // when
    class Receiver : public Observer<Receiver, ImuData>
    {
    public :
        ImuData _last;
        bool _received = false;
        std::weak_ptr<Imu> _imu;

    public :
        Receiver(decltype(_imu) imu)
            : _imu(imu) {}

    public :
        auto onEvent(ImuData data) -> void override
        {
            _imu.lock()->stop();
            _last = data;
            _received = true;
        }
    };

    auto receiver = Receiver::create(imu);
    imu->subscribe_update_imuData(receiver);
    auto future = imu->run();

    // then
    future.wait();
    ASSERT_TRUE(receiver->_received);
}
} // namespace common::hal::test