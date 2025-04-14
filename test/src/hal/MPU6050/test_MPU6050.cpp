#include <gtest/gtest.h>

#include "hal/imu/MPU6050.hpp"

namespace common::hal::test
{
TEST(test_MPU6050, parse)
{
    // given
    constexpr auto message = "16384 -16384 16384 131 -131 131";

    // when
    const auto [accData, gyroData] = MPU6050::parse(message);

    // then
    ASSERT_EQ(accData._x, 1.0);
    ASSERT_EQ(accData._y, -1.0);
    ASSERT_EQ(accData._z, 1.0);

    ASSERT_EQ(gyroData._x, 1.0);
    ASSERT_EQ(gyroData._y, -1.0);
    ASSERT_EQ(gyroData._z, 1.0);
}
} // namespace common::hal::test